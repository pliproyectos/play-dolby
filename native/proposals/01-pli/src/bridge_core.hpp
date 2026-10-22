#pragma once
#include <array>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace bridge {
constexpr unsigned Channels = 8;
constexpr unsigned Capacity = 32768;
using Frame = std::array<float, Channels>;

// One producer (capture callback), one consumer (render callback).
class Ring {
    std::vector<Frame> frames{Capacity};
    alignas(64) std::atomic<uint64_t> write{0};
    alignas(64) std::atomic<uint64_t> read{0};
public:
    void reset() { write = 0; read = 0; } // Streams must be stopped.
    uint64_t available() const { return write.load(std::memory_order_acquire) - read.load(std::memory_order_relaxed); }
    bool push(const Frame& f) {
        auto w = write.load(std::memory_order_relaxed);
        if (w - read.load(std::memory_order_acquire) >= Capacity) return false;
        frames[w % Capacity] = f;
        write.store(w + 1, std::memory_order_release);
        return true;
    }
    Frame peek(unsigned offset) const { return frames[(read.load(std::memory_order_relaxed) + offset) % Capacity]; }
    void consume(unsigned n) { read.fetch_add(n, std::memory_order_release); }
};

struct Core {
    Ring ring;
    std::array<std::atomic<int>, Channels> route;
    std::array<std::atomic<float>, Channels> peak;
    std::atomic<float> gain{0.12589254f}; // -18 dB initial master gain
    std::atomic<uint64_t> underflows{0}, overflows{0}, invalid{0}, clipped{0};
    std::atomic<double> ppm{0};
    unsigned target = 2400; // 50 ms at 48 kHz
    double phase = 0, ratio = 1;
    bool primed = false;
    Core() { for (unsigned c=0;c<Channels;++c) {route[c]=c;peak[c]=0;} }
    void reset() {
        ring.reset(); phase=0; ratio=1; primed=false; ppm=0;
        underflows=0;overflows=0;invalid=0;clipped=0;
        for(auto& p:peak)p=0;
    }
    void capture(const float* in, unsigned frames, unsigned count) {
        std::array<int,Channels> map;
        for(unsigned c=0;c<Channels;++c)map[c]=route[c].load();
        for(unsigned i=0;i<frames;++i) {
            Frame f{};
            for(unsigned c=0;c<Channels;++c) {
                int k=map[c];
                if(in && k>=0 && unsigned(k)<count) {
                    float s=in[i*count+k];
                    if(std::isfinite(s))f[c]=s;else ++invalid;
                }
            }
            if(!ring.push(f))++overflows;
        }
    }
    void render(float* out, unsigned frames, unsigned count) {
        std::fill(out,out+frames*count,0.f);
        if(!primed) { if(ring.available()<target)return;primed=true; }
        // Low-bandwidth fill-level servo. Linear interpolation is provisional.
        double error=(double(ring.available())-target)/target;
        double wanted=1.+std::clamp(error*.002,-.003,.003);
        ratio += .02*(wanted-ratio);
        ppm=(ratio-1.)*1e6;
        std::array<float,Channels> peaks{};
        float g=gain.load();
        for(unsigned i=0;i<frames;++i) {
            unsigned need=unsigned(phase+ratio);
            if(ring.available()<std::max(2u,need+1)) {
                ++underflows;primed=false;phase=0;break;
            }
            auto a=ring.peek(0), b=ring.peek(1);
            for(unsigned c=0;c<count;++c) {
                float value=float(a[c]+phase*(b[c]-a[c]))*g;
                if(std::abs(value)>1.f)++clipped;
                value=std::clamp(value,-1.f,1.f);
                out[i*count+c]=value;
                peaks[c]=std::max(peaks[c],std::abs(value));
            }
            phase+=ratio;
            unsigned used=unsigned(phase);phase-=used;ring.consume(used);
        }
        for(unsigned c=0;c<count;++c)peak[c]=peaks[c];
    }
};
}
