// capas71: separa un WAV estéreo en seis capas y las coreografía por separado en 7.1.
//   capas71.exe entrada.wav carpeta_salida
// Separación sin IA (STFT 4096/1024, Hann):
//   - Armónico/percusivo por filtrado de mediana (HPSS, Fitzgerald 2010): lo sostenido en el
//     tiempo es armónico; lo ancho en frecuencia y breve es percusivo. Máscaras suaves de Wiener.
//   - Bandas con cruces suaves: 150 Hz (bombo/bajo) y 4 kHz (brillos).
//   - En medios armónicos, lo idéntico en L y R (centro: voz, solista) se separa del resto.
// Las seis máscaras suman 1 en cada celda: las capas sumadas reconstruyen el original
// (la reconstrucción se mide y se reporta). No son instrumentos limpios: son capas.
// Con --domo ficha.json el paneo usa PLI Espacial (native/espacial) con los ángulos reales del domo.
// Salidas: capas/<capa>.wav (estéreo, para revisar o mezclar en otro programa),
//          *-7.1-instrumentos.wav y capas.txt con la medición y la coreografía.
#include "audio71.hpp"
#include "../espacial/pli_espacial.hpp"
#include <complex>
#include <filesystem>
#include <functional>

using cfloat = std::complex<float>;
constexpr unsigned N = 4096, Hop = 1024, Bins = N / 2 + 1, Med = 17;

void fft(std::vector<cfloat>& a, bool inverse) {
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        double ang = 2 * Pi / double(len) * (inverse ? 1 : -1);
        cfloat wl(float(std::cos(ang)), float(std::sin(ang)));
        for (size_t i = 0; i < n; i += len) {
            cfloat w(1);
            for (size_t j = 0; j < len / 2; ++j) {
                cfloat u = a[i + j], v = a[i + j + len / 2] * w;
                a[i + j] = u + v; a[i + j + len / 2] = u - v; w *= wl;
            }
        }
    }
    if (inverse) for (auto& x : a) x /= float(n);
}

enum Layer { Bombo, Percusion, Bajo, Voz, Colchon, Brillos, NLayers };
const char* LayerName[NLayers] = { "bombo", "percusion", "bajo", "voz", "colchon", "brillos" };

struct Stems { std::array<std::vector<float>, NLayers> l, r; };

Stems separate(const Stereo& s) {
    const size_t len = s.l.size(), frames = (len + N) / Hop + 1;
    std::vector<float> win(N);
    for (unsigned i = 0; i < N; ++i) win[i] = float(.5 - .5 * std::cos(2 * Pi * i / N));
    auto frameStart = [&](size_t f) { return int64_t(f) * Hop - int64_t(N); };
    auto analyze = [&](size_t f, std::vector<cfloat>& L, std::vector<cfloat>& R) {
        L.assign(N, 0); R.assign(N, 0);
        for (unsigned i = 0; i < N; ++i) {
            int64_t t = frameStart(f) + i;
            if (t >= 0 && size_t(t) < len) { L[i] = s.l[size_t(t)] * win[i]; R[i] = s.r[size_t(t)] * win[i]; }
        }
        fft(L, false); fft(R, false);
    };
    // Pasada 1: magnitud conjunta |L|+|R| (las máscaras son comunes a L y R: cada capa conserva su imagen estéreo).
    std::vector<float> mag(frames * Bins);
    std::vector<cfloat> L, R;
    for (size_t f = 0; f < frames; ++f) {
        analyze(f, L, R);
        for (unsigned k = 0; k < Bins; ++k) mag[f * Bins + k] = std::abs(L[k]) + std::abs(R[k]);
    }
    auto median = [](std::array<float, Med>& v) { std::nth_element(v.begin(), v.begin() + Med / 2, v.end()); return v[Med / 2]; };
    auto lowWeight = [&](unsigned k, double fc) { double fr = double(k) * s.rate / N; return float(1 / (1 + std::pow(fr / fc, 8))); };
    // Pasada 2: máscaras, síntesis por capa con suma-traslape (Hann² con salto N/4 suma 1,5).
    Stems st;
    for (int c = 0; c < NLayers; ++c) { st.l[c].assign(len, 0); st.r[c].assign(len, 0); }
    std::array<std::vector<cfloat>, NLayers> YL, YR;
    for (size_t f = 0; f < frames; ++f) {
        analyze(f, L, R);
        for (int c = 0; c < NLayers; ++c) { YL[c].assign(N, 0); YR[c].assign(N, 0); }
        for (unsigned k = 0; k < Bins; ++k) {
            std::array<float, Med> tv, fv;
            for (unsigned j = 0; j < Med; ++j) {
                size_t ff = size_t(std::clamp<int64_t>(int64_t(f) + j - Med / 2, 0, int64_t(frames) - 1));
                unsigned kk = unsigned(std::clamp<int>(int(k) + int(j) - int(Med / 2), 0, int(Bins) - 1));
                tv[j] = mag[ff * Bins + k]; fv[j] = mag[f * Bins + kk];
            }
            float h = median(tv), p = median(fv);
            float mh = h * h / (h * h + p * p + 1e-12f), mp = 1 - mh;
            float lo150 = lowWeight(k, 150), lo4k = lowWeight(k, 4000);
            double el = std::norm(L[k]), er = std::norm(R[k]);
            float psi = float(std::clamp(2 * double((L[k] * std::conj(R[k])).real()) / (el + er + 1e-20), 0., 1.));
            float center = psi * psi;  // sólo lo muy parecido en L y R cuenta como centro
            float mid = mh * (1 - lo150) * lo4k;
            std::array<float, NLayers> m = { mp * lo150, mp * (1 - lo150), mh * lo150, mid * center, mid * (1 - center), mh * (1 - lo150) * (1 - lo4k) };
            for (int c = 0; c < NLayers; ++c) {
                YL[c][k] = L[k] * m[c]; YR[c][k] = R[k] * m[c];
                if (k > 0 && k < N / 2) { YL[c][N - k] = std::conj(YL[c][k]); YR[c][N - k] = std::conj(YR[c][k]); }
            }
        }
        for (int c = 0; c < NLayers; ++c) {
            fft(YL[c], true); fft(YR[c], true);
            for (unsigned i = 0; i < N; ++i) {
                int64_t t = frameStart(f) + i;
                if (t < 0 || size_t(t) >= len) continue;
                st.l[c][size_t(t)] += YL[c][i].real() * win[i] / 1.5f;
                st.r[c][size_t(t)] += YR[c][i].real() * win[i] / 1.5f;
            }
        }
    }
    return st;
}

// Cuánto se mueve todo a lo largo de la canción: quieto en la entrada, más rápido en el tramo
// 120–150 s, calmado al cierre. Multiplica las velocidades de giro de cada capa.
double motion(double t, double total) {
    if (t < 8) return 0;
    if (t < 16) return (t - 8) / 8;
    if (t >= 120 && t < 150) return 1.6;
    if (t > total - 20) return std::max(.4, (total - t) / 20);
    return 1;
}

// Ruta de «destino» vista desde la carpeta del archivo «desde», con «/»: así la escena funciona si
// se copia el proyecto a otra PC o carpeta. Si no hay ruta relativa (otra unidad), queda absoluta.
std::string rutaRelativa(const std::string& destino, const std::string& desde) {
    namespace fs = std::filesystem;
    fs::path d = fs::absolute(fs::u8path(destino)), base = fs::absolute(fs::u8path(desde)).parent_path();
    std::error_code ec; fs::path r = fs::relative(d, base, ec);
    return (ec || r.empty() ? d : r).generic_u8string();
}

// La misma coreografía de abajo, como escena de PLI Espacial (native/espacial/ESCENAS.md), para
// editarla y renderizarla con escena71 sin volver a separar. motion() pasa a ser la curva «velocidad».
void escribeEscena(const std::string& ruta, const std::string& base, const std::string& ficha, double total) {
    auto texto = [](const std::string& s) {
        std::string r = "\"";
        for (char c : s) { if (c == '"' || c == '\\') r += '\\'; r += c; }
        return r + "\"";
    };
    char v[512];
    if (total >= 170) std::snprintf(v, sizeof v, "[[0,0],[8,0],[16,1],[120,1],[120,1.6],[150,1.6],[150,1],[%.3f,1],[%.3f,0.4],[%.3f,0.4]]", total - 20, total - 8, total);
    else std::snprintf(v, sizeof v, "[[0,0],[8,0],[16,1],[%.3f,1]]", std::max(16., total));
    std::ofstream f(std::filesystem::u8path(ruta));
    f << "{\n  \"nombre\": " << texto(base + " - capas") << ",\n"
      << "  \"ficha\": " << (ficha.empty() ? std::string("null") : texto(rutaRelativa(ficha, ruta))) << ",\n"
      << "  \"velocidad\": " << v << ",\n"
      << "  \"objetos\": [\n"
      << "    { \"nombre\": \"bajo\",      \"archivo\": \"capas/bajo.wav\",      \"azimut\": 0, \"ancho\": 30 },\n"
      << "    { \"nombre\": \"bombo\",     \"archivo\": \"capas/bombo.wav\",     \"azimut\": 0, \"ancho\": 30 },\n"
      << "    { \"nombre\": \"voz\",       \"archivo\": \"capas/voz.wav\",       \"mono\": true, \"movimiento\": { \"tipo\": \"vaiven\", \"centro\": 0, \"amplitud\": 25, \"periodo_s\": 24 } },\n"
      << "    { \"nombre\": \"colchon\",   \"archivo\": \"capas/colchon.wav\",   \"ancho\": 90, \"movimiento\": { \"tipo\": \"giro\", \"inicio\": 0, \"rpm\": 1.5 } },\n"
      << "    { \"nombre\": \"brillos\",   \"archivo\": \"capas/brillos.wav\",   \"ancho\": 60, \"movimiento\": { \"tipo\": \"giro\", \"inicio\": 180, \"rpm\": -3 } },\n"
      << "    { \"nombre\": \"percusion\", \"archivo\": \"capas/percusion.wav\", \"ancho\": 45, \"movimiento\": { \"tipo\": \"giro\", \"inicio\": 0, \"rpm\": 5 } }\n"
      << "  ],\n"
      << "  \"lfe\": { \"fuentes\": [\"bajo\", \"bombo\"], \"corte_hz\": 80, \"ganancia\": 0.5 }\n"
      << "}\n";
    if (!f) throw std::runtime_error("No se pudo escribir " + ruta);
}

int main(int argc, char** argv) {
    if (argc != 3 && !(argc == 5 && std::string(argv[3]) == "--domo")) {
        std::printf("Uso: capas71 entrada.wav carpeta_salida [--domo pruebas-domo\\domo.json]\n"
                    "  Sin --domo: anillo fijo de siempre. Con --domo: PLI Espacial (VBAP 2D) con los ángulos de la ficha.\n");
        return 2;
    }
    try {
        std::string in = argv[1], dir = argv[2];
        std::unique_ptr<pli::Disposicion> domo;
        if (argc == 5) domo = std::make_unique<pli::Disposicion>(pli::Disposicion::desdeFicha(argv[4]));
        auto pan = [&](std::array<float, NCH>& o, float x, double deg) { if (domo) domo->panea(o, x, deg); else panRing(o, x, deg); };
        std::string base = in.substr(in.find_last_of("/\\") + 1); base = base.substr(0, base.rfind('.'));
        Stereo s = readWav(in);
        const unsigned rate = s.rate; const size_t len = s.l.size(); const double total = double(len) / rate;
        Stems st = separate(s);

        std::ostringstream rep; char line[256];
        rep << "Capas — " << base << "\n";
        if (domo) {
            rep << "Paneo: PLI Espacial (VBAP 2D) con la ficha " << argv[4] << ":";
            for (auto& b : domo->bocinas()) { std::snprintf(line, sizeof line, " %s %.0f°", b.id.c_str(), b.azimut); rep << line; }
            rep << "\n\n";
        } else rep << "Paneo: anillo fijo (C 0, FR 30, SR 90, BR 150, BL 210, SL 270, FL 330)\n\n";
        // Reconstrucción: suma de capas contra el original, lejos de los bordes.
        double eo = 0, ee = 0, etot = 0; std::array<double, NLayers> el{};
        for (size_t i = 0; i < len; ++i) {
            double sl = 0, sr = 0;
            for (int c = 0; c < NLayers; ++c) { sl += st.l[c][i]; sr += st.r[c][i]; el[c] += double(st.l[c][i]) * st.l[c][i] + double(st.r[c][i]) * st.r[c][i]; }
            if (i > N && i + N < len) { eo += double(s.l[i]) * s.l[i] + double(s.r[i]) * s.r[i]; ee += (sl - s.l[i]) * (sl - s.l[i]) + (sr - s.r[i]) * (sr - s.r[i]); }
        }
        for (auto e : el) etot += e;
        std::snprintf(line, sizeof line, "Reconstrucción (suma de capas vs original): error %.1f dB bajo la señal\n\n", 10 * std::log10(eo / std::max(ee, 1e-30))); rep << line;
        rep << "Energía por capa:\n";
        std::string capas = dir + "/capas";
        std::filesystem::create_directories(capas);
        for (int c = 0; c < NLayers; ++c) {
            std::snprintf(line, sizeof line, "   %-10s %5.1f %%\n", LayerName[c], 100 * el[c] / etot); rep << line;
            writeStereo(capas + "/" + LayerName[c] + ".wav", st.l[c], st.r[c], rate);
        }

        // Coreografía. Ángulo 0 = centro, positivo = horario. Ancho = mitad de la apertura L–R.
        struct Move { Layer layer; double start, rpm, width; const char* text; };
        const Move moves[] = {
            { Colchon, 0, 1.5, 90, "colchón: abierto a los lados (L en SL, R en SR), gira horario 1,5 v/min" },
            { Brillos, 180, -3, 60, "brillos: nacen atrás, giran antihorario 3 v/min" },
            { Percusion, 0, 5, 45, "percusión aguda: nace al frente abierta, gira horario 5 v/min" },
        };
        rep << "\nCoreografía (las velocidades se multiplican por el tramo: 0 hasta 8 s, sube a 1 en 16 s, 1,6 entre 120 y 150 s, baja al final):\n"
            << "   bajo: fijo en FL/FR + LFE (graves < 80 Hz)\n   bombo: fijo al frente (FL+FR) + LFE\n"
            << "   voz/solista: al frente, se mece entre FL, C y FR (±25°, ciclo de 24 s)\n";
        for (auto& m : moves) rep << "   " << m.text << "\n";

        Multi out(len);
        std::array<double, 3> theta{}; for (int j = 0; j < 3; ++j) theta[j] = moves[j].start;
        auto lp1 = Biquad::make(false, 80, rate), lp2 = Biquad::make(false, 80, rate);
        std::ostringstream trace; trace << "\nPosición de las capas móviles (grados) cada 7 s:\n";
        for (size_t i = 0; i < len; ++i) {
            double t = double(i) / rate, a = motion(t, total);
            auto& o = out[i];
            o[FL] += st.l[Bajo][i] + st.l[Bombo][i];
            o[FR] += st.r[Bajo][i] + st.r[Bombo][i];
            o[LFE] = .5f * lp2(lp1(.5f * (st.l[Bajo][i] + st.r[Bajo][i] + st.l[Bombo][i] + st.r[Bombo][i])));
            pan(o, .5f * (st.l[Voz][i] + st.r[Voz][i]), 25 * std::sin(2 * Pi * t / 24) * std::min(1., a));
            for (int j = 0; j < 3; ++j) {
                theta[j] += moves[j].rpm * a * 360. / 60. / rate;
                Layer c = moves[j].layer;
                pan(o, st.l[c][i], theta[j] - moves[j].width);
                pan(o, st.r[c][i], theta[j] + moves[j].width);
            }
            if (i % (7 * rate) == 0) {
                std::snprintf(line, sizeof line, "   %4.0f s: colchón %4.0f  brillos %4.0f  percusión %4.0f\n", t,
                              std::fmod(std::fmod(theta[0], 360) + 360, 360), std::fmod(std::fmod(theta[1], 360) + 360, 360), std::fmod(std::fmod(theta[2], 360) + 360, 360));
                trace << line;
            }
        }
        rep << trace.str();
        escribeEscena(dir + "/escena.json", base, argc == 5 ? argv[4] : "", total);
        rep << "\nEscena editable de PLI Espacial: escena.json (renderizar con escena71.exe)\n";
        double g = normalize(out);
        Stats ms = measure(out);
        std::string path = dir + "/" + base + " - 7.1-instrumentos.wav";
        writeWav71(path, out, rate);
        std::snprintf(line, sizeof line, "\n== %s\n   Ganancia global: %.2f dB | no finitos: %zu\n", path.substr(path.find_last_of("/\\") + 1).c_str(), db(g), ms.nonFinite); rep << line;
        for (int c = 0; c < NCH; ++c) { std::snprintf(line, sizeof line, "   %-3s pico %7.2f dBFS  RMS %7.2f dBFS\n", Names[c], db(ms.peak[c]), db(ms.rms[c])); rep << line; }
        std::ofstream(dir + "/capas.txt") << rep.str();
        std::printf("%s", rep.str().c_str());
        return ms.nonFinite == 0 ? 0 : 1;
    } catch (const std::exception& e) { std::printf("FALLA: %s\n", e.what()); return 1; }
}
