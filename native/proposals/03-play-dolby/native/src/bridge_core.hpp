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
    uint64_t available() const { auto r=read.load(std::memory_order_acquire); return std::min<uint64_t>(Capacity,write.load(std::memory_order_acquire)-r); }
    unsigned pushBlock(const Frame* source, unsigned count) {
        auto w=write.load(std::memory_order_relaxed);
        auto free=Capacity-(w-read.load(std::memory_order_acquire));
        unsigned n=unsigned(std::min<uint64_t>(count,free));
        for(unsigned i=0;i<n;++i)frames[(w+i)%Capacity]=source[i];
        write.store(w+n,std::memory_order_release);
        return n;
    }
    bool push(const Frame& f) {
        auto w = write.load(std::memory_order_relaxed);
        if (w - read.load(std::memory_order_acquire) >= Capacity) return false;
        frames[w % Capacity] = f;
        write.store(w + 1, std::memory_order_release);
        return true;
    }
    uint64_t readPosition() const { return read.load(std::memory_order_relaxed); }
    const Frame& at(uint64_t position) const { return frames[position % Capacity]; }
    void consume(unsigned n) { read.store(read.load(std::memory_order_relaxed)+n, std::memory_order_release); }
};

struct Core {
    Ring ring;
    std::array<std::atomic<int>, Channels> route;
    std::array<std::atomic<float>, Channels> peak;
    std::atomic<float> gain{0.12589254f}; // -18 dB initial master gain
    std::atomic<uint64_t> underflows{0}, overflows{0}, invalid{0}, clipped{0};
    std::atomic<double> ppm{0};
    unsigned target = 512; // Minimum queue: 10.67 ms at 48 kHz.
    std::atomic<unsigned> captureBlock{0};
    double phase = 0, ratio = 1;
    bool primed = false;
    Core() { for (unsigned c=0;c<Channels;++c) {route[c]=c;peak[c]=0;} }
    void reset() {
        ring.reset(); target=512;captureBlock=0; phase=0; ratio=1; primed=false; ppm=0;
        underflows=0;overflows=0;invalid=0;clipped=0;
        for(auto& p:peak)p=0;
    }
    void capture(const float* in, unsigned frames, unsigned count) {
        std::array<int,Channels> map;
        for(unsigned c=0;c<Channels;++c)map[c]=route[c].load();
        if(frames>captureBlock.load(std::memory_order_relaxed))captureBlock.store(frames,std::memory_order_relaxed);
        uint64_t bad=0,dropped=0;
        // Fixed stack chunks; publish once per chunk instead of per sample.
        std::array<Frame,128> block;
        for(unsigned base=0;base<frames;base+=unsigned(block.size())) {
            unsigned n=std::min<unsigned>(block.size(),frames-base);
            for(unsigned i=0;i<n;++i) {
                Frame f{};
                for(unsigned c=0;c<Channels;++c) {
                    int k=map[c];
                    if(in && k>=0 && unsigned(k)<count) {
                        float sample=in[size_t(base+i)*count+k];
                        if(std::isfinite(sample))f[c]=sample;else ++bad;
                    }
                }
                block[i]=f;
            }
            dropped+=n-ring.pushBlock(block.data(),n);
        }
        if(bad)invalid.fetch_add(bad,std::memory_order_relaxed);
        if(dropped)overflows.fetch_add(dropped,std::memory_order_relaxed);
    }

    void render(float* out, unsigned frames, unsigned count) {
        std::fill(out,out+frames*count,0.f);
        if(count>Channels)return;
        unsigned block=std::max(frames,captureBlock.load(std::memory_order_relaxed));
        target=std::max(target,unsigned(std::min<uint64_t>(Capacity/2,std::max<uint64_t>(512,uint64_t(block)*2+2))));
        auto available=ring.available();
        if(!primed) { if(available<target){for(auto& p:peak)p=0;return;}primed=true; }
        const auto start=ring.readPosition();
        unsigned consumed=0;
        uint64_t clips=0;
        // Low-bandwidth fill-level servo. Linear interpolation is provisional.
        double error=(double(available)-target)/target;
        double wanted=1.+std::clamp(error*.01,-.003,.003);
        ratio += (1.-std::exp(-double(frames)/12672.))*(wanted-ratio);
        ppm=(ratio-1.)*1e6;
        std::array<float,Channels> peaks{};
        float g=gain.load();
        for(unsigned i=0;i<frames;++i) {
            unsigned need=unsigned(phase+ratio);
            if(available-consumed<std::max(2u,need+1)) {
                ++underflows;primed=false;phase=0;break;
            }
            const auto& a=ring.at(start+consumed);
            const auto& b=ring.at(start+consumed+1);
            for(unsigned c=0;c<count;++c) {
                float value=float(a[c]+phase*(b[c]-a[c]))*g;
                if(std::abs(value)>1.f)++clips;
                value=std::clamp(value,-1.f,1.f);
                out[i*count+c]=value;
                peaks[c]=std::max(peaks[c],std::abs(value));
            }
            phase+=ratio;
            unsigned used=unsigned(phase);phase-=used;consumed+=used;
        }
        ring.consume(consumed);
        if(clips)clipped.fetch_add(clips,std::memory_order_relaxed);
        for(unsigned c=0;c<count;++c)peak[c]=peaks[c];
    }
};
}

