// Prueba física de «Sonido de Windows» (loopback). Manda un tono MUY bajo (−60 dBFS) a una salida y lo
// captura de vuelta: flujo continuo sin audio, nivel correcto y efecto de silenciar la salida.
//   loopback_test.exe [parte del nombre de la salida] [--mute]
// --mute silencia la salida 1 s para medir si la captura sigue (restaura el estado al terminar).
#include "../src/loopback.hpp"
#include <endpointvolume.h>
#include <cstdio>

static const GUID kIidVol = { 0x5CDF2C82, 0x841E, 0x4546, { 0x97, 0x22, 0x0C, 0xF7, 0x40, 0x78, 0x22, 0x9A } };
static std::atomic<double> suma{ 0 }; static std::atomic<uint64_t> n{ 0 };
static void reinicia() { suma = 0; n = 0; }
static double rmsDb() { return n ? 10 * std::log10(suma / double(n) + 1e-30) : -999; }

// Tono por la misma salida (WASAPI compartido, formato de mezcla float).
static void tono(IMMDevice* d, double segundos, double amplitud) {
    IAudioClient* c = nullptr; IAudioRenderClient* r = nullptr; WAVEFORMATEX* mix = nullptr;
    d->Activate(loopback::kIidClient, CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&c)); c->GetMixFormat(&mix);
    loopback::Formato f; f.lee(mix);
    if (!f.flotante) { std::printf("  (la mezcla no es flotante: se omite el tono)\n"); CoTaskMemFree(mix); c->Release(); Sleep(DWORD(segundos * 1000)); return; }
    c->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 1000000, 0, mix, nullptr); c->GetService(loopback::kIidRender, reinterpret_cast<void**>(&r));
    UINT32 tam = 0; c->GetBufferSize(&tam); c->Start();
    double fase = 0; const size_t total = size_t(segundos * f.rate); size_t hecho = 0;
    while (hecho < total) {
        UINT32 pend = 0; c->GetCurrentPadding(&pend); UINT32 libre = std::min<UINT32>(tam - pend, UINT32(total - hecho));
        if (libre) { BYTE* p = nullptr; r->GetBuffer(libre, &p); float* o = reinterpret_cast<float*>(p);
            for (UINT32 i = 0; i < libre; ++i) { float v = float(amplitud * std::sin(fase)); fase += 2 * 3.14159265358979 * 1000 / f.rate; for (unsigned k = 0; k < f.canales; ++k) o[i * f.canales + k] = v; }
            r->ReleaseBuffer(libre, 0); hecho += libre; }
        Sleep(5);
    }
    Sleep(150); c->Stop(); r->Release(); c->Release(); CoTaskMemFree(mix);
}

int main(int argc, char** argv) {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    std::wstring busca = L"Altavoces"; bool mute = false;
    for (int i = 1; i < argc; ++i) { std::string a = argv[i]; if (a == "--mute") mute = true; else busca = std::wstring(a.begin(), a.end()); }
    auto lista = loopback::salidas();
    std::printf("Salidas de Windows: %zu\n", lista.size());
    const loopback::Dispositivo* sel = nullptr;
    for (auto& d : lista) { std::wprintf(L"  · %ls\n", d.nombre.c_str()); if (!sel && d.nombre.find(busca) != std::wstring::npos) sel = &d; }
    if (!sel) { std::printf("FALLA: no hay salida que contenga el nombre pedido.\n"); return 1; }
    std::wprintf(L"Capturando: %ls\n", sel->nombre.c_str());
    loopback::Captura cap;
    if (!cap.abre(sel->id, [](const float* x, unsigned fr, unsigned ch) { double s = 0; for (size_t i = 0; i < size_t(fr) * ch; ++i) s += double(x[i]) * x[i]; suma = suma + s; n += size_t(fr) * ch; })) {
        std::printf("FALLA: %s\n", cap.error.c_str()); return 1;
    }
    std::printf("Formato de mezcla: %u Hz, %u canales\n", cap.rate, cap.canales);
    IMMDeviceEnumerator* en = nullptr; IMMDevice* dev = nullptr;
    CoCreateInstance(loopback::kClsEnum, nullptr, CLSCTX_ALL, loopback::kIidEnum, reinterpret_cast<void**>(&en)); en->GetDevice(sel->id.c_str(), &dev);
    int fallas = 0;

    uint64_t c0 = cap.cuadros(); reinicia(); Sleep(1000); uint64_t c1 = cap.cuadros();
    double flujo = double(c1 - c0) / cap.rate;
    std::printf("1) Sin audio: %.2f s de cuadros en 1 s (flujo continuo), nivel %.1f dBFS\n", flujo, rmsDb());
    if (flujo < .9 || flujo > 1.1) { ++fallas; std::printf("   FALLA: el flujo no es continuo\n"); }

    reinicia(); tono(dev, 2, 0.001);   // −60 dBFS de pico → −63 dBFS RMS
    double nivel = rmsDb();
    std::printf("2) Tono de −60 dBFS (pico): capturado %.1f dBFS RMS (sin procesos de Windows serían ≈ −63)\n", nivel);
    if (nivel < -90) { ++fallas; std::printf("   FALLA: no llegó el tono\n"); }
    else if (std::fabs(nivel + 63) > 2) std::printf("   AVISO: %+.1f dB de diferencia: algún mejorador de la salida (Nahimic, FxSound…) altera el audio antes de la captura\n", nivel + 63);

    if (mute) {
        IAudioEndpointVolume* vol = nullptr; BOOL antes = FALSE;
        if (SUCCEEDED(dev->Activate(kIidVol, CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&vol)))) {
            vol->GetMute(&antes); vol->SetMute(TRUE, nullptr);
            reinicia(); tono(dev, 1, 0.001);
            std::printf("3) Con la salida SILENCIADA: capturado %.1f dBFS RMS → %s\n", rmsDb(), rmsDb() > -80 ? "la captura SIGUE (se puede callar esa salida)" : "la captura queda en silencio (no callar esa salida)");
            vol->SetMute(antes, nullptr); vol->Release();
            std::printf("   silencio restaurado a: %s\n", antes ? "silenciada" : "con sonido");
        }
    }
    cap.cierra(); dev->Release(); en->Release(); CoUninitialize();
    std::printf(fallas ? "RESULTADO: FALLA\n" : "RESULTADO: PASA\n");
    return fallas;
}
