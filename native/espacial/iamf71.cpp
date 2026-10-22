// iamf71: IAMF (Alliance for Open Media) → WAV 7.1 en orden de Windows, listo para el domo.
//   iamf71.exe entrada.iamf salida.wav
// Decodifica con iamfdec.exe (libiamf v2.0.0, solo PCM en este build) al sistema ITU «I» (0+7+0) y
// reordena los canales: libiamf entrega L R C LFE Lss Rss Lrs Rrs (laterales antes que traseros);
// Windows/WAVE 0x63F espera FL FR FC LFE BL BR SL SR (traseros antes que laterales).
// Fuente del orden: oar/src/renderer/ear/m2m_rdr.c (0+7+0: M+030 M-030 M+000 LFE1 M+090 M-090 M+135 M-135).
#include "../upmix/audio71.hpp"
#include <filesystem>
#include <windows.h>

namespace fs = std::filesystem;

static Multi lee8(const fs::path& ruta, unsigned& rate) {
    std::ifstream f(ruta, std::ios::binary);
    std::vector<char> d((std::istreambuf_iterator<char>(f)), {});
    auto u16 = [&](size_t o) { return uint32_t(uint8_t(d[o]) | uint8_t(d[o + 1]) << 8); };
    auto u32 = [&](size_t o) { return u16(o) | u16(o + 2) << 16; };
    unsigned fmt = 0, ch = 0, bits = 0; size_t data = 0, len = 0;
    if (d.size() < 12 || std::memcmp(d.data() + 8, "WAVE", 4)) throw std::runtime_error("iamfdec no produjo un WAV válido.");
    for (size_t o = 12; o + 8 <= d.size();) {
        size_t s = u32(o + 4);
        if (!std::memcmp(d.data() + o, "fmt ", 4)) { fmt = u16(o + 8); ch = u16(o + 10); rate = u32(o + 12); bits = u16(o + 22); if (fmt == 0xFFFE) fmt = u16(o + 32); }
        if (!std::memcmp(d.data() + o, "data", 4)) { data = o + 8; len = std::min(s, d.size() - data); }
        o += 8 + s + (s & 1);
    }
    if (ch != 8 || fmt != 1 || (bits != 16 && bits != 24 && bits != 32)) throw std::runtime_error("Se esperaba PCM de 8 canales de iamfdec (llegaron " + std::to_string(ch) + " canales, " + std::to_string(bits) + " bit).");
    const unsigned B = bits / 8; const size_t frames = len / (8 * B);
    // ITU 0+7+0 → Windows: L R C LFE | Lrs Rrs | Lss Rss
    const int desde[8] = { 0, 1, 2, 3, 6, 7, 4, 5 };
    Multi m(frames);
    for (size_t i = 0; i < frames; ++i)
        for (int c = 0; c < 8; ++c) {
            size_t o = data + (i * 8 + size_t(desde[c])) * B; float v;
            if (B == 2) v = int16_t(u16(o)) / 32768.f;
            else if (B == 3) v = float(int32_t(uint32_t(uint8_t(d[o])) << 8 | uint32_t(uint8_t(d[o + 1])) << 16 | uint32_t(uint8_t(d[o + 2])) << 24) / 2147483648.);
            else v = float(int32_t(u32(o)) / 2147483648.);
            m[i][c] = v;
        }
    return m;
}

int main(int argc, char** argv) {
    if (argc != 3) { std::printf("Uso: iamf71 entrada.iamf salida.wav\n"); return 2; }
    try {
        wchar_t exe[MAX_PATH]; GetModuleFileNameW(nullptr, exe, MAX_PATH);
        fs::path dec = fs::path(exe).parent_path() / L"iamfdec.exe";
        if (!fs::exists(dec)) throw std::runtime_error("Falta iamfdec.exe junto a iamf71.exe (se compila con native\\build-iamf.ps1).");
        fs::path entrada = fs::absolute(fs::u8path(argv[1])), salida = fs::absolute(fs::u8path(argv[2]));
        if (!fs::exists(entrada)) throw std::runtime_error("No existe " + entrada.u8string());
        // iamfdec no escribe en rutas cortas 8.3 (p. ej. LUISMA~1): el temporal va junto a la salida.
        fs::path tmp = salida.parent_path() / (salida.stem().wstring() + L".itu-tmp.wav");
        std::wstring cmd = L"\"\"" + dec.wstring() + L"\" -o3 \"" + tmp.wstring() + L"\" -s8 \"" + entrada.wstring() + L"\" >NUL 2>&1\"";
        int r = _wsystem(cmd.c_str());
        if (r != 0 || !fs::exists(tmp)) throw std::runtime_error("iamfdec falló (código " + std::to_string(r) + "). ¿El IAMF usa Opus/AAC/FLAC? Este build solo decodifica PCM.");
        unsigned rate = 48000;
        Multi m = lee8(tmp, rate);
        fs::remove(tmp);
        Stats st = measure(m);
        writeWav71(salida.u8string(), m, rate);
        std::printf("IAMF → 7.1: %s\n   %.1f s a %u Hz · reordenado ITU (laterales, traseros) → Windows (traseros, laterales)\n", salida.u8string().c_str(), double(m.size()) / rate, rate);
        for (int c = 0; c < NCH; ++c) std::printf("   %-3s pico %7.2f dBFS  RMS %7.2f dBFS\n", Names[c], db(st.peak[c]), db(st.rms[c]));
        return st.nonFinite ? 1 : 0;
    } catch (const std::exception& e) { std::printf("FALLA: %s\n", e.what()); return 1; }
}
