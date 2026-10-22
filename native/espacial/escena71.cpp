// escena71: renderiza una escena de PLI Espacial (JSON) a un WAV 7.1 (24 bit, máscara 0x63F).
//   escena71.exe escena.json [salida.wav]
// Sin salida: escribe «<nombre> - 7.1-escena.wav» y «<nombre>-escena.txt» junto a la escena.
// Formato de la escena: native/espacial/ESCENAS.md.
#include "escena.hpp"
#include <chrono>

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) { std::printf("Uso: escena71 escena.json [salida.wav]\n"); return 2; }
    try {
        namespace fs = std::filesystem;
        auto t0 = std::chrono::steady_clock::now();
        std::string ruta = argv[1];
        pli::Escena e = pli::leeEscena(ruta);
        pli::Disposicion d = e.ficha.empty() ? pli::Disposicion::anilloClasico() : pli::Disposicion::desdeFicha(e.ficha);
        fs::path carpeta = fs::u8path(ruta).parent_path();
        std::string salida = argc == 3 ? argv[2] : (carpeta / fs::u8path(e.nombre + " - 7.1-escena.wav")).u8string();

        std::ostringstream rep; char line[512];
        rep << "Escena — " << e.nombre << "\n";
        rep << "Bocinas (" << (e.ficha.empty() ? "anillo clásico" : e.ficha) << "):";
        for (auto& b : d.bocinas()) { std::snprintf(line, sizeof line, " %s %.0f°", b.id.c_str(), b.azimut > 180 ? b.azimut - 360 : b.azimut); rep << line; }
        rep << "\n\nObjetos:\n";
        const char* tipo[] = { "puntos", "giro", "vaivén" };
        for (auto& o : e.objetos) {
            std::snprintf(line, sizeof line, "   %-12s %-7s %s  nivel %+.1f dB  ancho %.0f°  desde %.1f s  dura %.1f s\n", o.nombre.c_str(),
                          tipo[o.mov.tipo], o.mono ? "mono   " : "estéreo", db(o.ganancia), o.ancho, o.inicio, double(o.audio.l.size()) / std::max(1u, o.audio.rate));
            rep << line;
        }
        if (e.lfe.activo) {
            rep << "LFE: " << e.lfe.ganancia << " × pasa-bajos " << e.lfe.corte << " Hz (24 dB/oct) de:";
            for (auto& f : e.lfe.fuentes) rep << " " << f;
            rep << "\n";
        } else rep << "LFE: apagado (la escena no lo pide)\n";

        Multi out = pli::renderiza(e, d);
        double g = normalize(out);
        Stats ms = measure(out);
        writeWav71(salida, out, pli::frecuenciaComun(e));
        double seg = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        std::snprintf(line, sizeof line, "\n== %s\n   %.1f s de audio a %u Hz, renderizado en %.1f s | ganancia global %.2f dB | no finitos: %zu\n",
                      fs::u8path(salida).filename().u8string().c_str(), double(out.size()) / pli::frecuenciaComun(e), pli::frecuenciaComun(e), seg, db(g), ms.nonFinite);
        rep << line;
        for (int c = 0; c < NCH; ++c) { std::snprintf(line, sizeof line, "   %-3s pico %7.2f dBFS  RMS %7.2f dBFS\n", Names[c], db(ms.peak[c]), db(ms.rms[c])); rep << line; }
        if (argc == 2) std::ofstream(carpeta / fs::u8path(e.nombre + "-escena.txt")) << rep.str();
        std::printf("%s", rep.str().c_str());
        return ms.nonFinite == 0 ? 0 : 1;
    } catch (const std::exception& ex) { std::printf("FALLA: %s\n", ex.what()); return 1; }
}
