// upmix71: analiza un WAV estéreo y genera versiones 7.1 (WAV EXTENSIBLE 0x63F, 24 bit).
//   upmix71.exe entrada.wav carpeta_salida
// Salidas:
//   *-7.1-conservador.wav  SPEC §3 modo 2: frente L/R, centro extraído regulable, ambiente
//                          lateral/trasero desde la diferencia L-R con decorrelación. LFE en silencio
//                          (el manejo de graves lo hace el Integra; evita doble procesamiento).
//   *-7.1-orbita.wav       Modo de juego para el domo: lecho conservador a -10 dB + la suma (L+R)
//                          girando alrededor de la sala por paneo de potencia constante entre pares
//                          de bocinas; LFE con graves < 80 Hz. La velocidad de giro cambia por tramos.
//   analisis.txt           Formato, picos, RMS, correlación L/R y comprobaciones de cada salida.
// Ganancia: una sola escala global por archivo, sólo hacia abajo, para pico <= -1 dBFS.
#include "audio71.hpp"


struct Delay {
    std::vector<float> buf; size_t pos = 0;
    explicit Delay(size_t n = 1) : buf(std::max<size_t>(n, 1)) {}
    float operator()(float x) { float y = buf[pos]; buf[pos] = x; pos = (pos + 1) % buf.size(); return y; }
};

struct Allpass {  // Schroeder: decorrela sin colorear el espectro de magnitud
    Delay d; float g;
    Allpass(size_t n, float gain) : d(n), g(gain) {}
    float operator()(float x) { float z = d.buf[d.pos]; float v = x + g * z; d(v); return z - g * v; }
};

struct Decorrelator {  // retardo de llegada + tres pasa-todo con longitudes primas distintas
    Delay pre; std::vector<Allpass> ap;
    Decorrelator(double ms, std::array<unsigned, 3> lens, unsigned rate) : pre(size_t(ms * rate / 1000)) {
        for (auto n : lens) ap.emplace_back(n * rate / 48000, .5f);
    }
    float operator()(float x) { float y = pre(x); for (auto& a : ap) y = a(y); return y; }
};


Multi conservative(const Stereo& s, float bedGain = 1.f) {
    const unsigned R = s.rate;
    Multi out(s.l.size());
    auto hpC = Biquad::make(true, 100, R), hpS = Biquad::make(true, 120, R), lpS = Biquad::make(false, 7000, R);
    Decorrelator dSL(10, { 347, 113, 37 }, R), dSR(13, { 389, 131, 41 }, R), dBL(20, { 433, 149, 43 }, R), dBR(24, { 467, 163, 53 }, R);
    const float kCenter = .45f, kFrontCut = .35f, kSide = .71f /* -3 dB */, kBack = .5f /* -6 dB */;
    for (size_t i = 0; i < s.l.size(); ++i) {
        float L = s.l[i], Rr = s.r[i], M = .5f * (L + Rr), S = lpS(hpS(.5f * (L - Rr)));
        auto& o = out[i];
        o[FL] = L - kFrontCut * M; o[FR] = Rr - kFrontCut * M;
        o[FC] = hpC(kCenter * M);
        o[LFE] = 0;
        o[SL] = kSide * dSL(S); o[SR] = -kSide * dSR(S);
        o[BL] = kBack * dBL(S); o[BR] = -kBack * dBR(S);
        for (auto& v : o) v *= bedGain;
    }
    return out;
}

Multi orbit(const Stereo& s, std::string& plan) {
    const unsigned R = s.rate;
    Multi out = conservative(s, .32f);  // lecho a -10 dB: que el giro se distinga
    // Anillo por ángulo (grados, sentido horario desde el centro): C, FR, SR, BR, BL, SL, FL.
    const int ring[7] = { FC, FR, SR, BR, BL, SL, FL };
    const double ang[7] = { 0, 30, 90, 150, 210, 270, 330 };
    auto lpL = Biquad::make(false, 80, R), lpL2 = Biquad::make(false, 80, R), hpO = Biquad::make(true, 120, R);
    // Tramos: (segundo de inicio, vueltas por minuto; negativo = antihorario)
    struct Seg { double t, rpm; };
    std::vector<Seg> segs = { { 0, 0 }, { 8, 2 }, { 40, 4 }, { 80, -3 }, { 120, 6 }, { 150, 1.5 } };
    double total = double(s.l.size()) / R, theta = 0;
    std::ostringstream p;
    for (size_t k = 0; k < segs.size(); ++k)
        if (segs[k].t < total) p << "    " << segs[k].t << " s: " << (segs[k].rpm == 0 ? std::string("quieto al frente") : std::to_string(segs[k].rpm).substr(0, 4) + " vueltas/min" + (segs[k].rpm < 0 ? " (antihorario)" : "")) << "\n";
    plan = p.str();
    double rpm = 0;
    for (size_t i = 0; i < s.l.size(); ++i) {
        double t = double(i) / R, want = 0;
        for (auto& sg : segs) if (t >= sg.t) want = sg.rpm;
        rpm += (want - rpm) * (1 - std::exp(-1. / (2.0 * R)));  // transición suave (~2 s)
        theta = std::fmod(theta + rpm * 360. / 60. / R + 360., 360.);
        float M = .5f * (s.l[i] + s.r[i]), obj = hpO(M);
        // Par de bocinas adyacentes que rodean theta; ley de potencia constante.
        int k = 6; for (int j = 0; j < 7; ++j) { double a = ang[j], b = ang[(j + 1) % 7] + (j == 6 ? 360 : 0); double th = theta < a ? theta + 360 : theta; if (th >= a && th < b) { k = j; break; } }
        double a = ang[k], b = ang[(k + 1) % 7] + (k == 6 ? 360 : 0), th = theta < a ? theta + 360 : theta;
        double x = (th - a) / (b - a) * Pi / 2;
        out[i][ring[k]] += obj * float(std::cos(x));
        out[i][ring[(k + 1) % 7]] += obj * float(std::sin(x));
        out[i][LFE] = .5f * lpL2(lpL(M));
    }
    return out;
}


int main(int argc, char** argv) {
    if (argc < 3) { std::printf("Uso: upmix71 entrada.wav carpeta_salida\n"); return 2; }
    try {
        std::string in = argv[1], dir = argv[2];
        std::string base = in.substr(in.find_last_of("/\\") + 1); base = base.substr(0, base.rfind('.'));
        Stereo s = readWav(in);
        std::ostringstream rep;
        rep << "Análisis — " << base << "\n\n";
        double pl = 0, pr = 0, el = 0, er = 0, lr = 0, em = 0, es = 0, dcl = 0, dcr = 0; size_t clipL = 0, clipR = 0;
        for (size_t i = 0; i < s.l.size(); ++i) {
            double L = s.l[i], Rr = s.r[i];
            pl = std::max(pl, std::fabs(L)); pr = std::max(pr, std::fabs(Rr)); el += L * L; er += Rr * Rr; lr += L * Rr;
            em += (L + Rr) * (L + Rr) / 4; es += (L - Rr) * (L - Rr) / 4; dcl += L; dcr += Rr;
            clipL += std::fabs(L) >= .9999; clipR += std::fabs(Rr) >= .9999;
        }
        size_t n = s.l.size();
        char line[256];
        std::snprintf(line, sizeof line, "Origen: PCM estéreo, %u Hz, %.1f s (%zu muestras)\n", s.rate, double(n) / s.rate, n); rep << line;
        std::snprintf(line, sizeof line, "Pico L/R: %.2f / %.2f dBFS | RMS L/R: %.2f / %.2f dBFS\n", db(pl), db(pr), db(std::sqrt(el / n)), db(std::sqrt(er / n))); rep << line;
        std::snprintf(line, sizeof line, "Muestras en tope digital L/R: %zu / %zu | DC L/R: %.5f / %.5f\n", clipL, clipR, dcl / n, dcr / n); rep << line;
        std::snprintf(line, sizeof line, "Correlación L/R: %.3f (1 = mono, 0 = sin relación) | energía lateral/central (S/M): %.1f dB\n\n", lr / std::sqrt(el * er), 10 * std::log10(es / em)); rep << line;

        std::string plan;
        struct Out { const char* tag; Multi m; } outs[] = { { "conservador", conservative(s) }, { "orbita", orbit(s, plan) } };
        bool ok = true;
        for (auto& o : outs) {
            double g = normalize(o.m);
            Stats st = measure(o.m);
            std::string path = dir + "/" + base + " - 7.1-" + o.tag + ".wav";
            writeWav71(path, o.m, s.rate);
            rep << "== " << o.tag << " → " << path.substr(path.find_last_of("/\\") + 1) << "\n";
            std::snprintf(line, sizeof line, "   Ganancia global aplicada: %.2f dB | no finitos: %zu\n", db(g), st.nonFinite); rep << line;
            for (int c = 0; c < NCH; ++c) { std::snprintf(line, sizeof line, "   %-3s pico %7.2f dBFS  RMS %7.2f dBFS\n", Names[c], db(st.peak[c]), db(st.rms[c])); rep << line; }
            ok = ok && st.nonFinite == 0;
            if (!std::strcmp(o.tag, "orbita")) rep << "   Recorrido de la órbita:\n" << plan;
            rep << "\n";
        }
        std::ofstream(dir + "/analisis.txt") << rep.str();
        std::printf("%s", rep.str().c_str());
        return ok ? 0 : 1;
    } catch (const std::exception& e) { std::printf("FALLA: %s\n", e.what()); return 1; }
}
