#pragma once
// Utilidades comunes de los generadores 7.1: lectura WAV estéreo, filtros, paneo en anillo,
// medición y escritura WAV EXTENSIBLE 0x63F de 24 bit.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

constexpr double Pi = 3.14159265358979323846;
enum { FL, FR, FC, LFE, BL, BR, SL, SR, NCH };
inline const char* Names[NCH] = { "FL", "FR", "C", "LFE", "BL", "BR", "SL", "SR" };

struct Stereo { unsigned rate = 0; std::vector<float> l, r; };

inline Stereo readWav(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("No se pudo abrir " + path);
    std::vector<char> d((std::istreambuf_iterator<char>(f)), {});
    auto u16 = [&](size_t o) { return uint16_t(uint8_t(d[o]) | uint8_t(d[o + 1]) << 8); };
    auto u32 = [&](size_t o) { return uint32_t(u16(o) | uint32_t(u16(o + 2)) << 16); };
    if (d.size() < 12 || std::memcmp(d.data(), "RIFF", 4) || std::memcmp(d.data() + 8, "WAVE", 4)) throw std::runtime_error("No es WAV RIFF.");
    unsigned fmt = 0, ch = 0, bits = 0, rate = 0; size_t data = 0, len = 0;
    for (size_t o = 12; o + 8 <= d.size();) {
        uint32_t size = u32(o + 4);
        if (!std::memcmp(d.data() + o, "fmt ", 4)) { fmt = u16(o + 8); ch = u16(o + 10); rate = u32(o + 12); bits = u16(o + 22); if (fmt == 0xFFFE) fmt = u16(o + 32); }
        if (!std::memcmp(d.data() + o, "data", 4)) { data = o + 8; len = std::min<size_t>(size, d.size() - data); }
        o += 8 + size + (size & 1);
    }
    bool pcm = fmt == 1 && (bits == 16 || bits == 24), flotante = fmt == 3 && bits == 32;
    if ((!pcm && !flotante) || (ch != 1 && ch != 2)) throw std::runtime_error("Se espera WAV mono o estéreo: PCM de 16/24 bit o flotante de 32 bit (" + path + ").");
    Stereo s; s.rate = rate;
    size_t frames = len / (ch * bits / 8);
    s.l.resize(frames); s.r.resize(frames);
    for (size_t i = 0; i < frames; ++i)
        for (unsigned c = 0; c < ch; ++c) {
            size_t o = data + (i * ch + c) * (bits / 8);
            float v;
            if (flotante) { uint32_t u = u32(o); std::memcpy(&v, &u, 4); }
            else v = bits == 16 ? int16_t(u16(o)) / 32768.f
                                : float(int32_t(uint32_t(uint8_t(d[o])) << 8 | uint32_t(uint8_t(d[o + 1])) << 16 | uint32_t(uint8_t(d[o + 2])) << 24) / 2147483648.);
            (c ? s.r : s.l)[i] = v;
        }
    if (ch == 1) s.r = s.l;  // mono: el mismo en L y R
    return s;
}

struct Biquad {  // RBJ cookbook, forma directa I
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    static Biquad make(bool high, double f, double rate, double q = .7071) {
        double w = 2 * Pi * f / rate, cw = std::cos(w), al = std::sin(w) / (2 * q), a0 = 1 + al; Biquad b;
        if (high) { b.b0 = (1 + cw) / 2 / a0; b.b1 = -(1 + cw) / a0; b.b2 = b.b0; }
        else { b.b0 = (1 - cw) / 2 / a0; b.b1 = (1 - cw) / a0; b.b2 = b.b0; }
        b.a1 = -2 * cw / a0; b.a2 = (1 - al) / a0; return b;
    }
    float operator()(float x) { double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2; x2 = x1; x1 = x; y2 = y1; y1 = y; return float(y); }
};

using Multi = std::vector<std::array<float, NCH>>;

// Anillo 7.1 por ángulo (grados, horario desde el centro): C 0, FR 30, SR 90, BR 150, BL 210, SL 270, FL 330.
// Suma x en el par de bocinas que rodea el ángulo, con ley de potencia constante.
inline void panRing(std::array<float, NCH>& o, float x, double deg) {
    static const int ring[7] = { FC, FR, SR, BR, BL, SL, FL };
    static const double ang[8] = { 0, 30, 90, 150, 210, 270, 330, 360 };
    double th = std::fmod(std::fmod(deg, 360.) + 360., 360.);
    int k = 0; while (k < 6 && th >= ang[k + 1]) ++k;
    double t = (th - ang[k]) / (ang[k + 1] - ang[k]) * Pi / 2;
    o[ring[k]] += x * float(std::cos(t));
    o[ring[(k + 1) % 7]] += x * float(std::sin(t));
}

inline void writeStereo(const std::string& path, const std::vector<float>& l, const std::vector<float>& r, unsigned rate) {
    std::vector<uint8_t> data; data.reserve(l.size() * 6);
    for (size_t i = 0; i < l.size(); ++i) for (float v : { l[i], r[i] }) {
        int32_t s = int32_t(std::lround(std::clamp(double(v), -1., 1. - 1. / 8388608) * 8388608.));
        data.push_back(uint8_t(s)); data.push_back(uint8_t(s >> 8)); data.push_back(uint8_t(s >> 16));
    }
    std::ofstream f(path, std::ios::binary);
    auto w16 = [&](uint16_t v) { f.put(char(v)); f.put(char(v >> 8)); };
    auto w32 = [&](uint32_t v) { w16(uint16_t(v)); w16(uint16_t(v >> 16)); };
    f.write("RIFF", 4); w32(uint32_t(4 + 8 + 16 + 8 + data.size())); f.write("WAVE", 4);
    f.write("fmt ", 4); w32(16); w16(1); w16(2); w32(rate); w32(rate * 6); w16(6); w16(24);
    f.write("data", 4); w32(uint32_t(data.size())); f.write(reinterpret_cast<const char*>(data.data()), std::streamsize(data.size()));
    if (!f) throw std::runtime_error("No se pudo escribir " + path);
}

struct Stats { std::array<double, NCH> peak{}, rms{}; size_t nonFinite = 0; };
inline Stats measure(const Multi& m) {
    Stats st;
    for (auto& f : m) for (int c = 0; c < NCH; ++c) { if (!std::isfinite(f[c])) { ++st.nonFinite; continue; } st.peak[c] = std::max(st.peak[c], double(std::fabs(f[c]))); st.rms[c] += double(f[c]) * f[c]; }
    for (auto& r : st.rms) r = std::sqrt(r / std::max<size_t>(1, m.size()));
    return st;
}
inline double db(double v) { return v > 0 ? 20 * std::log10(v) : -999; }

inline double normalize(Multi& m) {  // sólo baja; pico global <= -1 dBFS
    double peak = 0; for (auto& f : m) for (float v : f) peak = std::max(peak, double(std::fabs(v)));
    double g = peak > .891 ? .891 / peak : 1.;
    if (g < 1) for (auto& f : m) for (auto& v : f) v = float(v * g);
    return g;
}

inline void writeWav71(const std::string& path, const Multi& m, unsigned rate) {
    std::vector<uint8_t> data; data.reserve(m.size() * NCH * 3);
    for (auto& f : m) for (float v : f) {
        int32_t s = int32_t(std::lround(std::clamp(double(v), -1., 1. - 1. / 8388608) * 8388608.));
        data.push_back(uint8_t(s)); data.push_back(uint8_t(s >> 8)); data.push_back(uint8_t(s >> 16));
    }
    std::ofstream f(path, std::ios::binary);
    auto w16 = [&](uint16_t v) { f.put(char(v)); f.put(char(v >> 8)); };
    auto w32 = [&](uint32_t v) { w16(uint16_t(v)); w16(uint16_t(v >> 16)); };
    const uint8_t pcm[16] = { 1, 0, 0, 0, 0, 0, 0x10, 0, 0x80, 0, 0, 0xAA, 0, 0x38, 0x9B, 0x71 };
    f.write("RIFF", 4); w32(uint32_t(4 + 8 + 40 + 8 + data.size())); f.write("WAVE", 4);
    f.write("fmt ", 4); w32(40); w16(0xFFFE); w16(NCH); w32(rate); w32(rate * NCH * 3); w16(NCH * 3); w16(24);
    w16(22); w16(24); w32(0x63F); f.write(reinterpret_cast<const char*>(pcm), 16);
    f.write("data", 4); w32(uint32_t(data.size())); f.write(reinterpret_cast<const char*>(data.data()), std::streamsize(data.size()));
    if (!f) throw std::runtime_error("No se pudo escribir " + path);
}
