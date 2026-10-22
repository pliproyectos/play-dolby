#pragma once
// Fuente archivo de Play Dolby: un WAV (o una carpeta de capas de capas71) hace el papel de la
// consola. Sus canales son «USB 1, 2, 3…». Todo se carga a memoria antes de iniciar; el callback de
// salida solo copia de memoria al núcleo del puente (sin disco, sin candados, sin reservar memoria),
// al ritmo de la salida HDMI: no hay deriva de reloj entre «consola» y salida.
#include "bridge_core.hpp"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace fuente {

struct Archivo {
    std::string nombre;
    unsigned rate = 0, canales = 0;
    std::vector<float> datos;            // intercalado: cuadro × canales
    std::vector<std::string> etiquetas;  // nombre de cada canal («voz L»…)
    size_t cuadros() const { return canales ? datos.size() / canales : 0; }
};

// WAV/RF64/BW64 PCM 16/24/32 o flotante de 32 bit, de 1 a 32 canales.
inline Archivo leeWav(const std::filesystem::path& ruta) {
    std::ifstream f(ruta, std::ios::binary);
    if (!f) throw std::runtime_error("No se pudo abrir " + ruta.u8string());
    std::vector<char> d((std::istreambuf_iterator<char>(f)), {});
    auto u16 = [&](size_t o) { return uint32_t(uint8_t(d[o]) | uint8_t(d[o + 1]) << 8); };
    auto u32 = [&](size_t o) { return u16(o) | u16(o + 2) << 16; };
    if (d.size() < 12 || std::memcmp(d.data() + 8, "WAVE", 4)) throw std::runtime_error(ruta.filename().u8string() + " no es un WAV.");
    unsigned fmt = 0, ch = 0, bits = 0, rate = 0; size_t data = 0, len = 0; uint64_t grande = 0;
    for (size_t o = 12; o + 8 <= d.size();) {
        uint64_t s = u32(o + 4);
        if (!std::memcmp(d.data() + o, "ds64", 4)) grande = uint64_t(u32(o + 16)) | uint64_t(u32(o + 20)) << 32;
        if (!std::memcmp(d.data() + o, "data", 4) && s == 0xFFFFFFFF) s = grande;
        if (o + 8 + s > d.size()) s = d.size() - o - 8;
        if (!std::memcmp(d.data() + o, "fmt ", 4)) { fmt = u16(o + 8); ch = u16(o + 10); rate = u32(o + 12); bits = u16(o + 22); if (fmt == 0xFFFE) fmt = u16(o + 32); }
        if (!std::memcmp(d.data() + o, "data", 4)) { data = o + 8; len = size_t(s); }
        o += 8 + size_t(s) + (s & 1);
    }
    const bool flotante = fmt == 3 && bits == 32, pcm = fmt == 1 && (bits == 16 || bits == 24 || bits == 32);
    if ((!pcm && !flotante) || ch < 1 || ch > 32 || !rate) throw std::runtime_error(ruta.filename().u8string() + ": formato no soportado (PCM 16/24/32 o flotante 32, 1 a 32 canales).");
    Archivo a; a.nombre = ruta.filename().u8string(); a.rate = rate; a.canales = ch;
    const unsigned B = bits / 8; const size_t n = len / (size_t(ch) * B);
    a.datos.resize(n * ch);
    for (size_t i = 0; i < n * ch; ++i) {
        size_t o = data + i * B; float v;
        if (flotante) { uint32_t u = u32(o); std::memcpy(&v, &u, 4); }
        else if (B == 2) v = int16_t(u16(o)) / 32768.f;
        else if (B == 3) v = float(int32_t(uint32_t(uint8_t(d[o])) << 8 | uint32_t(uint8_t(d[o + 1])) << 16 | uint32_t(uint8_t(d[o + 2])) << 24) / 2147483648.);
        else v = float(int32_t(u32(o)) / 2147483648.);
        a.datos[i] = std::isfinite(v) ? v : 0.f;
    }
    for (unsigned c = 0; c < ch; ++c) a.etiquetas.push_back("canal " + std::to_string(c + 1));
    return a;
}

// Las 6 capas de capas71, en el orden que usa la configuración automática:
// USB 1-2 voz · 3-4 colchón · 5-6 brillos · 7-8 percusión · 9-10 bajo · 11-12 bombo.
inline const char* Capas[6] = { "voz", "colchon", "brillos", "percusion", "bajo", "bombo" };

inline Archivo leeCapas(const std::filesystem::path& carpeta) {
    Archivo a; a.nombre = carpeta.parent_path().filename().u8string() + " (capas)"; a.canales = 12;
    std::vector<Archivo> partes; std::string faltan;
    for (auto c : Capas) {
        auto r = carpeta / (std::string(c) + ".wav");
        if (!std::filesystem::exists(r)) { faltan += std::string(faltan.empty() ? "" : ", ") + c + ".wav"; continue; }
        partes.push_back(leeWav(r));
    }
    if (!faltan.empty()) throw std::runtime_error("La carpeta no es de capas71: faltan " + faltan + ".");
    size_t n = 0;
    for (auto& p : partes) {
        if (p.canales != 2) throw std::runtime_error(p.nombre + " debe ser estéreo.");
        if (a.rate && p.rate != a.rate) throw std::runtime_error("Las capas tienen frecuencias distintas.");
        a.rate = p.rate; n = std::max(n, p.cuadros());
    }
    a.datos.assign(n * 12, 0.f);
    for (size_t k = 0; k < partes.size(); ++k) {
        for (size_t i = 0; i < partes[k].cuadros(); ++i) { a.datos[i * 12 + k * 2] = partes[k].datos[i * 2]; a.datos[i * 12 + k * 2 + 1] = partes[k].datos[i * 2 + 1]; }
        a.etiquetas.push_back(std::string(Capas[k]) + " L"); a.etiquetas.push_back(std::string(Capas[k]) + " R");
    }
    return a;
}

// Alimenta el núcleo desde memoria, dentro del callback de salida, antes de core.render().
// Mantiene el anillo con la cola que el núcleo pide (salida + bloque de alimentación + margen).
class Reproductor {
public:
    static constexpr unsigned Bloque = 256;
    void prepara(const Archivo* a, bool repetir) { a_ = a; pos_ = 0; repetir_ = repetir; terminado_ = false; }
    void alimenta(bridge::Core& core, unsigned frames) {
        if (!a_ || !a_->canales) return;
        const uint64_t meta = uint64_t(frames) + Bloque + core.margin + Bloque;
        const size_t total = a_->cuadros();
        size_t pos = pos_.load(std::memory_order_relaxed);
        while (core.ring.available() < meta) {
            if (pos >= total) { if (!repetir_ || !total) { terminado_ = true; silencio(core); break; } pos = 0; }
            unsigned n = unsigned(std::min<size_t>(Bloque, total - pos));
            core.capture(a_->datos.data() + pos * a_->canales, n, a_->canales);
            pos += n;
        }
        pos_.store(pos, std::memory_order_relaxed);
    }
    double segundos() const { return a_ && a_->rate ? double(pos_.load(std::memory_order_relaxed)) / a_->rate : 0; }  // lo lee la interfaz
    bool terminado() const { return terminado_; }
private:
    void silencio(bridge::Core& core) {  // al terminar sin repetir: cuadros en cero para que la salida baje limpia
        static const float cero[Bloque * 32] = {};
        while (core.ring.available() < uint64_t(Bloque) * 4) core.capture(cero, Bloque, a_->canales);
    }
    const Archivo* a_ = nullptr; std::atomic<size_t> pos_{ 0 }; bool repetir_ = true; std::atomic<bool> terminado_{ false };
};

}  // namespace fuente
