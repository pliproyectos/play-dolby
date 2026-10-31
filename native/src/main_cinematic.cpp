#ifndef UNICODE
#define UNICODE
#endif
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <cstring>
#include "../vendor/portaudio.h"
#include "bridge_core.hpp"
#include "fuente_archivo.hpp"
#include "wasapi_options.hpp"
#include "window_chrome.hpp"

// Forward declaration of fonts used by theme
HFONT font = nullptr, titleFont = nullptr, smallFont = nullptr, channelFont = nullptr;

struct AudioApi {
    HMODULE module = nullptr;
#define PA_FN(name) decltype(&name) name = nullptr;
    PA_FN(Pa_Initialize) PA_FN(Pa_Terminate) PA_FN(Pa_GetDeviceCount)
    PA_FN(Pa_GetDeviceInfo) PA_FN(Pa_GetHostApiInfo) PA_FN(Pa_GetErrorText)
    PA_FN(Pa_IsFormatSupported) PA_FN(Pa_OpenStream) PA_FN(Pa_StartStream)
    PA_FN(Pa_AbortStream) PA_FN(Pa_CloseStream) PA_FN(Pa_IsStreamActive)
    PA_FN(Pa_GetStreamInfo)
#undef PA_FN
    void load() {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(nullptr, path, MAX_PATH);
        auto dll = std::filesystem::path(path).parent_path() / L"portaudio.dll";
        module = LoadLibraryW(dll.c_str());
        if (!module) throw std::runtime_error("No se pudo cargar portaudio.dll.");
#define LOAD(name) { auto address = GetProcAddress(module, #name); static_assert(sizeof(name) == sizeof(address)); std::memcpy(&name, &address, sizeof(name)); if (!name) throw std::runtime_error(#name); }
        LOAD(Pa_Initialize) LOAD(Pa_Terminate) LOAD(Pa_GetDeviceCount)
        LOAD(Pa_GetDeviceInfo) LOAD(Pa_GetHostApiInfo) LOAD(Pa_GetErrorText)
        LOAD(Pa_IsFormatSupported) LOAD(Pa_OpenStream) LOAD(Pa_StartStream)
        LOAD(Pa_AbortStream) LOAD(Pa_CloseStream) LOAD(Pa_IsStreamActive) LOAD(Pa_GetStreamInfo)
#undef LOAD
        auto e = Pa_Initialize();
        if (e != paNoError) throw std::runtime_error(Pa_GetErrorText(e));
    }
} api;

std::wstring wide(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), n);
    if (!w.empty()) w.pop_back();
    return w;
}

std::filesystem::path dataDir() {
    wchar_t dir[32768];
    GetEnvironmentVariableW(L"LOCALAPPDATA", dir, 32768);
    auto p = std::filesystem::path(dir) / L"PlanetarioBridge";
    std::filesystem::create_directories(p);
    return p;
}

void log(const std::string& s) {
    std::ofstream f(dataDir() / L"bridge.log", std::ios::app);
    auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    f << t << " " << s << "\n";
}

// Identificadores de la lista de entradas que no son dispositivos de PortAudio.
constexpr int IdArchivo = -100, IdElegirArchivo = -101, IdElegirCapas = -102;
const char* bridge_names[8] = { "FL", "FR", "C", "LFE", "BL", "BR", "SL", "SR" };

struct Engine {
    bridge::Core core;
    // Fuente archivo: un WAV (o carpeta de capas) en memoria hace de consola. Solo se cambia con el puente detenido.
    fuente::Archivo archivo;
    fuente::Reproductor reproductor;
    bool desdeArchivo = false;
    PaStream *input = nullptr, *output = nullptr;
    unsigned inputChannels = 0, outputChannels = 2;
    std::atomic<uint64_t> inputFlags{0}, outputFlags{0};
    bool test = false;
    double rate = 48000;  // Sigue al reloj de la consola (Qu-5 USB: 96 kHz).
    std::atomic<int> toneChannel{-1};
    std::atomic<unsigned> toneRemaining{0};
    double tonePhase = 0;
    bool running() const { return output != nullptr; }

    static int capture(const void* in, void*, unsigned long n, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags flags, void* self) {
        auto& e = *static_cast<Engine*>(self);
        if (flags) ++e.inputFlags;
        e.core.capture(static_cast<const float*>(in), n, e.inputChannels);
        return paContinue;
    }

    static int render(const void*, void* out, unsigned long n, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags flags, void* self) {
        auto& e = *static_cast<Engine*>(self);
        if (flags) ++e.outputFlags;
        auto samples = static_cast<float*>(out);
        if (!e.test) {
            if (e.desdeArchivo) e.reproductor.alimenta(e.core, unsigned(n));
            e.core.render(samples, n, e.outputChannels);
        } else {
            std::fill(samples, samples + n * e.outputChannels, 0.f);
            int channel = e.toneChannel.load();
            unsigned remain = e.toneRemaining.load();
            for (unsigned c = 0; c < 8; ++c) e.core.peak[c] = 0;
            if (channel >= 0 && unsigned(channel) < e.outputChannels) {
                unsigned emitted = std::min<unsigned>(n, remain);
                for (unsigned i = 0; i < emitted; ++i) {
                    const double total = 2 * e.rate, ramp = .05 * e.rate;
                    double envelope = std::min({ 1., double(total - remain + i) / ramp, double(remain - i) / ramp });
                    float value = float(std::sin(e.tonePhase) * .0158489 * envelope); // -36 dBFS
                    samples[i * e.outputChannels + channel] = value;
                    e.tonePhase += 6.283185307179586 * (channel == 3 ? 60. : 440.) / e.rate;
                    if (e.tonePhase > 6.283185307179586) e.tonePhase -= 6.283185307179586;
                }
                e.toneRemaining = remain - emitted;
                e.core.peak[channel] = emitted ? .0158489f : 0.f;
            }
        }
        return paContinue;
    }

    void stop() {
        if (input) { api.Pa_AbortStream(input); api.Pa_CloseStream(input); input = nullptr; }
        if (output) { api.Pa_AbortStream(output); api.Pa_CloseStream(output); output = nullptr; }
        desdeArchivo = false;
        toneRemaining = 0;
        toneChannel = -1;
        for (auto& p : core.peak) p = 0;
    }

    void check(PaError e) {
        if (e != paNoError) throw std::runtime_error(api.Pa_GetErrorText(e));
    }

    void start(int in, int out, unsigned channels, bool testing, int tone = -1) {
        stop();
        core.reset();
        test = testing;
        outputChannels = channels;
        inputFlags = 0;
        outputFlags = 0;
        try {
            auto od = api.Pa_GetDeviceInfo(out);
            if (!od) throw std::runtime_error("Selecciona la salida HDMI.");
            if (api.Pa_GetHostApiInfo(od->hostApi)->type != paWASAPI) throw std::runtime_error("Selecciona una salida WASAPI.");
            // La consola conserva su reloj; Windows adapta la salida a la frecuencia del mezclador.
            desdeArchivo = in == IdArchivo && !test;
            core.servo = !desdeArchivo;  // archivo: mismo reloj que la salida, copia exacta
            auto id = in >= 0 ? api.Pa_GetDeviceInfo(in) : nullptr;
            rate = in == IdArchivo && archivo.rate ? archivo.rate : id ? id->defaultSampleRate : 48000;
            core.espacio.prepara(core.espacio.disposicion(), rate);  // PLI Espacial: misma ficha, frecuencia de la consola
            // Compartido: Chrome y el puente pueden usar el mismo receptor simultáneamente.
            // AutoConvert permite la consola a 96 kHz aunque Windows mezcle a 48 kHz.
            auto wasapiInfo = wasapi::streamInfo(false, channels, true);
            PaStreamParameters op{ out, int(channels), paFloat32, 0, &wasapiInfo };
            if (api.Pa_IsFormatSupported(nullptr, &op, rate) != paNoError)
                throw std::runtime_error("El HDMI no acepta " + std::to_string(channels) + " canales a " + std::to_string(int(rate)) + " Hz en modo compartido. Configura la salida de Windows con el mismo número de canales y revisa que ninguna aplicación la use en exclusiva.");
            if (!test) {
                if (!id && !desdeArchivo) throw std::runtime_error("Selecciona la consola, un archivo WAV o una carpeta de capas en «01 Conecta tu consola».");
                // Cuántas entradas pide el parche (rutas directas + objetos de PLI Espacial) y de dónde.
                inputChannels = 1; std::string quien;
                for (unsigned c = 0; c < channels; ++c) {
                    unsigned pide = unsigned(std::max(0, core.route[c].load()) + 1);
                    if (pide > inputChannels) { inputChannels = pide; quien = "la salida " + std::string(bridge_names[c]) + " (USB " + std::to_string(pide) + ")"; }
                }
                if (core.espacio.activo) for (int k = 0; k < pli::MaxObjetos; ++k) {
                    auto& o = core.espacio.obj[k];
                    unsigned pide = unsigned(std::max(0, o.entrada.load()) + (o.estereo && o.entrada >= 0 ? 2 : 1));
                    if (pide > inputChannels) { inputChannels = pide; quien = "el objeto " + std::to_string(k + 1) + " de PLI Espacial (USB " + std::to_string(pide) + ")"; }
                }
                const unsigned tiene = desdeArchivo ? archivo.canales : unsigned(id->maxInputChannels);
                if (tiene < inputChannels)
                    throw std::runtime_error("La entrada tiene " + std::to_string(tiene) + " canales y " + quien + " pide " + std::to_string(inputChannels) +
                                             ". Elige la consola u otra fuente, o pon en «Silencio» las salidas por encima de USB " + std::to_string(tiene) + ".");
            }
            if (!test && desdeArchivo) {
                if (!archivo.cuadros()) throw std::runtime_error("El archivo está vacío.");
                inputChannels = archivo.canales;
                reproductor.prepara(&archivo, true);
            } else if (!test) {
                // Bloque ASIO de 64 muestras: con 0 el driver Qu baja a 16 (estable 2 min, pero frágil).
                // 64 a 96 kHz = 0,67 ms; probado sin faltas ni avisos del driver.
                PaStreamParameters ip{ in, int(inputChannels), paFloat32, 64. / rate, nullptr };
                check(api.Pa_IsFormatSupported(&ip, nullptr, rate));
                check(api.Pa_OpenStream(&input, &ip, nullptr, rate, paFramesPerBufferUnspecified, paNoFlag, capture, this));
            }
            check(api.Pa_OpenStream(&output, nullptr, &op, rate, paFramesPerBufferUnspecified, paNoFlag, render, this));
            if (test) {
                toneChannel = tone;
                toneRemaining = unsigned(2 * rate);
                tonePhase = 0;
            }
            if (input) check(api.Pa_StartStream(input));
            check(api.Pa_StartStream(output));
            std::ostringstream s;
            s << "START " << (test ? "test" : "bridge") << " output=" << od->name << " channels=" << channels << " rate=" << rate << " shared autoconvert latency=" << api.Pa_GetStreamInfo(output)->outputLatency;
            if (input) s << " input=" << api.Pa_GetDeviceInfo(in)->name << " channels=" << inputChannels << " latency=" << api.Pa_GetStreamInfo(input)->inputLatency;
            if (desdeArchivo) s << " input=archivo:" << archivo.nombre << " channels=" << archivo.canales;
            log(s.str());
        } catch (...) {
            stop();
            throw;
        }
    }
} engine;

HWND window = nullptr, inputBox = nullptr, outputBox = nullptr, profileBox = nullptr, statusLabel = nullptr, gainBox = nullptr;
HWND routes[8] = {}, meters[8] = {}, testButtons[8] = {};

#include "ui_theme_cinematic.hpp"
#include "cinematic_viewport.hpp"

std::wstring boxText(HWND c) {
    int n = GetWindowTextLengthW(c);
    std::wstring s(n + 1, 0);
    GetWindowTextW(c, s.data(), n + 1);
    s.resize(n);
    return s;
}

unsigned channelCount() {
    int p = SendMessage(profileBox, CB_GETCURSEL, 0, 0);
    return p == 0 ? 2 : p == 1 ? 6 : 8;
}

HWND control(const wchar_t* cls, const wchar_t* text, int x, int y, int w, int h, int id, DWORD extra = 0) {
    if (std::wcscmp(cls, L"BUTTON") == 0) extra |= BS_OWNERDRAW;
    bool combo = std::wcscmp(cls, L"COMBOBOX") == 0;
    if (combo) extra |= CBS_OWNERDRAWFIXED | CBS_HASSTRINGS;
    HWND c = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | extra, x, y, w, h, window, (HMENU)(INT_PTR)id, GetModuleHandle(nullptr), nullptr);
    SendMessage(c, WM_SETFONT, (WPARAM)font, TRUE);
    if (combo) {
        SendMessage(c, CB_SETITEMHEIGHT, -1, 34);
        SendMessage(c, CB_SETITEMHEIGHT, 0, 32);
        SetWindowSubclass(c, comboProc, 1, 0);
    }
    cinematicView::remember(c,x,y,w,h,combo,id==0||(id>=30&&id<38));
    if(combo)SetWindowSubclass(c,cinematicView::comboPaint,2,0);
    if(std::wcscmp(cls,L"BridgeMeter")==0)SetWindowSubclass(c,cinematicView::meterPaint,2,0);
    return c;
}

void status(const std::wstring& s) {
    static std::wstring previous;
    static bool wasRunning = false;
    if (s != previous) {
        SetWindowTextW(statusLabel, s.c_str());
        previous = s;
    }
    const bool running = engine.running();
    if (running != wasRunning) {
        RECT badge={1024,18,1190,53};cinematicView::invalidate(window,badge);
        wasRunning = running;
    }
}

int deviceId(HWND box) {
    int i = SendMessage(box, CB_GETCURSEL, 0, 0);
    return i < 0 ? -1 : int(SendMessage(box, CB_GETITEMDATA, i, 0));
}

void enableControls(bool idle) {
    for (auto c : { inputBox, outputBox, profileBox, gainBox }) EnableWindow(c, idle);
    for (int i = 0; i < 8; ++i) {
        EnableWindow(routes[i], idle && i < int(channelCount()));
        EnableWindow(testButtons[i], idle && i < int(channelCount()));
    }
    for (int id : { 10, 11, 14, 15, 16 }) EnableWindow(GetDlgItem(window, id), idle);
    RECT channels = { 34, 436, 1186, 725 };
    cinematicView::invalidate(window,channels);
}

void stop();
bool start(bool test, int channel = -1);

#include "channel_tester.hpp"
#include "espacial_panel.hpp"
#include "fuente_ui.hpp"
#include "actualizacion_ui.hpp"

// Botón «Extensión Chrome 7.1»: abre INSTALAR-EXTENSION.bat (copia la extensión a la PC, pone la ruta en el
// portapapeles y abre chrome://extensions). Instalado: <exe>\extension-chrome; en el proyecto: chrome-extension.
void instalaExtension() {
    wchar_t exe[MAX_PATH]; GetModuleFileNameW(nullptr, exe, MAX_PATH);
    auto dir = std::filesystem::path(exe).parent_path();
    std::filesystem::path bat = dir / L"extension-chrome" / L"INSTALAR-EXTENSION.bat";
    if (!std::filesystem::exists(bat)) bat = (dir / L".." / L".." / L"chrome-extension" / L"INSTALAR-EXTENSION.bat").lexically_normal();
    if (!std::filesystem::exists(bat)) { status(L"No encuentro la extensión de Chrome junto a Play Dolby (carpeta extension-chrome)."); return; }
    wchar_t sys[MAX_PATH]; GetSystemDirectoryW(sys, MAX_PATH);
    std::wstring cmd = L"\"" + std::wstring(sys) + L"\\cmd.exe\" /c \"\"" + bat.wstring() + L"\"\"";
    STARTUPINFOW si{}; si.cb = sizeof(si); PROCESS_INFORMATION pi{};
    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, bat.parent_path().c_str(), &si, &pi)) {
        status(L"No se pudo abrir el instalador de la extensión."); return;
    }
    CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
    log("EXTENSION instalador abierto");
    status(L"Se abrió el instalador de la extensión: en Chrome activa Modo de desarrollador › Cargar descomprimida › Ctrl+V. "
           L"La extensión y Play Dolby pueden sonar a la vez por la salida compartida de Windows.");
}

void stop() {
    tester::cancel();
    engine.stop();
    enableControls(true);
    status(L"Detenido. Salida en silencio.");
    log("STOP");
    tester::sync();
}

void refresh() {
    stop();
    api.Pa_Terminate();
    auto err = api.Pa_Initialize();
    SendMessage(inputBox, CB_RESETCONTENT, 0, 0);
    SendMessage(outputBox, CB_RESETCONTENT, 0, 0);
    if (err != paNoError) {
        status(wide(api.Pa_GetErrorText(err)));
        return;
    }
    int total = api.Pa_GetDeviceCount();
    bool asio = false;
    for (int i = 0; i < total; ++i) {
        auto d = api.Pa_GetDeviceInfo(i);
        auto host = api.Pa_GetHostApiInfo(d->hostApi);
        if (host->type == paASIO) asio = true;
        auto add = [&](HWND box, int channels) {
            auto s = wide(std::string(d->name) + " / " + host->name + " / " + std::to_string(channels) + " canales");
            int ix = SendMessage(box, CB_ADDSTRING, 0, (LPARAM)s.c_str());
            SendMessage(box, CB_SETITEMDATA, ix, i);
        };
        if (d->maxInputChannels > 0 && (host->type == paASIO || host->type == paWASAPI)) add(inputBox, d->maxInputChannels);
        if (d->maxOutputChannels >= 2 && host->type == paWASAPI) add(outputBox, d->maxOutputChannels);
    }
    fuenteUI::agregaOpciones();  // «▶ Archivo WAV…», «▶ Carpeta de capas…» y el archivo cargado
    SendMessage(inputBox, CB_SETCURSEL, -1, 0);
    SendMessage(outputBox, CB_SETCURSEL, -1, 0);
    status(asio ? L"Dispositivos detectados. Selecciona la consola ASIO y la salida HDMI WASAPI." : L"No hay controlador ASIO detectado. Puedes probar WASAPI; Para la Qu-5 instala el controlador Qu-5/6/7 y conecta USB-C.");
    tester::sync();
}

void save() {
    std::wofstream f(dataDir() / L"perfil.txt");
    f << boxText(inputBox) << L'\n' << boxText(outputBox) << L'\n' << SendMessage(profileBox, CB_GETCURSEL, 0, 0) << L'\n' << SendMessage(gainBox, CB_GETCURSEL, 0, 0) << L'\n';
    for (auto r : routes) f << SendMessage(r, CB_GETCURSEL, 0, 0) << L'\n';
    status(L"Perfil guardado en %LOCALAPPDATA%\\PlanetarioBridge. Nunca se inicia audio automáticamente.");
}

void load() {
    std::wifstream f(dataDir() / L"perfil.txt");
    if (!f) { status(L"No hay perfil guardado."); return; }
    std::wstring in, out;
    std::getline(f, in);
    std::getline(f, out);
    int p, g;
    int map[8];
    if (!(f >> p >> g) || p < 0 || p > 2 || g < 0 || g > 3) { status(L"Perfil inválido."); return; }
    for (auto& m : map) if (!(f >> m) || m < 0 || m > 32) { status(L"Perfil inválido."); return; }
    auto match = [](HWND box, const std::wstring& value) {
        int n = SendMessage(box, CB_GETCOUNT, 0, 0);
        for (int i = 0; i < n; ++i) {
            int len = SendMessage(box, CB_GETLBTEXTLEN, i, 0);
            std::wstring s(len + 1, 0);
            SendMessage(box, CB_GETLBTEXT, i, (LPARAM)s.data());
            s.resize(len);
            if (s == value) {
                SendMessage(box, CB_SETCURSEL, i, 0);
                return true;
            }
        }
        SendMessage(box, CB_SETCURSEL, -1, 0);
        return false;
    };
    bool a = match(inputBox, in), b = match(outputBox, out);
    SendMessage(profileBox, CB_SETCURSEL, p, 0);
    SendMessage(gainBox, CB_SETCURSEL, g, 0);
    for (int i = 0; i < 8; ++i) SendMessage(routes[i], CB_SETCURSEL, map[i], 0);
    enableControls(true);
    status(a && b ? L"Perfil cargado. Revisa los dispositivos antes de iniciar." : L"Dispositivo del perfil ausente. Selección vacía; no se sustituye por otro.");
    tester::sync();
}

bool start(bool test, int channel) {
    try {
        for (int c = 0; c < 8; ++c) engine.core.route[c] = int(SendMessage(routes[c], CB_GETCURSEL, 0, 0)) - 1;
        const float gains[] = { .12589254f, .25118864f, .50118723f, 1.f };
        int g = SendMessage(gainBox, CB_GETCURSEL, 0, 0);
        engine.core.gain = gains[std::clamp(g, 0, 3)];
        engine.start(deviceId(inputBox), deviceId(outputBox), channelCount(), test, channel);
        enableControls(false);
        tester::sync();
        return true;
    } catch (const std::exception& e) {
        log(std::string("ERROR ") + e.what());
        status(L"No se inició: " + wide(e.what()));
        enableControls(true);
        tester::sync();
        return false;
    }
}

LRESULT CALLBACK proc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_THEMECHANGED || m == WM_SETTINGCHANGE || m == WM_DWMCOMPOSITIONCHANGED) {
        chrome::apply(h);
    }
    if(m==WM_SIZE){cinematicView::fit(h);return 0;}
    if(m==WM_GETMINMAXINFO){  // tamaño mínimo usable: 70 % del diseño (1220×840)
        RECT r={0,0,854,588};AdjustWindowRect(&r,WS_OVERLAPPEDWINDOW,FALSE);
        auto mm=(MINMAXINFO*)l;mm->ptMinTrackSize.x=r.right-r.left;mm->ptMinTrackSize.y=r.bottom-r.top;return 0;
    }
    if(m==WM_DISPLAYCHANGE){
        if(cinematicView::fullscreen){
            MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);
            GetMonitorInfoW(MonitorFromWindow(h,MONITOR_DEFAULTTONEAREST),&monitor);
            SetWindowPos(h,nullptr,monitor.rcMonitor.left,monitor.rcMonitor.top,monitor.rcMonitor.right-monitor.rcMonitor.left,monitor.rcMonitor.bottom-monitor.rcMonitor.top,SWP_NOZORDER|SWP_NOACTIVATE);
        }
        cinematicView::fit(h);return 0;
    }
    if (m == WM_ERASEBKGND) return 1;
    if (m == WM_PAINT) {
        PAINTSTRUCT ps;
        auto dc = BeginPaint(h, &ps);
        RECT r;
        GetClientRect(h, &r);
        cinematicView::paint(dc,r,ps.rcPaint,engine.running());
        EndPaint(h, &ps);
        return 0;
    }
    if (m == WM_CTLCOLORSTATIC) {
        SetTextColor((HDC)w, theme::textLight);
        SetBkColor((HDC)w, theme::bgPaper);
        return (LRESULT)theme::background;
    }
    if (m == WM_CTLCOLORLISTBOX) {
        SetTextColor((HDC)w, theme::textLight);
        SetBkColor((HDC)w, theme::cardBg);
        return (LRESULT)theme::cardBrush;
    }
    if (m == WM_MEASUREITEM) {
        auto item = (MEASUREITEMSTRUCT*)l;
        item->itemHeight = std::max(18,int(std::lround(32*cinematicView::scale)));
        return TRUE;
    }
    if (m == WM_DRAWITEM) {
        auto d = (DRAWITEMSTRUCT*)l;
        if (d->CtlType == ODT_COMBOBOX) {
            theme::fill(d->hDC, d->rcItem, d->itemState & ODS_SELECTED ? RGB(38, 52, 74) : theme::cardBg);
            if (d->itemID != UINT(-1)) {
                int n = int(SendMessage(d->hwndItem, CB_GETLBTEXTLEN, d->itemID, 0));
                std::wstring value(n + 1, 0);
                SendMessage(d->hwndItem, CB_GETLBTEXT, d->itemID, (LPARAM)value.data());
                RECT r = d->rcItem;
                r.left += 12;
                theme::text(d->hDC, value.c_str(), r, (HFONT)SendMessageW(d->hwndItem,WM_GETFONT,0,0), theme::textLight, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            }
            return TRUE;
        }
        if (d->CtlType != ODT_BUTTON) return FALSE;
        bool disabled = d->itemState & ODS_DISABLED, pressed = d->itemState & ODS_SELECTED;
        const bool space = d->CtlID == 19 && engine.core.espacio.activo, hot = d->CtlID == 11 || (d->CtlID == 25 && !actualizacion::lista.empty()) || space;
        COLORREF bg = hot ? theme::blue : d->CtlID == 12 ? RGB(54,39,70) : RGB(35,32,46);
        if (disabled) bg = RGB(24, 25, 34);
        else if (pressed) bg = RGB(45, 48, 68);

        theme::fill(d->hDC, d->rcItem, theme::cardBg);
        theme::round(d->hDC, d->rcItem, bg, 6, hot ? theme::accentCyan : theme::cardBorder);
        SetBkMode(d->hDC, TRANSPARENT);
        SetTextColor(d->hDC, disabled ? RGB(100, 105, 120) : (hot ? RGB(255, 255, 255) : theme::textLight));
        auto old = SelectObject(d->hDC, (HFONT)SendMessageW(d->hwndItem,WM_GETFONT,0,0));
        auto caption = boxText(d->hwndItem);
        DrawTextW(d->hDC, caption.c_str(), -1, &d->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(d->hDC, old);
        if (d->itemState & ODS_FOCUS) {
            RECT r = d->rcItem;
            InflateRect(&r, -3, -3);
            DrawFocusRect(d->hDC, &r);
        }
        return TRUE;
    }
    if (m == WM_COMMAND) {
        int id = LOWORD(w), code = HIWORD(w);
        if (code == CBN_SELCHANGE) {
            if (id == 1) fuenteUI::seleccion();
            InvalidateRect((HWND)l, nullptr, FALSE);
            tester::sync();
        }
        if (id == 10) refresh();
        if (id == 11) start(false);
        if (id == 12) stop();
        if (id == 14) save();
        if (id == 15) load();
        if (id == 16) tester::open();
        if (id == 18) espacialUI::open();
        if (id == 19) espacialUI::alterna();
        if (id == 25) actualizacion::instalar();
        if (id == 26) instalaExtension();
        if (id >= 20 && id < 24) espacialUI::escenaRapida(id - 20);
        if (id == 24 && code == CBN_SELCHANGE) {
            const float speeds[] = { 0.f, .25f, .5f, .75f, 1.f, 1.25f, 1.5f };
            int s = std::clamp(int(SendMessage((HWND)l, CB_GETCURSEL, 0, 0)), 0, 6);
            engine.core.espacio.velocidad = speeds[s];
            status(L"Dynamic Space: velocidad maestra " + boxText((HWND)l).substr(10) + L".");
        }
        if (id >= 100 && id < 108) start(true, id - 100);
        if (id == 3 && code == CBN_SELCHANGE) {
            enableControls(true);
            status(L"Perfil actualizado. 5.1 usa FL FR C LFE BL BR; verifica el mapeo físico del receptor.");
        }
        return 0;
    }
    if (m == WM_TIMER) {
        if(w==2){
            static const auto began=std::chrono::steady_clock::now();
            theme::animationSeconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-began).count();
            BOOL effects=TRUE;SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION,0,&effects,0);
            // Dynamic Space encendido: domo y barras se animan siempre; apagado, solo si Windows tiene animaciones.
            const bool space=engine.core.espacio.activo;
            if(!IsIconic(h)&&(space||effects))cinematicView::invalidate(h,space?theme::dynamicBounds:theme::orbitBounds);
            return 0;
        }
        espacialUI::syncPrincipal();
        actualizacion::tick();
        tester::tick();

        if (engine.running()) {
            if (api.Pa_IsStreamActive(engine.output) != 1 || (engine.input && api.Pa_IsStreamActive(engine.input) != 1)) {
                stop();
                status(L"Dispositivo detenido o desconectado. Selecciona la salida y reinicia manualmente.");
                return 0;
            }
            if (engine.test && engine.toneRemaining == 0) {
                tester::finishTone();
                return 0;
            }
            std::wostringstream s;
            if (engine.desdeArchivo)
                s << L"Archivo " << fuenteUI::tiempo(engine.reproductor.segundos()) << L" / " << fuenteUI::tiempo(double(engine.archivo.cuadros()) / engine.archivo.rate) << L" (se repite) | ";
            s << (engine.test ? L"Tono de prueba −36 dBFS" : L"Puente en vivo") << L" | buffer " << engine.core.ring.available() * 1000 / unsigned(engine.rate) << L" ms · " << int(engine.rate / 1000) << L" kHz | corrección " << int(engine.core.ppm.load()) << L" ppm\n"
              << L"Faltas: " << engine.core.underflows << L" | desbordes: " << engine.core.overflows << L" | clipping: " << engine.core.clipped << L" | avisos driver E/S: " << engine.inputFlags << L" / " << engine.outputFlags;
            status(s.str());
        } else {
            tester::tick();
        }
        for (auto hMeter : meters) meter::update(hMeter);
        return 0;
    }
    if (m == WM_CLOSE) {
        stop();
        if(tester::panel)DestroyWindow(tester::panel);
        if(espacialUI::panel)DestroyWindow(espacialUI::panel);
        DestroyWindow(h);
        return 0;
    }
    if (m == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);  // diálogos de archivo/carpeta de Windows
    Gdiplus::GdiplusStartupInput graphicsInput;
    ULONG_PTR graphicsToken;
    if (Gdiplus::GdiplusStartup(&graphicsToken, &graphicsInput, nullptr) != Gdiplus::Ok) return 1;
    try {
        api.load();
    } catch (const std::exception& e) {
        MessageBoxW(nullptr, wide(e.what()).c_str(), L"Play Dolby", MB_ICONERROR);
        Gdiplus::GdiplusShutdown(graphicsToken);
        return 1;
    }
    InitCommonControls();
    auto makeFont = [](int size, int weight) {
        return CreateFontW(-size, 0, 0, 0, weight, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    };
    font = makeFont(15, FW_NORMAL);
    smallFont = makeFont(12, FW_NORMAL);
    titleFont = makeFont(48, FW_SEMIBOLD);
    channelFont = makeFont(24, FW_SEMIBOLD);

    WNDCLASSW wc{};
    wc.lpfnWndProc = proc;
    wc.hInstance = inst;
    wc.lpszClassName = L"PlanetarioBridgeCinematic";
    wc.hbrBackground = theme::background;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = chrome::icon(false);
    RegisterClassW(&wc);

    WNDCLASSW meterClass{};
    meterClass.lpfnWndProc = meterProc;
    meterClass.hInstance = inst;
    meterClass.lpszClassName = L"BridgeMeter";
    RegisterClassW(&meterClass);

    RECT bounds = { 0, 0, 1220, 840 };
    // Ventana normal de Windows: minimizar, maximizar y bordes para cambiar el tamaño (el contenido se escala).
    DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
    AdjustWindowRect(&bounds, style, FALSE);

    window = CreateWindowW(wc.lpszClassName, L"Play Dolby", style, CW_USEDEFAULT, CW_USEDEFAULT, bounds.right - bounds.left, bounds.bottom - bounds.top, nullptr, nullptr, inst, nullptr);
    chrome::apply(window);
    chrome::setIcons(window);

    // Actualizaciones con aviso: «Buscar actualizaciones», o resaltado «Actualización X · Instalar» si hay una lista.
    HWND botonActualizar = control(L"BUTTON", L"⟳  Buscar actualizaciones", 690, 18, 318, 35, 25, WS_TABSTOP);
    control(L"BUTTON", L"◆  Extensión Chrome 7.1", 440, 18, 236, 35, 26, WS_TABSTOP);

    // Dynamic Space (PLI Espacial fase 3b), en la columna junto al domo: encendido, escenas rápidas y velocidad maestra.
    control(L"BUTTON", L"○  DYNAMIC SPACE", 974, 80, 216, 30, 19, WS_TABSTOP);
    {
        const wchar_t* escenas[] = { L"◌  Quieto", L"↻  Órbita", L"∿  Respira", L"★  La de 1:45" };
        for (int i = 0; i < 4; ++i) control(L"BUTTON", escenas[i], 974, 116 + i * 31, 216, 27, 20 + i, WS_TABSTOP);
        HWND speed = control(L"COMBOBOX", L"", 974, 241, 216, 260, 24, CBS_DROPDOWNLIST | WS_TABSTOP);
        for (auto s : { L"Velocidad 0× · congelar", L"Velocidad 0,25×", L"Velocidad 0,5×", L"Velocidad 0,75×", L"Velocidad 1×", L"Velocidad 1,25×", L"Velocidad 1,5×" }) SendMessage(speed, CB_ADDSTRING, 0, (LPARAM)s);
        SendMessage(speed, CB_SETCURSEL, 4, 0);
    }

    inputBox = control(L"COMBOBOX", L"", 48, 320, 534, 350, 1, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP);
    outputBox = control(L"COMBOBOX", L"", 636, 320, 536, 350, 2, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP);

    profileBox = control(L"COMBOBOX", L"", 30, 384, 214, 180, 3, CBS_DROPDOWNLIST | WS_TABSTOP);
    for (auto s : { L"Estéreo · 2 canales", L"5.1 · 6 canales", L"7.1 · 8 canales" }) SendMessage(profileBox, CB_ADDSTRING, 0, (LPARAM)s);
    SendMessage(profileBox, CB_SETCURSEL, 0, 0);

    gainBox = control(L"COMBOBOX", L"", 258, 384, 196, 180, 4, CBS_DROPDOWNLIST | WS_TABSTOP);
    for (auto s : { L"Ganancia −18 dB", L"Ganancia −12 dB", L"Ganancia −6 dB", L"Ganancia 0 dB" }) SendMessage(gainBox, CB_ADDSTRING, 0, (LPARAM)s);
    SendMessage(gainBox, CB_SETCURSEL, 0, 0);

    control(L"BUTTON", L"↻  Dispositivos", 474, 384, 220, 36, 10, WS_TABSTOP);
    control(L"BUTTON", L"Guardar", 710, 384, 120, 36, 14, WS_TABSTOP);
    control(L"BUTTON", L"Cargar", 842, 384, 120, 36, 15, WS_TABSTOP);
    control(L"BUTTON", L"◎  Probador de canales", 974, 384, 216, 36, 16, WS_TABSTOP);

    for (int c = 0; c < 8; ++c) {
        int x = 30 + c * 146;
        routes[c] = control(L"COMBOBOX", L"", x + 12, 570, 114, 300, 30 + c, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP);
        SendMessage(routes[c], CB_ADDSTRING, 0, (LPARAM)L"Silencio");
        for (int j = 1; j <= 32; ++j) {
            auto s = L"USB " + std::to_wstring(j);
            SendMessage(routes[c], CB_ADDSTRING, 0, (LPARAM)s.c_str());
        }
        SendMessage(routes[c], CB_SETCURSEL, c + 1, 0);
        meters[c] = control(L"BridgeMeter", L"", x + 26, 609, 86, 66, 200 + c);
        testButtons[c] = control(L"BUTTON", L"▷  Probar", x + 12, 684, 114, 28, 100 + c, WS_TABSTOP);
    }

    control(L"BUTTON", L"✦  PLI Espacial", 514, 745, 228, 36, 18, WS_TABSTOP);
    control(L"BUTTON", L"▶  Iniciar puente", 758, 745, 228, 36, 11, WS_TABSTOP);
    control(L"BUTTON", L"■  Detener · Esc", 1002, 745, 174, 36, 12, WS_TABSTOP);

    statusLabel = control(L"STATIC", L"", 34, 804, 1150, 30, 0);
    SendMessage(statusLabel, WM_SETFONT, (WPARAM)smallFont, TRUE);

    cinematicView::ready=true;
    // Start fitted to the current monitor work area, including shorter laptop displays.
    MONITORINFO initialMonitor{};initialMonitor.cbSize=sizeof(initialMonitor);
    GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&initialMonitor);
    const auto& work=initialMonitor.rcWork;
    int initialWidth=std::min(int(bounds.right-bounds.left),int(work.right-work.left));
    int initialHeight=std::min(int(bounds.bottom-bounds.top),int(work.bottom-work.top));
    SetWindowPos(window,nullptr,work.left+(work.right-work.left-initialWidth)/2,work.top+(work.bottom-work.top-initialHeight)/2,initialWidth,initialHeight,SWP_NOZORDER|SWP_NOACTIVATE);
    cinematicView::fit(window);
    refresh();
    espacialUI::cargaFicha();  // puente detenido: lee pruebas-domo\domo.json para PLI Espacial
    theme::dynamicPainter = espacialUI::pintaPrincipal;
    espacialUI::syncPrincipal();
    actualizacion::inicia(botonActualizar);
    SetTimer(window, 1, 100, nullptr);
    SetTimer(window, 2, 33, nullptr);
    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            stop();
            continue;
        }
        if(tester::panel&&IsDialogMessage(tester::panel,&msg))continue;
        if(espacialUI::panel&&IsDialogMessage(espacialUI::panel,&msg))continue;
        if (!IsDialogMessage(window, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    api.Pa_Terminate();
    if (api.module) FreeLibrary(api.module);
    cinematicView::release();
    theme::releaseLogo();
    DeleteObject(font);
    DeleteObject(titleFont);
    DeleteObject(smallFont);
    DeleteObject(channelFont);
    DeleteObject(theme::background);
    DeleteObject(theme::cardBrush);
    DeleteObject(theme::slotBrush);
    Gdiplus::GdiplusShutdown(graphicsToken);
    return 0;
}
