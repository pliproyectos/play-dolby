// Pruebas de PLI Espacial (fase 1): paneo VBAP 2D desde domo.json.
//   espacial_tests.exe [ruta a domo.json]
#include "../espacial/adm.hpp"
#include "../src/bridge_core.hpp"
#include "../src/fuente_archivo.hpp"
#include <cstdio>

using namespace pli;
static int fallas = 0, pruebas = 0;
#define CHECK(cond, ...) do { ++pruebas; if (!(cond)) { ++fallas; std::printf("FALLA %s:%d  ", __FILE__, __LINE__); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static double potencia(const std::array<float, NumCanales>& g) { double s = 0; for (float v : g) s += double(v) * v; return s; }

static void pruebasDeDisposicion(const Disposicion& d, const char* nombre) {
    // 1. Un objeto justo en una bocina sale solo por esa bocina.
    for (const Bocina& b : d.bocinas()) {
        auto g = d.ganancias(b.azimut);
        for (int c = 0; c < NumCanales; ++c)
            CHECK(std::fabs(g[c] - (c == b.canal ? 1.f : 0.f)) < 1e-5f, "%s: objeto en %s (%.1f°) -> canal %s = %.6f", nombre, b.id.c_str(), b.azimut, IdCanal[c], g[c]);
    }
    // 2. Potencia constante, sin negativos, LFE en cero y sin saltos al girar (paso de 0,1°).
    std::array<float, NumCanales> prev = d.ganancias(0);
    double peorPot = 0, peorSalto = 0;
    for (int i = 1; i <= 3600; ++i) {
        double th = i * 0.1;
        auto g = d.ganancias(th);
        peorPot = std::max(peorPot, std::fabs(potencia(g) - 1));
        for (int c = 0; c < NumCanales; ++c) { CHECK(g[c] >= 0, "%s: ganancia negativa en %.1f°", nombre, th); peorSalto = std::max(peorSalto, double(std::fabs(g[c] - prev[c]))); }
        CHECK(g[3] == 0.f, "%s: el LFE recibió señal en %.1f°", nombre, th);
        prev = g;
    }
    CHECK(peorPot < 1e-5, "%s: la potencia se desvía %.2e de 1", nombre, peorPot);
    CHECK(peorSalto < 0.02, "%s: salto de ganancia de %.4f en 0,1° (debe ser continuo)", nombre, peorSalto);
    // 3. Un ángulo y su equivalente +360 / -360 dan lo mismo.
    for (double th : { -170.0, -30.0, 12.5, 200.0 }) {
        auto a = d.ganancias(th), b = d.ganancias(th + 360), c = d.ganancias(th - 720);
        for (int k = 0; k < NumCanales; ++k) CHECK(std::fabs(a[k] - b[k]) < 1e-5f && std::fabs(a[k] - c[k]) < 1e-5f, "%s: %.1f° no equivale a su vuelta completa", nombre, th);
    }
}

static bool rechaza(const std::string& json) {
    try { interpretaEscena(LectorJson(json).leer(), "."); } catch (const std::exception&) { return true; }
    return false;
}

static void pruebasDeEscena(const Disposicion& domo) {
    // Curvas: interpolación, extremos fijos y escalón.
    Curva c; c.p = { { 0, 0 }, { 10, 100 }, { 10, 50 }, { 20, 50 } };
    CHECK(c.en(-5) == 0 && c.en(5) == 50 && c.en(10) == 50 && c.en(15) == 50 && c.en(99) == 50, "curva: interpolación o escalón");
    Curva rampa; rampa.p = { { 0, 0 }, { 4, 8 } };
    CHECK(std::fabs(rampa.en(1) - 2) < 1e-12 && Curva{}.en(3) == 1, "curva: rampa o curva vacía (vale 1)");

    // Giro: 60 rpm a 1000 Hz = una vuelta por segundo.
    Movimiento g; g.tipo = Movimiento::Giro; g.inicio = 10; g.rpm = 60;
    Recorrido rg(g, 1000); double az = 0;
    for (int i = 0; i < 1000; ++i) az = rg.siguiente();
    CHECK(std::fabs(az - 370) < 1e-6, "giro: tras 1 s a 60 rpm debe ir en 370°, va en %.6f", az);
    // Giro con velocidad 0 la primera mitad: media vuelta.
    Movimiento g2 = g; g2.velocidad.p = { { 0, 0 }, { .5, 0 }, { .5, 1 } };
    Recorrido rg2(g2, 1000); for (int i = 0; i < 1000; ++i) az = rg2.siguiente();
    CHECK(std::fabs(az - 190) < 1e-6, "giro con velocidad: debe ir en 190°, va en %.6f", az);
    // Vaivén: en un cuarto de periodo llega a centro + amplitud; la velocidad > 1 no la agranda.
    Movimiento v; v.tipo = Movimiento::Vaiven; v.centro = 5; v.amplitud = 20; v.periodo = 4; v.velocidad.p = { { 0, 1.6 } };
    Recorrido rv(v, 100); for (int i = 0; i <= 100; ++i) az = rv.siguiente();
    CHECK(std::fabs(az - 25) < 1e-9, "vaivén: a 1 s debe ir en 25°, va en %.6f", az);

    // Lectura: lo válido se acepta y lo inválido se rechaza con mensaje.
    const std::string base = R"({"objetos":[{"nombre":"a","archivo":"a.wav","azimut":30}]})";
    CHECK(!rechaza(base), "una escena mínima válida debe aceptarse");
    CHECK(rechaza(R"({"objetos":[]})"), "sin objetos");
    CHECK(rechaza(R"({"objetos":[{"nombre":"a","archivo":"a.wav"}]})"), "sin azimut ni movimiento");
    CHECK(rechaza(R"({"objetos":[{"nombre":"a","archivo":"a.wav","azimut":0},{"nombre":"a","archivo":"b.wav","azimut":0}]})"), "nombres repetidos");
    CHECK(rechaza(R"({"objetos":[{"nombre":"a","archivo":"a.wav","movimiento":{"tipo":"espiral"}}]})"), "tipo de movimiento desconocido");
    CHECK(rechaza(R"({"objetos":[{"nombre":"a","archivo":"a.wav","movimiento":{"tipo":"vaiven","periodo_s":0}}]})"), "vaivén con periodo 0");
    CHECK(rechaza(R"({"objetos":[{"nombre":"a","archivo":"a.wav","movimiento":{"tipo":"puntos","puntos":[[5,0],[1,9]]}}]})"), "puntos fuera de orden");
    CHECK(rechaza(R"({"objetos":[{"nombre":"a","archivo":"a.wav","azimut":0,"ancho":200}]})"), "ancho > 180");
    CHECK(rechaza(R"({"objetos":[{"nombre":"a","archivo":"a.wav","azimut":0}],"lfe":{"fuentes":["zz"]}})"), "lfe con fuente inexistente");
    CHECK(rechaza(R"({"objetos":[{"nombre":"a","archivo":"a.wav","azimut":0,"nivel_db":"alto"}]})"), "nivel no numérico");
    Escena e0 = interpretaEscena(LectorJson(base).leer(), ".");
    CHECK(!e0.lfe.activo && e0.ficha.empty(), "por defecto: LFE apagado y anillo clásico");

    // Render con audio sintético (1 kHz de muestreo, 1000 muestras de 0,5).
    auto objeto = [](const char* n, std::vector<float> l, std::vector<float> r) { Objeto o; o.nombre = n; o.audio.rate = 1000; o.audio.l = std::move(l); o.audio.r = std::move(r); return o; };
    std::vector<float> uno(1000, .5f), cero(1000, 0.f);
    {   // Mono fijo en FR: solo FR, y el LFE apagado.
        Escena e; Objeto o = objeto("a", uno, uno); o.mono = true; o.mov.puntos.p = { { 0, 30 } }; e.objetos.push_back(o);
        Multi m = renderiza(e, domo);
        bool ok = m.size() == 1000;
        for (auto& f : m) for (int k = 0; k < NumCanales; ++k) ok = ok && std::fabs(f[k] - (k == 1 ? .5f : 0.f)) < 1e-6f;
        CHECK(ok, "render: objeto mono en 30° debe salir solo por FR con 0,5");
    }
    {   // Estéreo con ancho 30 en el centro: L a FL, R a FR. Nivel −6,02 dB = mitad.
        Escena e; Objeto o = objeto("a", uno, cero); o.ancho = 30; o.ganancia = std::pow(10., -6.0206 / 20); o.mov.puntos.p = { { 0, 0 } }; e.objetos.push_back(o);
        Multi m = renderiza(e, domo);
        CHECK(std::fabs(m[10][0] - .25f) < 1e-4f && std::fabs(m[10][1]) < 1e-6f, "render: L de un estéreo ancho 30 debe ir a FL a la mitad (%.4f, FR %.4f)", m[10][0], m[10][1]);
    }
    {   // Inicio retrasado: la escena dura lo que el objeto más tardío y antes hay silencio.
        Escena e; Objeto o = objeto("a", uno, uno); o.mono = true; o.inicio = .5; o.mov.puntos.p = { { 0, 0 } }; e.objetos.push_back(o);
        Multi m = renderiza(e, domo);
        CHECK(m.size() == 1500 && m[499][2] == 0.f && std::fabs(m[500][2] - .5f) < 1e-6f, "render: inicio_s retrasa al objeto (%zu muestras)", m.size());
    }
    {   // LFE solo cuando se pide, y solo con las fuentes dadas.
        Escena e; Objeto a = objeto("a", uno, uno), b = objeto("b", uno, uno);
        a.mono = b.mono = true; a.mov.puntos.p = b.mov.puntos.p = { { 0, 0 } };
        e.objetos = { a, b };
        CHECK(renderiza(e, domo)[900][3] == 0.f, "render: sin «lfe» el sub queda en silencio");
        e.lfe.activo = true; e.lfe.fuentes = { "a" }; e.lfe.ganancia = 1; e.lfe.corte = 80;
        float sub = renderiza(e, domo)[999][3];
        CHECK(std::fabs(sub - .5f) < .01f, "render: lfe de una fuente de CD 0,5 debe asentarse en 0,5 (%.4f)", sub);
    }
    {   // Frecuencias distintas: se rechaza.
        Escena e; Objeto a = objeto("a", uno, uno), b = objeto("b", uno, uno); b.audio.rate = 2000;
        e.objetos = { a, b }; bool r = false;
        try { renderiza(e, domo); } catch (const std::exception&) { r = true; }
        CHECK(r, "render: debe rechazar WAV con frecuencias distintas");
    }
}

static void pruebasEnVivo(const Disposicion& domo) {
    const unsigned F = 64, C = 4;  // bloque ASIO de 64, consola con 4 entradas
    std::vector<float> in(F * C, 0.f);
    for (unsigned i = 0; i < F; ++i) { in[i * C + 1] = .5f; in[i * C + 2] = .25f; }  // USB 2 = 0,5 · USB 3 = 0,25
    std::vector<Cuadro> out(F);
    auto bloque = [&](Espacio& e) { std::fill(out.begin(), out.end(), Cuadro{}); e.suma(in.data(), F, C, out.data()); };

    {   // Fijo en FR: el primer bloque sube en rampa desde 0, el segundo ya es constante.
        Espacio e; e.prepara(&domo, 96000);
        e.obj[0].entrada = 1; e.obj[0].azimut = 30;
        bloque(e);
        bool rampa = true;
        for (unsigned i = 0; i < F; ++i) rampa = rampa && std::fabs(out[i][1] - 0.f) < 1e-7f;  // 1.er bloque: anuncia el cambio (de nada a USB 2)
        bloque(e);
        for (unsigned i = 0; i < F; ++i) rampa = rampa && std::fabs(out[i][1] - .5f * float(i + 1) / F) < 1e-6f;
        bloque(e);
        bool fijo = true;
        for (unsigned i = 0; i < F; ++i) for (int c = 0; c < NumCanales; ++c) fijo = fijo && std::fabs(out[i][c] - (c == 1 ? .5f : 0.f)) < 1e-6f;
        CHECK(rampa, "vivo: la entrada nueva debe subir en rampa desde 0");
        CHECK(fijo, "vivo: objeto fijo en 30° debe salir solo por FR con 0,5");
        CHECK(std::fabs(e.obj[0].posicion - 30) < 1e-4f, "vivo: la posición publicada debe ser 30 (%.3f)", e.obj[0].posicion.load());

        // Mover el objeto: sin saltos entre muestras consecutivas.
        float ultimo = out[F - 1][1];
        e.obj[0].azimut = 100;  // a SR
        bloque(e);
        float salto = std::fabs(out[0][1] - ultimo);
        for (unsigned i = 1; i < F; ++i) salto = std::max(salto, std::fabs(out[i][1] - out[i - 1][1]));
        CHECK(salto < .5f / F * 1.01f, "vivo: mover el objeto no debe dar saltos (%.5f)", salto);
        CHECK(std::fabs(out[F - 1][7] - .5f) < 1e-6f && std::fabs(out[F - 1][1]) < 1e-6f, "vivo: al final del bloque debe estar en SR");

        // Cambiar de entrada: la anterior baja a 0 en un bloque y la nueva sube en el siguiente.
        e.obj[0].entrada = 2;
        bloque(e);
        CHECK(std::fabs(out[0][7] - .5f * (1 - 1.f / F)) < 1e-6f && std::fabs(out[F - 1][7]) < 1e-7f, "vivo: la entrada anterior debe bajar a 0");
        bloque(e);
        CHECK(std::fabs(out[F - 1][7] - .25f) < 1e-6f, "vivo: la entrada nueva debe llegar a 0,25 (%.5f)", out[F - 1][7]);
        // Apagar: baja a 0 y después silencio.
        e.obj[0].entrada = -1; bloque(e); bloque(e);
        bool callado = true; for (auto& f : out) for (float v : f) callado = callado && v == 0.f;
        CHECK(callado, "vivo: un objeto apagado debe quedar en silencio");
    }
    {   // Giro a 60 rpm: en 1 s (1500 bloques de 64 a 96 kHz) da una vuelta completa.
        Espacio e; e.prepara(&domo, 96000);
        e.obj[3].entrada = 1; e.obj[3].modo = int(Modo::Giro); e.obj[3].rpm = 60; e.obj[3].azimut = 10;
        float maxPos = 0, minPos = 360;
        for (int b = 0; b < 1500; ++b) { bloque(e); maxPos = std::max(maxPos, e.obj[3].posicion.load()); minPos = std::min(minPos, e.obj[3].posicion.load()); }
        float p = e.obj[3].posicion;
        CHECK(std::fabs(p - 10) < .01f || std::fabs(p - 370) < .01f, "vivo: tras 1 s a 60 rpm debe volver a 10° (%.3f)", p);
        CHECK(maxPos > 350 && minPos < 5, "vivo: el giro debe recorrer todo el anillo");
        // LFE nunca recibe señal del objeto.
        bool lfe = true; for (auto& f : out) lfe = lfe && f[3] == 0.f;
        CHECK(lfe, "vivo: el LFE no debe recibir señal de un objeto");
    }
    {   // Velocidad maestra: a 0,5× el giro de 60 rpm da media vuelta en 1 s; a 0× se congela.
        Espacio e; e.prepara(&domo, 96000); e.velocidad = .5f;
        e.obj[0].entrada = 1; e.obj[0].modo = int(Modo::Giro); e.obj[0].rpm = 60;
        for (int b = 0; b < 1500; ++b) bloque(e);
        CHECK(std::fabs(e.obj[0].posicion - 180) < .01f, "vivo: a 0,5× debe ir en 180° (%.3f)", e.obj[0].posicion.load());
        e.velocidad = 0; float antes = e.obj[0].posicion;
        for (int b = 0; b < 100; ++b) bloque(e);
        CHECK(e.obj[0].posicion == antes, "vivo: a 0× el objeto debe quedarse quieto");
    }
    {   // Vaivén ±30° en 8 s: a los 2 s está en centro + 30.
        Espacio e; e.prepara(&domo, 96000);
        e.obj[1].entrada = 1; e.obj[1].modo = int(Modo::Vaiven); e.obj[1].amplitud = 30; e.obj[1].periodo = 8;
        for (int b = 0; b < 3000; ++b) bloque(e);
        CHECK(std::fabs(e.obj[1].posicion - 30) < .01f, "vivo: vaivén a los 2 s debe ir en 30° (%.3f)", e.obj[1].posicion.load());
    }
    {   // Entrada que la consola no manda, NaN y sin ficha: no truenan ni suenan.
        Espacio e; e.prepara(&domo, 96000);
        e.obj[0].entrada = 9; bloque(e); bloque(e);
        bool callado = true; for (auto& f : out) for (float v : f) callado = callado && v == 0.f;
        CHECK(callado, "vivo: una entrada inexistente debe quedar en silencio");
        e.obj[0].entrada = 0; in[0] = std::nanf(""); bloque(e); bloque(e);
        bool finito = true; for (auto& f : out) for (float v : f) finito = finito && std::isfinite(v);
        CHECK(finito, "vivo: un NaN de entrada no debe pasar a la salida");
        in[0] = 0;
        Espacio sinFicha; sinFicha.obj[0].entrada = 1; bloque(sinFicha);
        CHECK(out[0][0] == 0.f, "vivo: sin ficha no debe sonar");
    }
    {   // Integración con el núcleo del puente: apagado no cambia nada; encendido se suma a las rutas.
        bridge::Core c; c.gain = 1;
        for (unsigned k = 0; k < 8; ++k) c.route[k] = -1;
        c.route[2] = 2;  // USB 3 directo al centro
        std::vector<float> o(256 * 8);
        std::vector<float> in8(256 * 4, 0.f);
        for (unsigned i = 0; i < 256; ++i) { in8[i * 4 + 1] = .5f; in8[i * 4 + 2] = .25f; }
        for (int b = 0; b < 8; ++b) c.capture(in8.data(), 256, 4);
        c.render(o.data(), 256, 8);
        CHECK(std::fabs(o[100 * 8 + 2] - .25f) < 1e-6f && o[100 * 8 + 1] == 0.f, "núcleo: con el espacio apagado solo debe sonar la ruta directa");
        c.reset(); c.espacio.prepara(&domo, 96000);
        c.espacio.obj[0].entrada = 1; c.espacio.obj[0].azimut = 30; c.espacio.activo = true;
        for (int b = 0; b < 8; ++b) c.capture(in8.data(), 256, 4);
        c.render(o.data(), 256, 8);  // los primeros 256 incluyen la rampa de entrada
        c.render(o.data(), 256, 8);
        CHECK(std::fabs(o[100 * 8 + 2] - .25f) < 1e-6f && std::fabs(o[100 * 8 + 1] - .5f) < 1e-5f, "núcleo: el objeto en FR debe sumarse a la ruta directa (C %.4f, FR %.4f)", o[100 * 8 + 2], o[100 * 8 + 1]);
    }
}

static void pruebasFuenteArchivo(const Disposicion& domo) {
    // Objeto estéreo: L (USB 2 = 0,5) a −30° (FL) y R (USB 3 = 0,25) a +30° (FR).
    {
        const unsigned F = 64, C = 4;
        std::vector<float> in(F * C, 0.f);
        for (unsigned i = 0; i < F; ++i) { in[i * C + 1] = .5f; in[i * C + 2] = .25f; }
        std::vector<Cuadro> out(F);
        auto bloque = [&](Espacio& e) { std::fill(out.begin(), out.end(), Cuadro{}); e.suma(in.data(), F, C, out.data()); };
        Espacio e; e.prepara(&domo, 48000);
        e.obj[0].entrada = 1; e.obj[0].estereo = true; e.obj[0].ancho = 30; e.obj[0].azimut = 0;
        bloque(e); bloque(e); bloque(e);
        CHECK(std::fabs(out[F - 1][0] - .5f) < 1e-5f && std::fabs(out[F - 1][1] - .25f) < 1e-5f && std::fabs(out[F - 1][2]) < 1e-5f,
              "estéreo: L a FL y R a FR (FL %.4f FR %.4f C %.4f)", out[F - 1][0], out[F - 1][1], out[F - 1][2]);
        e.obj[0].estereo = false; bloque(e);  // pasar a mono: primero baja a 0
        CHECK(std::fabs(out[F - 1][0]) < 1e-6f && std::fabs(out[F - 1][1]) < 1e-6f, "estéreo → mono: debe bajar a 0 antes de cambiar");
        bloque(e);
        CHECK(std::fabs(out[F - 1][2] - .5f) < 1e-5f, "mono: USB 2 al centro (%.4f)", out[F - 1][2]);
    }
    // Reproductor: copia exacta del archivo, sin faltas, con y sin repetición.
    fuente::Archivo a; a.nombre = "prueba"; a.rate = 48000; a.canales = 2;
    const size_t N = 5000;
    for (size_t i = 0; i < N; ++i) { a.datos.push_back(float(i % 1000) / 1000.f); a.datos.push_back(-float(i % 777) / 1000.f); }
    for (bool repetir : { true, false }) {
        bridge::Core c; c.gain = 1; c.servo = false;
        for (unsigned k = 0; k < 8; ++k) c.route[k] = -1;
        c.route[0] = 0; c.route[1] = 1;
        fuente::Reproductor r; r.prepara(&a, repetir);
        std::vector<float> o(960 * 8);
        bool exacto = true; size_t pos = 0;
        for (int b = 0; b < 40; ++b) {  // 38 400 cuadros: el archivo da 7,7 vueltas
            r.alimenta(c, 960); c.render(o.data(), 960, 8);
            for (unsigned i = 0; i < 960; ++i, ++pos) {
                float l = pos < N || repetir ? a.datos[(pos % N) * 2] : 0.f, rr = pos < N || repetir ? a.datos[(pos % N) * 2 + 1] : 0.f;
                if (o[i * 8] != l || o[i * 8 + 1] != rr) exacto = false;
            }
        }
        CHECK(c.underflows == 0 && c.overflows == 0, "reproductor (%s): sin faltas ni desbordes (%llu/%llu)", repetir ? "repite" : "una vez", (unsigned long long)c.underflows.load(), (unsigned long long)c.overflows.load());
        CHECK(exacto, "reproductor (%s): la salida debe ser el archivo exacto", repetir ? "repite" : "una vez");
        CHECK(repetir != r.terminado(), "reproductor: «terminado» solo sin repetición");
    }
    // Lectura de WAV y de carpeta de capas.
    namespace fs = std::filesystem;
    fs::path dir = fs::temp_directory_path() / "pli_capas_prueba";
    fs::create_directories(dir);
    std::vector<float> l(100), rr(100);
    for (int i = 0; i < 100; ++i) { l[i] = float(i) / 200; rr[i] = -float(i) / 400; }
    for (auto capa : fuente::Capas) writeStereo((dir / (std::string(capa) + ".wav")).u8string(), l, rr, 48000);
    fuente::Archivo w = fuente::leeWav(dir / "voz.wav");
    bool igual = w.canales == 2 && w.cuadros() == 100;
    for (int i = 0; igual && i < 100; ++i) igual = std::fabs(w.datos[i * 2] - l[i]) < 1e-6f && std::fabs(w.datos[i * 2 + 1] - rr[i]) < 1e-6f;
    CHECK(igual, "fuente: leeWav estéreo de 24 bit");
    fuente::Archivo k = fuente::leeCapas(dir);
    CHECK(k.canales == 12 && k.cuadros() == 100 && k.etiquetas[4] == "brillos L" && std::fabs(k.datos[50 * 12 + 9] - rr[50]) < 1e-6f, "fuente: carpeta de capas → 12 canales en orden");
    fs::remove(dir / "bombo.wav");
    bool rechazo = false; try { fuente::leeCapas(dir); } catch (const std::exception&) { rechazo = true; }
    CHECK(rechazo, "fuente: una carpeta sin bombo.wav debe rechazarse");
    fs::remove_all(dir);
}

static void pruebasAdm(const Disposicion& domo) {
    using namespace pli::adm;
    // XML: espacios de nombres, entidades, comentarios, autocierre; error si no cierra.
    Nodo x = LectorXml("<?xml version=\"1.0\"?><!-- c --><a:raiz xmlns:a=\"u\" k='v &amp; w'><b>1 &lt; 2</b><c/></a:raiz>").leer();
    CHECK(x.hijos.size() == 1 && x.hijos[0].nombre == "raiz" && x.hijos[0].a("k") == "v & w" && x.hijos[0].hijo("b")->texto == "1 < 2" && x.hijos[0].hijo("c"), "XML: lectura básica");
    bool r = false; try { LectorXml("<a><b></a>").leer(); } catch (const std::exception&) { r = true; }
    CHECK(r, "XML: debe rechazar etiquetas mal cerradas");
    // Tiempos y signo del azimut.
    CHECK(std::fabs(leeTiempo("00:01:02.50000") - 62.5) < 1e-9 && std::fabs(leeTiempo("00:00:01.24000S48000") - 1.5) < 1e-9 && tiempo(62.5) == "00:01:02.50000", "ADM: tiempos");
    CHECK(admAz(30) == -30 && admAz(-100) == 100 && admAz(180) == 180, "ADM: azimut a la izquierda es positivo");

    // Escena con todo: fijo, estéreo, giro, vaivén, objeto LFE y LFE filtrado.
    const unsigned R = 8000; const size_t N = 3 * R;
    auto senal = [&](double f, double fase) { std::vector<float> v(N); for (size_t i = 0; i < N; ++i) v[i] = float(.2 * std::sin(2 * pli::Pi * f * i / R + fase)); return v; };
    auto objeto = [&](const char* n, std::vector<float> l, std::vector<float> r2) { Objeto o; o.nombre = n; o.audio.rate = R; o.audio.l = std::move(l); o.audio.r = std::move(r2); return o; };
    Escena e;
    { Objeto o = objeto("fijo", senal(110, 0), senal(110, 0)); o.mono = true; o.mov.puntos.p = { { 0, 30 } }; e.objetos.push_back(o); }
    { Objeto o = objeto("pareja", senal(220, 0), senal(330, 1)); o.ancho = 30; o.ganancia = .5; o.mov.puntos.p = { { 0, 0 } }; e.objetos.push_back(o); }
    { Objeto o = objeto("gira", senal(440, 0), senal(440, 0)); o.mono = true; o.mov.tipo = Movimiento::Giro; o.mov.inicio = 170; o.mov.rpm = 6; e.objetos.push_back(o); }
    { Objeto o = objeto("mece", senal(550, 0), senal(550, 0)); o.mono = true; o.inicio = .5; o.mov.tipo = Movimiento::Vaiven; o.mov.amplitud = 40; o.mov.periodo = 2; e.objetos.push_back(o); }
    { Objeto o = objeto("sub", senal(40, 0), senal(40, 0)); o.lfe = true; o.mov.puntos.p = { { 0, 0 } }; e.objetos.push_back(o); }
    e.lfe.activo = true; e.lfe.fuentes = { "fijo" }; e.lfe.corte = 80; e.lfe.ganancia = .5;

    Archivo a = desdeEscena(e);
    CHECK(a.pistas.size() == 7, "ADM: 7 pistas esperadas (fijo, pareja L/R, gira, mece, sub, LFE), hay %zu", a.pistas.size());
    Metadatos m = escribeMetadatos(a, "prueba & ida");
    std::vector<std::vector<float>> canales; for (auto& p : a.pistas) canales.push_back(p.audio);
    Archivo b = leeMetadatos(m.xml, m.chna, canales, R);
    CHECK(b.pistas.size() == 7 && b.avisos.empty(), "ADM: deben volver las 7 pistas sin avisos (%zu) %s", b.pistas.size(), b.avisos.c_str());
    int lfe = 0; for (auto& p : b.pistas) lfe += p.tipo == Tipo::Lfe;
    CHECK(lfe == 2, "ADM: deben volver 2 pistas LFE, volvieron %d", lfe);
    // Ida y vuelta: el render debe coincidir (fuera de los primeros 0,2 s, donde el primer bloque se adelanta).
    Multi m1 = renderiza(e, domo), m2 = renderiza(aEscena(b, "vuelta", ""), domo);
    CHECK(m1.size() == m2.size(), "ADM: misma duración (%zu, %zu)", m1.size(), m2.size());
    double err = 0, sig = 0;
    for (size_t i = size_t(.2 * R); i < std::min(m1.size(), m2.size()); ++i) for (int c = 0; c < NumCanales; ++c) { double d = m1[i][c] - m2[i][c]; err += d * d; sig += double(m1[i][c]) * m1[i][c]; }
    double dbErr = 10 * std::log10(err / sig + 1e-30);
    CHECK(dbErr < -40, "ADM: el render de ida y vuelta difiere %.1f dB (debe ser < −40 dB)", dbErr);

    // Archivo completo en disco (24 bit).
    std::string tmp = (std::filesystem::temp_directory_path() / "pli_adm_prueba.wav").u8string();
    escribeArchivo(tmp, a, m);
    Archivo c = leeArchivo(tmp);
    std::filesystem::remove(std::filesystem::u8path(tmp));
    double peor = 0; for (size_t k = 0; k < c.pistas.size() && k < a.pistas.size(); ++k) for (size_t i = 0; i < N; ++i) peor = std::max(peor, double(std::fabs(c.pistas[k].audio[i] - a.pistas[k].audio[i])));
    CHECK(c.pistas.size() == 7 && peor < 2.0 / 8388608, "ADM: el archivo debe conservar las pistas a 24 bit (error %.2e)", peor);

    // Definiciones comunes: una pista que apunta a AC_00010001 (L) sin XML propio.
    std::vector<uint8_t> chna = { 1, 0, 1, 0, 1, 0 };
    for (char ch : std::string("ATU_00000001")) chna.push_back(uint8_t(ch));
    for (char ch : std::string("AT_00010001_01")) chna.push_back(uint8_t(ch));
    for (char ch : std::string("AP_00010002")) chna.push_back(uint8_t(ch));
    chna.push_back(0);
    Archivo dc = leeMetadatos("<ebuCoreMain><coreMetadata/></ebuCoreMain>", chna, { std::vector<float>(10, .1f) }, 48000);
    CHECK(dc.pistas.size() == 1 && dc.pistas[0].tipo == Tipo::Altavoz && dc.pistas[0].azimut.p[0].second == -30, "ADM: AC_00010001 debe ser L a −30°");
    // Bloques separados: se queda en 90° (ADM, izquierda) hasta t=2 y llega a 0° en t=3; con salto en t=4.
    std::vector<uint8_t> chna1 = { 1, 0, 1, 0, 1, 0 };
    for (char ch : std::string("ATU_00000001")) chna1.push_back(uint8_t(ch));
    for (char ch : std::string("AT_00031001_01")) chna1.push_back(uint8_t(ch));
    for (char ch : std::string("AP_00031001")) chna1.push_back(uint8_t(ch));
    chna1.push_back(0);
    const char* xmlSep = R"(<audioFormatExtended><audioChannelFormat audioChannelFormatID="AC_00031001" audioChannelFormatName="nave" typeLabel="0003">
      <audioBlockFormat rtime="00:00:00.00000" duration="00:00:01.00000"><position coordinate="azimuth">90</position><position coordinate="elevation">0</position></audioBlockFormat>
      <audioBlockFormat rtime="00:00:02.00000" duration="00:00:01.00000"><position coordinate="azimuth">0</position><position coordinate="elevation">0</position></audioBlockFormat>
      <audioBlockFormat rtime="00:00:04.00000" duration="00:00:01.00000"><jumpPosition interpolationLength="0">1</jumpPosition><position coordinate="azimuth">-30</position><position coordinate="elevation">0</position></audioBlockFormat>
    </audioChannelFormat></audioFormatExtended>)";
    Archivo sep = leeMetadatos(xmlSep, chna1, { std::vector<float>(10, .1f) }, 48000);
    const Curva& cs = sep.pistas.at(0).azimut;
    CHECK(std::fabs(cs.en(1.5) + 90) < 1e-9 && std::fabs(cs.en(2.5) + 45) < 1e-9 && std::fabs(cs.en(3.5)) < 1e-9 && std::fabs(cs.en(3.99)) < 1e-9 && std::fabs(cs.en(4.01) - 30) < 1e-9,
          "ADM: bloques separados (1,5 s %.2f · 2,5 s %.2f · 3,5 s %.2f · 4,01 s %.2f)", cs.en(1.5), cs.en(2.5), cs.en(3.5), cs.en(4.01));
    r = false; try { leeMetadatos("<a/>", { 0, 0, 0, 0 }, {}, 48000); } catch (const std::exception&) { r = true; }
    CHECK(r, "ADM: sin pistas utilizables debe dar error");
}

int main(int argc, char** argv) {
    const std::string ficha = argc > 1 ? argv[1] : "../pruebas-domo/domo.json";
    try {
        // Ficha real del domo.
        Disposicion domo = Disposicion::desdeFicha(ficha);
        CHECK(domo.bocinas().size() == 7, "la ficha debe dar 7 bocinas con ángulo (el LFE no), dio %zu", domo.bocinas().size());
        pruebasDeDisposicion(domo, "domo.json");
        // Simetría izquierda/derecha de la ficha: g(θ)[FL] = g(-θ)[FR], etc.
        const int espejo[NumCanales] = { 1, 0, 2, 3, 5, 4, 7, 6 };
        for (double th = 0; th < 360; th += 7.3) {
            auto a = domo.ganancias(th), b = domo.ganancias(-th);
            for (int c = 0; c < NumCanales; ++c) CHECK(std::fabs(a[c] - b[espejo[c]]) < 1e-5f, "espejo falla en %.1f° canal %s", th, IdCanal[c]);
        }
        // A la mitad entre FL (-30) y FR (+30) pasando por el centro: 0° es solo el centro.
        // A la mitad entre FC (0) y FR (30): 15° reparte igual.
        auto m = domo.ganancias(15);
        CHECK(std::fabs(m[2] - m[1]) < 1e-5f && std::fabs(m[2] - float(std::sqrt(.5))) < 1e-5f, "15° debe repartir igual C y FR (%.4f, %.4f)", m[2], m[1]);

        // Anillo clásico de capas71.
        pruebasDeDisposicion(Disposicion::anilloClasico(), "anillo clásico");

        // Rechazos.
        bool rechazo = false;
        try { Disposicion({ { "FL", 0, -30 }, { "FC", 2, 0 }, { "FR", 1, 30 } }); } catch (const std::exception&) { rechazo = true; }
        CHECK(rechazo, "debe rechazar un anillo con un hueco de 300° (solo bocinas al frente)");
        rechazo = false;
        try { Disposicion({ { "FL", 0, 0 }, { "FC", 2, 0 }, { "FR", 1, 120 }, { "SR", 7, 240 } }); } catch (const std::exception&) { rechazo = true; }
        CHECK(rechazo, "debe rechazar dos bocinas en el mismo ángulo");
        rechazo = false;
        try { indiceCanal("XX"); } catch (const std::exception&) { rechazo = true; }
        CHECK(rechazo, "debe rechazar un id de canal desconocido");

        pruebasDeEscena(domo);
        pruebasEnVivo(domo);
        pruebasAdm(domo);
        pruebasFuenteArchivo(domo);

        // JSON: escapes y BOM no rompen la lectura.
        Json j = LectorJson(R"({"a":"x\"y","n":-1.5e1,"l":[null,true,false],"o":{}})").leer();
        CHECK(j["a"].s == "x\"y" && j["n"].n == -15 && j["l"].lista.size() == 3 && j["o"].tipo == Json::Objeto, "lector JSON");
    } catch (const std::exception& e) {
        std::printf("FALLA: %s\n", e.what()); return 1;
    }
    std::printf("PLI Espacial: %d comprobaciones, %d fallas\n", pruebas, fallas);
    return fallas ? 1 : 0;
}
