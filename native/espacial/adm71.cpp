// adm71: PLI Espacial ↔ ADM (ITU-R BS.2076, archivo BW64/WAV con chna + axml).
//   adm71 exporta escena.json salida-adm.wav
//       Convierte una escena en un ADM: una pista por objeto (dos si es estéreo) con su trayectoria.
//   adm71 importa entrada-adm.wav carpeta [--domo pruebas-domo\domo.json]
//       Extrae las pistas a carpeta\pistas\, escribe carpeta\escena.json (editable) y la renderiza
//       a 7.1 con las bocinas de la ficha.
#include "adm.hpp"
#include <cctype>

namespace fs = std::filesystem;

static void escribeMono(const fs::path& ruta, const std::vector<float>& x, unsigned rate) {
    std::ofstream f(ruta, std::ios::binary);
    auto w16 = [&](uint32_t v) { f.put(char(v)); f.put(char(v >> 8)); };
    auto w32 = [&](uint32_t v) { w16(v & 0xFFFF); w16(v >> 16); };
    const uint32_t datos = uint32_t(x.size() * 3);
    f.write("RIFF", 4); w32(4 + 24 + 8 + datos + (datos & 1)); f.write("WAVE", 4);
    f.write("fmt ", 4); w32(16); w16(1); w16(1); w32(rate); w32(rate * 3); w16(3); w16(24);
    f.write("data", 4); w32(datos);
    for (float v : x) { int32_t s = int32_t(std::lround(std::clamp(double(v), -1., 1. - 1. / 8388608) * 8388608.)); f.put(char(s)); f.put(char(s >> 8)); f.put(char(s >> 16)); }
    if (datos & 1) f.put(0);
    if (!f) throw std::runtime_error("No se pudo escribir " + ruta.u8string());
}

static std::string texto(const std::string& s) {
    std::string r = "\"";
    for (char c : s) { if (c == '"' || c == '\\') r += '\\'; if (c == '\n') { r += "\\n"; continue; } r += c; }
    return r + "\"";
}

static int exporta(const std::string& escena, const std::string& salida) {
    pli::Escena e = pli::leeEscena(escena);
    pli::adm::Archivo a = pli::adm::desdeEscena(e);
    pli::adm::Metadatos m = pli::adm::escribeMetadatos(a, e.nombre);
    pli::adm::escribeArchivo(salida, a, m);
    std::printf("ADM escrito: %s\n   %zu pistas a %u Hz, %.1f s, XML de %.0f kB\n", salida.c_str(), a.pistas.size(), a.rate,
                a.pistas.empty() ? 0. : double(a.pistas[0].audio.size()) / a.rate, m.xml.size() / 1024.);
    for (auto& p : a.pistas)
        std::printf("   %-22s %-9s %zu puntos de trayectoria\n", p.nombre.c_str(), p.tipo == pli::adm::Tipo::Objeto ? "objeto" : p.tipo == pli::adm::Tipo::Lfe ? "LFE" : "altavoz", p.azimut.p.size());
    return 0;
}

static int importa(const std::string& entrada, const std::string& carpeta, const std::string& ficha) {
    pli::adm::Archivo a = pli::adm::leeArchivo(entrada);
    fs::path dir = fs::u8path(carpeta), pistas = dir / "pistas";
    fs::create_directories(pistas);
    std::string nombre = fs::u8path(entrada).stem().u8string();
    pli::Escena e = pli::adm::aEscena(a, nombre.find("ADM") == std::string::npos ? nombre + " - ADM" : nombre, ficha);

    std::ostringstream j; j << std::fixed;
    std::string fichaRel;  // relativa a la carpeta de la escena: el proyecto se puede mover de PC
    if (!ficha.empty()) { std::error_code ec; fs::path r = fs::relative(fs::absolute(fs::u8path(ficha)), fs::absolute(dir), ec); fichaRel = (ec || r.empty() ? fs::absolute(fs::u8path(ficha)) : r).generic_u8string(); }
    j << "{\n  \"nombre\": " << texto(e.nombre) << ",\n  \"ficha\": " << (ficha.empty() ? std::string("null") : texto(fichaRel)) << ",\n  \"objetos\": [\n";
    for (size_t k = 0; k < e.objetos.size(); ++k) {
        auto& o = e.objetos[k];
        std::string archivo;
        for (char c : o.nombre) archivo += (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_') ? c : '_';
        char pre[8]; std::snprintf(pre, sizeof pre, "%02zu-", k + 1);
        archivo = "pistas/" + std::string(pre) + archivo + ".wav";
        escribeMono(dir / fs::u8path(archivo), o.audio.l, a.rate);
        o.archivo = (dir / fs::u8path(archivo)).u8string();
        j << "    { \"nombre\": " << texto(o.nombre) << ", \"archivo\": " << texto(archivo) << ", \"mono\": true";
        const auto& p = o.mov.puntos.p;
        if (o.lfe) j << ", \"lfe\": true";
        else if (p.size() == 1) j << ", \"azimut\": " << std::setprecision(2) << p[0].second;
        else {
            j << ", \"movimiento\": { \"tipo\": \"puntos\", \"puntos\": [";
            for (size_t i = 0; i < p.size(); ++i) j << (i ? "," : "") << "[" << std::setprecision(3) << p[i].first << "," << std::setprecision(2) << p[i].second << "]";
            j << "] }";
        }
        j << " }" << (k + 1 < e.objetos.size() ? "," : "") << "\n";
    }
    j << "  ]\n}\n";
    std::ofstream(dir / "escena.json") << j.str();

    pli::Disposicion d = ficha.empty() ? pli::Disposicion::anilloClasico() : pli::Disposicion::desdeFicha(ficha);
    Multi out = pli::renderiza(e, d);
    double g = normalize(out); Stats ms = measure(out);
    fs::path wav = dir / fs::u8path(e.nombre + " - 7.1-escena.wav");
    writeWav71(wav.u8string(), out, a.rate);

    std::printf("ADM importado: %s\n   %zu pistas a %u Hz\n", entrada.c_str(), e.objetos.size(), a.rate);
    for (auto& o : e.objetos)
        std::printf("   %-22s %s\n", o.nombre.c_str(), o.lfe ? "LFE directo" : o.mov.puntos.p.size() == 1 ? "fijo" : (std::to_string(o.mov.puntos.p.size()) + " puntos de trayectoria").c_str());
    if (!a.avisos.empty()) std::printf("Avisos:\n%s", a.avisos.c_str());
    std::printf("Escena editable: %s\n7.1: %s (ganancia global %.2f dB, no finitos %zu)\n", (dir / "escena.json").u8string().c_str(), wav.u8string().c_str(), db(g), ms.nonFinite);
    for (int c = 0; c < NCH; ++c) std::printf("   %-3s pico %7.2f dBFS  RMS %7.2f dBFS\n", Names[c], db(ms.peak[c]), db(ms.rms[c]));
    return ms.nonFinite ? 1 : 0;
}

int main(int argc, char** argv) {
    try {
        std::string orden = argc > 1 ? argv[1] : "";
        if (orden == "exporta" && argc == 4) return exporta(argv[2], argv[3]);
        if (orden == "importa" && (argc == 4 || (argc == 6 && std::string(argv[4]) == "--domo"))) return importa(argv[2], argv[3], argc == 6 ? argv[5] : "");
        std::printf("Uso:\n  adm71 exporta escena.json salida-adm.wav\n  adm71 importa entrada-adm.wav carpeta [--domo pruebas-domo\\domo.json]\n");
        return 2;
    } catch (const std::exception& e) { std::printf("FALLA: %s\n", e.what()); return 1; }
}
