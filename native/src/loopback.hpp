#pragma once
// «Sonido de Windows»: captura (WASAPI loopback) lo que suena por una salida de Windows — por ejemplo
// el navegador con YouTube Music — para que Play Dolby lo reparta por el domo como si fuera la consola.
// La salida capturada debe ser OTRA que el receptor HDMI (Play Dolby lo usa en exclusiva).
// Windows no entrega paquetes cuando nada suena; para que el flujo no se corte se manda silencio a esa
// misma salida (truco estándar de loopback). Todo corre en un hilo propio (COM MTA).
// Sin dependencias: solo WASAPI de Windows (la portaudio.dll 19.5 del proyecto no ofrece loopback).
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <condition_variable>
#include <cstring>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace loopback {

// GUID propios (evitan depender de qué traiga la biblioteca uuid del compilador).
inline const GUID kClsEnum = { 0xBCDE0395, 0xE52F, 0x467C, { 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E } };
inline const GUID kIidEnum = { 0xA95664D2, 0x9614, 0x4F35, { 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6 } };
inline const GUID kIidClient = { 0x1CB9AD4C, 0xDBFA, 0x4C32, { 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2 } };
inline const GUID kIidCapture = { 0xC8ADBD64, 0xE71E, 0x48A0, { 0xA4, 0xDE, 0x18, 0x5C, 0x39, 0x5C, 0xD3, 0x17 } };
inline const GUID kIidRender = { 0xF294ACFC, 0x3146, 0x4483, { 0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2 } };
inline const GUID kSubFloat = { 0x00000003, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };
inline const GUID kSubPcm = { 0x00000001, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };
inline const PROPERTYKEY kNombre = { { 0xA45C254E, 0xDF1C, 0x4EFD, { 0x80, 0x20, 0x67, 0xD1, 0x46, 0xA8, 0x50, 0xE0 } }, 14 };

struct Dispositivo { std::wstring id, nombre; };

// Salidas de audio activas de Windows (para elegir qué capturar). Llamar con COM ya iniciado.
inline std::vector<Dispositivo> salidas() {
    std::vector<Dispositivo> r;
    IMMDeviceEnumerator* e = nullptr;
    if (FAILED(CoCreateInstance(kClsEnum, nullptr, CLSCTX_ALL, kIidEnum, reinterpret_cast<void**>(&e)))) return r;
    IMMDeviceCollection* c = nullptr;
    if (SUCCEEDED(e->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &c))) {
        UINT n = 0; c->GetCount(&n);
        for (UINT i = 0; i < n; ++i) {
            IMMDevice* d = nullptr;
            if (FAILED(c->Item(i, &d))) continue;
            LPWSTR id = nullptr; std::wstring nombre;
            d->GetId(&id);
            IPropertyStore* ps = nullptr;
            if (SUCCEEDED(d->OpenPropertyStore(STGM_READ, &ps))) {
                PROPVARIANT v; PropVariantInit(&v);
                if (SUCCEEDED(ps->GetValue(kNombre, &v)) && v.vt == VT_LPWSTR) nombre = v.pwszVal;
                PropVariantClear(&v); ps->Release();
            }
            if (id) { r.push_back({ id, nombre }); CoTaskMemFree(id); }
            d->Release();
        }
        c->Release();
    }
    e->Release();
    return r;
}

// Convierte un bloque del formato de mezcla de Windows a float intercalado.
struct Formato {
    unsigned rate = 0, canales = 0, bits = 0, bloque = 0; bool flotante = false;
    bool lee(const WAVEFORMATEX* f) {
        rate = f->nSamplesPerSec; canales = f->nChannels; bits = f->wBitsPerSample; bloque = f->nBlockAlign;
        if (f->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) flotante = true;
        else if (f->wFormatTag == WAVE_FORMAT_PCM) flotante = false;
        else if (f->wFormatTag == WAVE_FORMAT_EXTENSIBLE && f->cbSize >= 22) {
            auto x = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(f);
            if (IsEqualGUID(x->SubFormat, kSubFloat)) flotante = true;
            else if (IsEqualGUID(x->SubFormat, kSubPcm)) flotante = false;
            else return false;
            if (x->Samples.wValidBitsPerSample) bits = f->wBitsPerSample;  // contenedor (24 en 32 se lee como 32)
        } else return false;
        return canales >= 1 && canales <= 32 && rate > 0 && (flotante ? bits == 32 : (bits == 16 || bits == 24 || bits == 32));
    }
    void convierte(const BYTE* p, unsigned frames, float* o) const {
        const size_t n = size_t(frames) * canales;
        for (size_t i = 0; i < n; ++i) {
            const BYTE* s = p + i * (bits / 8);
            float v;
            if (flotante) std::memcpy(&v, s, 4);
            else if (bits == 16) v = int16_t(uint16_t(s[0] | s[1] << 8)) / 32768.f;
            else if (bits == 24) v = float(int32_t(uint32_t(s[0]) << 8 | uint32_t(s[1]) << 16 | uint32_t(s[2]) << 24) / 2147483648.);
            else { int32_t x; std::memcpy(&x, s, 4); v = float(x / 2147483648.); }
            o[i] = std::isfinite(v) ? v : 0.f;
        }
    }
};

class Captura {
public:
    using Destino = std::function<void(const float*, unsigned frames, unsigned canales)>;
    unsigned rate = 0, canales = 0;
    std::string error;

    // Abre la salida «id» en loopback y empieza a entregar audio a «destino» desde su propio hilo.
    bool abre(const std::wstring& id, Destino destino) {
        cierra();
        destino_ = std::move(destino); corre_ = true; listo_ = false; error.clear();
        hilo_ = std::thread([this, id] { bucle(id); });
        std::unique_lock<std::mutex> l(m_);
        cv_.wait_for(l, std::chrono::seconds(5), [this] { return listo_; });
        if (!listo_) { error = "La salida de Windows no respondió."; corre_ = false; }
        if (!error.empty()) { l.unlock(); cierra(); return false; }
        return true;
    }
    void cierra() { corre_ = false; if (hilo_.joinable()) hilo_.join(); }
    ~Captura() { cierra(); }
    uint64_t cuadros() const { return cuadros_.load(); }  // entregados (para diagnóstico)

private:
    void avisa(const std::string& e) { std::lock_guard<std::mutex> l(m_); if (!e.empty()) error = e; listo_ = true; cv_.notify_all(); }
    void bucle(std::wstring id) {
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        IMMDeviceEnumerator* en = nullptr; IMMDevice* dev = nullptr; IAudioClient* cap = nullptr; IAudioClient* ren = nullptr;
        IAudioCaptureClient* cc = nullptr; IAudioRenderClient* rc = nullptr; WAVEFORMATEX* mix = nullptr;
        auto falla = [&](const char* q, HRESULT h) { char b[160]; std::snprintf(b, sizeof b, "%s (código 0x%08lX)", q, (unsigned long)h); avisa(b); };
        HRESULT h;
        Formato fmt;
        if (FAILED(h = CoCreateInstance(kClsEnum, nullptr, CLSCTX_ALL, kIidEnum, reinterpret_cast<void**>(&en)))) falla("No se pudo abrir el sistema de audio de Windows", h);
        else if (FAILED(h = en->GetDevice(id.c_str(), &dev))) falla("La salida de Windows ya no existe (¿se desconectó?)", h);
        else if (FAILED(h = dev->Activate(kIidClient, CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&cap)))) falla("No se pudo activar la captura", h);
        else if (FAILED(h = cap->GetMixFormat(&mix))) falla("No se pudo leer el formato de la salida", h);
        else if (!fmt.lee(mix)) avisa("Formato de la salida de Windows no soportado.");
        else if (FAILED(h = cap->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 200000, 0, mix, nullptr))) falla("No se pudo iniciar la captura (loopback)", h);
        else if (FAILED(h = cap->GetService(kIidCapture, reinterpret_cast<void**>(&cc)))) falla("No se pudo obtener el capturador", h);
        else {
            // Silencio hacia la misma salida: mantiene el flujo aunque nada suene.
            if (SUCCEEDED(dev->Activate(kIidClient, CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&ren))) &&
                SUCCEEDED(ren->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 200000, 0, mix, nullptr)) &&
                SUCCEEDED(ren->GetService(kIidRender, reinterpret_cast<void**>(&rc)))) ren->Start();
            else if (ren) { ren->Release(); ren = nullptr; }
            rate = fmt.rate; canales = fmt.canales;
            cap->Start();
            avisa("");
            std::vector<float> buf;
            while (corre_) {
                Sleep(3);
                if (rc) {
                    UINT32 tam = 0, pend = 0; BYTE* p = nullptr;
                    if (SUCCEEDED(ren->GetBufferSize(&tam)) && SUCCEEDED(ren->GetCurrentPadding(&pend)) && tam > pend &&
                        SUCCEEDED(rc->GetBuffer(tam - pend, &p))) rc->ReleaseBuffer(tam - pend, AUDCLNT_BUFFERFLAGS_SILENT);
                }
                UINT32 paquete = 0;
                while (corre_ && SUCCEEDED(cc->GetNextPacketSize(&paquete)) && paquete) {
                    BYTE* datos = nullptr; UINT32 frames = 0; DWORD flags = 0;
                    if (FAILED(cc->GetBuffer(&datos, &frames, &flags, nullptr, nullptr))) break;
                    buf.resize(size_t(frames) * fmt.canales);
                    if (flags & AUDCLNT_BUFFERFLAGS_SILENT) std::fill(buf.begin(), buf.end(), 0.f);
                    else fmt.convierte(datos, frames, buf.data());
                    if (frames && destino_) destino_(buf.data(), frames, fmt.canales);
                    cuadros_ += frames;
                    cc->ReleaseBuffer(frames);
                }
            }
            cap->Stop(); if (ren) ren->Stop();
        }
        if (!listo_) avisa("No se pudo abrir la salida de Windows.");
        if (rc) rc->Release(); if (ren) ren->Release(); if (cc) cc->Release(); if (cap) cap->Release();
        if (mix) CoTaskMemFree(mix); if (dev) dev->Release(); if (en) en->Release();
        CoUninitialize();
    }
    Destino destino_;
    std::thread hilo_;
    std::atomic<bool> corre_{ false };
    std::atomic<uint64_t> cuadros_{ 0 };
    std::mutex m_; std::condition_variable cv_; bool listo_ = false;
};

}  // namespace loopback
