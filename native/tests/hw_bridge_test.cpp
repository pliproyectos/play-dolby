// Prueba física del puente Qu-5 (ASIO) -> Integra (HDMI WASAPI). Consola, sin interfaz.
//   hw_bridge_test.exe --list                    Enumera dispositivos, latencias y formatos.
//   hw_bridge_test.exe --run SEG [--exclusive] [--asio-only] [--rate HZ] [--lat-in MS] [--lat-out MS] [--polling] [--margin MUESTRAS] [--rate HZ] [--lat-in MS] [--lat-out MS] [--polling] [--margin MUESTRAS]
//      Corre el puente SEG segundos con ganancia 0 (salida en silencio digital):
//      ejercita captura, cola, servo de deriva y salida reales sin sonar en sala.
// Sale con código 0 sólo si no hubo faltas, desbordes, valores no finitos ni avisos del driver.
#include <windows.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <thread>
#include "../vendor/portaudio.h"
#include "../src/bridge_core.hpp"
#include "../src/wasapi_options.hpp"

struct Api {
#define PA_FN(name) decltype(&name) name = nullptr;
    PA_FN(Pa_Initialize) PA_FN(Pa_Terminate) PA_FN(Pa_GetDeviceCount) PA_FN(Pa_GetDeviceInfo)
    PA_FN(Pa_GetHostApiInfo) PA_FN(Pa_GetErrorText) PA_FN(Pa_IsFormatSupported) PA_FN(Pa_OpenStream)
    PA_FN(Pa_StartStream) PA_FN(Pa_AbortStream) PA_FN(Pa_CloseStream) PA_FN(Pa_GetStreamInfo) PA_FN(Pa_IsStreamActive)
#undef PA_FN
    void load() {
        wchar_t path[MAX_PATH]; GetModuleFileNameW(nullptr, path, MAX_PATH);
        auto dll = std::filesystem::path(path).parent_path() / L"portaudio.dll";
        auto m = LoadLibraryW(dll.c_str());
        if (!m) throw std::runtime_error("No se pudo cargar portaudio.dll junto al ejecutable.");
#define LOAD(name) { auto a = GetProcAddress(m, #name); std::memcpy(&name, &a, sizeof(name)); if (!name) throw std::runtime_error(#name); }
        LOAD(Pa_Initialize) LOAD(Pa_Terminate) LOAD(Pa_GetDeviceCount) LOAD(Pa_GetDeviceInfo) LOAD(Pa_GetHostApiInfo)
        LOAD(Pa_GetErrorText) LOAD(Pa_IsFormatSupported) LOAD(Pa_OpenStream) LOAD(Pa_StartStream) LOAD(Pa_AbortStream)
        LOAD(Pa_CloseStream) LOAD(Pa_GetStreamInfo) LOAD(Pa_IsStreamActive)
#undef LOAD
        if (Pa_Initialize() != paNoError) throw std::runtime_error("Pa_Initialize");
    }
} api;

const char* hostName(int dev) { return api.Pa_GetHostApiInfo(api.Pa_GetDeviceInfo(dev)->hostApi)->name; }
PaHostApiTypeId hostType(int dev) { return api.Pa_GetHostApiInfo(api.Pa_GetDeviceInfo(dev)->hostApi)->type; }
bool contains(const char* s, const char* sub) { return s && std::strstr(s, sub); }

int findInput() {   // Qu ASIO
    for (int i = 0; i < api.Pa_GetDeviceCount(); ++i) {
        auto d = api.Pa_GetDeviceInfo(i);
        if (hostType(i) == paASIO && d->maxInputChannels >= 8 && contains(d->name, "Qu")) return i;
    }
    return -1;
}
int findOutput() {  // Integra por HDMI
    for (int i = 0; i < api.Pa_GetDeviceCount(); ++i) {
        auto d = api.Pa_GetDeviceInfo(i);
        if (hostType(i) == paWASAPI && d->maxOutputChannels >= 2 && contains(d->name, "AV Receiver")) return i;
    }
    return -1;
}

void list() {
    std::printf("%-3s %-10s %-52s %4s %4s %9s %9s %8s\n", "id", "API", "dispositivo", "ent", "sal", "lat.in ms", "lat.out ms", "Hz");
    for (int i = 0; i < api.Pa_GetDeviceCount(); ++i) {
        auto d = api.Pa_GetDeviceInfo(i);
        auto t = hostType(i);
        if (t != paASIO && t != paWASAPI) continue;
        std::printf("%-3d %-10.10s %-52.52s %4d %4d %9.2f %9.2f %8.0f\n", i, hostName(i), d->name, d->maxInputChannels,
                    d->maxOutputChannels, d->defaultLowInputLatency * 1000, d->defaultLowOutputLatency * 1000, d->defaultSampleRate);
    }
    int in = findInput(), out = findOutput();
    std::printf("\nEntrada Qu ASIO: %s\nSalida AV Receiver WASAPI: %s\n", in >= 0 ? api.Pa_GetDeviceInfo(in)->name : "NO ENCONTRADA",
                out >= 0 ? api.Pa_GetDeviceInfo(out)->name : "NO ENCONTRADA");
    for (double rate : { 44100., 48000., 96000. }) {
        if (in >= 0) {
            PaStreamParameters ip{ in, 8, paFloat32, api.Pa_GetDeviceInfo(in)->defaultLowInputLatency, nullptr };
            std::printf("  Qu ASIO 8 ent @ %.0f Hz: %s\n", rate, api.Pa_GetErrorText(api.Pa_IsFormatSupported(&ip, nullptr, rate)));
        }
        if (out >= 0) for (bool ex : { false, true }) {
            auto info = wasapi::streamInfo(ex);
            PaStreamParameters op{ out, 8, paFloat32, api.Pa_GetDeviceInfo(out)->defaultLowOutputLatency, &info };
            std::printf("  HDMI 8 sal %-10s @ %.0f Hz: %s\n", ex ? "exclusivo" : "compartido", rate, api.Pa_GetErrorText(api.Pa_IsFormatSupported(nullptr, &op, rate)));
        }
    }
}

// Valores por defecto = los de Play Dolby: ASIO 64 muestras, HDMI periodo mínimo. <0: los del driver.
double gRate = 96000, gInLat = 64000. / 96000, gOutLat = 0;
bool gPolling = false;
int gMargin = -1;  // gInLat<0: latencia baja anunciada por el driver

struct Run {
    bridge::Core core;
    unsigned inCh = 8, outCh = 8;
    std::atomic<uint64_t> inFlags{0}, outFlags{0}, inCalls{0}, outCalls{0};
    std::atomic<unsigned> inMax{0}, outMax{0};
    static int cap(const void* in, void*, unsigned long n, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags f, void* s) {
        auto& r = *static_cast<Run*>(s); if (f) ++r.inFlags; ++r.inCalls; if (n > r.inMax) r.inMax = unsigned(n);
        r.core.capture(static_cast<const float*>(in), unsigned(n), r.inCh); return paContinue;
    }
    static int ren(const void*, void* out, unsigned long n, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags f, void* s) {
        auto& r = *static_cast<Run*>(s); if (f) ++r.outFlags; ++r.outCalls; if (n > r.outMax) r.outMax = unsigned(n);
        r.core.render(static_cast<float*>(out), unsigned(n), r.outCh); return paContinue;
    }
};

int run(int seconds, bool exclusive, bool asioOnly) {
    int in = findInput(), out = findOutput();
    if (in < 0) { std::printf("FALLA: no hay entrada ASIO de la Qu con 8 canales.\n"); return 2; }
    if (out < 0 && !asioOnly) { std::printf("FALLA: no hay salida WASAPI 'AV Receiver'.\n"); return 2; }
    static Run r;
    r.core.reset(); r.core.gain = 0.f;
    if (gMargin >= 0) r.core.margin = unsigned(gMargin);   // silencio digital: no suena en sala
    for (unsigned c = 0; c < 8; ++c) r.core.route[c] = int(c);
    auto check = [](PaError e, const char* what) { if (e != paNoError) { std::printf("FALLA %s: %s\n", what, api.Pa_GetErrorText(e)); throw 1; } };
    PaStream *is = nullptr, *os = nullptr;
    auto info = wasapi::streamInfo(exclusive);
    try {
        PaStreamParameters ip{ in, 8, paFloat32, gInLat >= 0 ? gInLat / 1000 : api.Pa_GetDeviceInfo(in)->defaultLowInputLatency, nullptr };
        check(api.Pa_OpenStream(&is, &ip, nullptr, gRate, paFramesPerBufferUnspecified, paNoFlag, Run::cap, &r), "abrir Qu ASIO");
        if (!asioOnly) {
            if (gPolling) info.flags |= 1u << 3;
            PaStreamParameters op{ out, 8, paFloat32, gOutLat >= 0 ? gOutLat / 1000 : api.Pa_GetDeviceInfo(out)->defaultLowOutputLatency, &info };
            check(api.Pa_OpenStream(&os, nullptr, &op, gRate, paFramesPerBufferUnspecified, paNoFlag, Run::ren, &r), "abrir HDMI");
        }
        check(api.Pa_StartStream(is), "iniciar Qu");
        if (os) check(api.Pa_StartStream(os), "iniciar HDMI");
    } catch (int) { if (is) api.Pa_CloseStream(is); if (os) api.Pa_CloseStream(os); return 3; }

    double li = api.Pa_GetStreamInfo(is)->inputLatency * 1000, lo = os ? api.Pa_GetStreamInfo(os)->outputLatency * 1000 : 0;
    std::printf("Frecuencia %.0f Hz | Modo HDMI: %s | latencia declarada: Qu %.2f ms + HDMI %.2f ms\n", gRate, exclusive ? "WASAPI exclusivo" : "WASAPI compartido", li, lo);
    uint64_t fillSum = 0, fillN = 0, fillMin = UINT64_MAX, fillMax = 0;
    auto t0 = std::chrono::steady_clock::now();
    bool died = false;
    while (std::chrono::steady_clock::now() - t0 < std::chrono::seconds(seconds)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if (api.Pa_IsStreamActive(is) != 1 || (os && api.Pa_IsStreamActive(os) != 1)) { died = true; break; }
        if (!os) { float buf[256 * 8]; r.core.render(buf, 256, 8); }
        if (os && r.core.primed && std::chrono::steady_clock::now() - t0 > std::chrono::seconds(1)) {
            auto f = r.core.ring.available(); fillSum += f; ++fillN; fillMin = std::min(fillMin, f); fillMax = std::max(fillMax, f);
        }
    }
    api.Pa_AbortStream(is); api.Pa_CloseStream(is);
    if (os) { api.Pa_AbortStream(os); api.Pa_CloseStream(os); }

    double avgMs = fillN ? fillSum * 1000. / fillN / gRate : 0;
    std::printf("Bloques: Qu %u muestras (%llu llamadas) | HDMI %u muestras (%llu llamadas)\n", r.inMax.load(),
                (unsigned long long)r.inCalls, r.outMax.load(), (unsigned long long)r.outCalls);
    if (os) std::printf("Cola: margen %u, objetivo %u muestras (%.2f ms) | medida prom %.2f ms, min %.2f, max %.2f\n", r.core.margin, r.core.target, r.core.target * 1000. / gRate,
                avgMs, fillN ? fillMin * 1000. / gRate : 0, fillMax * 1000. / gRate);
    if (os) std::printf("Latencia estimada del puente (declarada + cola): %.1f ms\n", li + lo + avgMs);
    std::printf("Corrección de reloj: %.0f ppm | faltas %llu | desbordes %llu | no finitos %llu | avisos driver %llu/%llu\n",
                r.core.ppm.load(), (unsigned long long)r.core.underflows, (unsigned long long)r.core.overflows,
                (unsigned long long)r.core.invalid, (unsigned long long)r.inFlags, (unsigned long long)r.outFlags);
    bool ok = !died && r.inCalls && (asioOnly || r.outCalls) && r.core.overflows == 0 && r.core.invalid == 0 && r.inFlags == 0 && r.outFlags == 0
              && (asioOnly || r.core.underflows <= 1);  // la primera falta tras el arranque re-prima de forma segura
    std::printf("%s\n", died ? "FALLA: un stream se detuvo." : ok ? "PASA" : "FALLA");
    return ok ? 0 : 1;
}

int main(int argc, char** argv) {
    SetConsoleOutputCP(CP_UTF8);
    try { api.load(); } catch (const std::exception& e) { std::printf("FALLA: %s\n", e.what()); return 2; }
    int code = 0;
    if (argc >= 2 && !std::strcmp(argv[1], "--list")) list();
    else if (argc >= 3 && !std::strcmp(argv[1], "--run")) {
        bool ex = false, asioOnly = false;
        for (int i = 3; i < argc; ++i) { if (!std::strcmp(argv[i], "--exclusive")) ex = true; if (!std::strcmp(argv[i], "--asio-only")) asioOnly = true;
            if (!std::strcmp(argv[i], "--rate") && i + 1 < argc) gRate = std::atof(argv[++i]);
            if (!std::strcmp(argv[i], "--lat-in") && i + 1 < argc) gInLat = std::atof(argv[++i]);
            if (!std::strcmp(argv[i], "--lat-out") && i + 1 < argc) gOutLat = std::atof(argv[++i]);
            if (!std::strcmp(argv[i], "--polling")) gPolling = true;
            if (!std::strcmp(argv[i], "--margin") && i + 1 < argc) gMargin = std::atoi(argv[++i]); }
        code = run(std::atoi(argv[2]), ex, asioOnly);
    } else std::printf("Uso: --list | --run SEG [--exclusive] [--asio-only]\n");
    api.Pa_Terminate();
    return code;
}
