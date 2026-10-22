#pragma once
// Actualizaciones CON AVISO dentro de Play Dolby (el trabajo pesado lo hace actualizador.ps1):
//  - Al abrir (tras 10 s) y cada «intervalo_horas» (actualizacion.json, 12 por omisión) lanza
//    «actualizador.ps1 -Revisar -DesdeApp» oculto: consulta GitHub, baja y verifica la versión nueva.
//  - Cada 30 s lee %LOCALAPPDATA%\PlanetarioBridge\actualizacion\estado.json; si hay una versión más
//    nueva lista, muestra el botón «⟳ Actualización … · Instalar» en el encabezado.
//  - Instalar: nunca con el puente en vivo; pide confirmación, lanza «-Instalar -Reabrir» y cierra.
// Solo se activa si junto al exe están VERSION.txt, actualizador.ps1 y actualizacion.json
// (versión instalada o USB). Desde native\bin (desarrollo) queda apagado.
namespace actualizacion {

std::filesystem::path dir;
std::wstring version;      // de VERSION.txt
bool activo = false;
double intervaloHoras = 12;
ULONGLONG inicio = 0, ultimaRevision = 0, ultimaLectura = 0;
std::wstring lista;        // versión descargada y verificada, lista para instalar
HWND boton = nullptr;
HANDLE busqueda = nullptr; // revisión pedida con el botón «Buscar actualizaciones» (en curso)

inline std::vector<int> partes(const std::wstring& v) {
    std::vector<int> r; int n = 0; bool hay = false;
    for (wchar_t c : v) { if (c >= L'0' && c <= L'9') { n = n * 10 + (c - L'0'); hay = true; } else if (c == L'.') { r.push_back(n); n = 0; hay = false; } else break; }
    if (hay) r.push_back(n);
    return r;
}
inline bool masNueva(const std::wstring& a, const std::wstring& b) {  // a > b
    auto x = partes(a), y = partes(b);
    x.resize(std::max(x.size(), y.size())); y.resize(x.size());
    return x > y;
}

// Lanza el actualizador. Devuelve el proceso si se pide (para esperar su resultado); si no, lo suelta.
inline HANDLE lanza(const std::wstring& args, bool oculto, bool esperar = false) {
    wchar_t sys[MAX_PATH]; GetSystemDirectoryW(sys, MAX_PATH);
    std::wstring cmd = L"\"" + std::wstring(sys) + L"\\WindowsPowerShell\\v1.0\\powershell.exe\" -NoProfile " + (oculto ? L"-WindowStyle Hidden " : L"") +
                       L"-ExecutionPolicy Bypass -File \"" + (dir / L"actualizador.ps1").wstring() + L"\" " + args;
    STARTUPINFOW si{}; si.cb = sizeof(si); PROCESS_INFORMATION pi{};
    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, oculto ? CREATE_NO_WINDOW : CREATE_NEW_CONSOLE, nullptr, dir.c_str(), &si, &pi)) {
        log("ACTUALIZACION no se pudo lanzar el actualizador"); return nullptr;
    }
    CloseHandle(pi.hThread);
    if (esperar) return pi.hProcess;
    CloseHandle(pi.hProcess); return nullptr;
}

inline std::string leeTexto(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary); std::stringstream ss; ss << f.rdbuf(); std::string t = ss.str();
    if (t.size() >= 3 && (unsigned char)t[0] == 0xEF && (unsigned char)t[1] == 0xBB && (unsigned char)t[2] == 0xBF) t.erase(0, 3);
    return t;
}

// El botón siempre está (si las actualizaciones están activas): «Buscar actualizaciones», o resaltado
// «Actualización X · Instalar» cuando hay una versión nueva lista.
inline void textoBoton() {
    std::wstring t = busqueda ? L"⟳  Buscando actualizaciones…" : lista.empty() ? L"⟳  Buscar actualizaciones" : L"⟳  Actualización " + lista + L" · Instalar";
    SetWindowTextW(boton, t.c_str()); InvalidateRect(boton, nullptr, FALSE);
}
inline void muestra(const std::wstring& v) {
    if (v == lista) return;
    lista = v; textoBoton();
    if (!v.empty()) status(L"Hay una versión nueva de Play Dolby (" + v + L"), descargada y verificada. Pulsa «Instalar» arriba cuando el audio esté detenido.");
}

inline void leeEstado() {
    try {
        auto t = leeTexto(dataDir() / L"actualizacion" / L"estado.json");
        if (t.empty()) return;
        pli::Json j = pli::LectorJson(t).leer();
        const bool disp = j["disponible"].tipo == pli::Json::Bool && j["disponible"].b;
        std::wstring v = j["version"].tipo == pli::Json::Texto ? wide(j["version"].s) : L"";
        muestra(disp && masNueva(v, version) ? v : L"");
    } catch (...) {}  // estado a medio escribir: se vuelve a leer en 30 s
}

inline void inicia(HWND b) {
    boton = b;
    wchar_t exe[MAX_PATH]; GetModuleFileNameW(nullptr, exe, MAX_PATH);
    dir = std::filesystem::path(exe).parent_path();
    std::wifstream v(dir / L"VERSION.txt"); std::getline(v, version);
    while (!version.empty() && (version.back() == L'\r' || version.back() == L' ')) version.pop_back();
    theme::versionTexto = version.empty() ? L"versión de desarrollo" : L"versión " + version;
    activo = !version.empty() && std::filesystem::exists(dir / L"actualizador.ps1") && std::filesystem::exists(dir / L"actualizacion.json");
    if (activo) {
        try { auto j = pli::LectorJson(leeTexto(dir / L"actualizacion.json")).leer(); if (j["intervalo_horas"].tipo == pli::Json::Numero && j["intervalo_horas"].n >= 1) intervaloHoras = j["intervalo_horas"].n; } catch (...) {}
        log("ACTUALIZACION activa, version " + std::string(version.begin(), version.end()) + ", cada " + std::to_string(int(intervaloHoras)) + " h");
        textoBoton();
    } else ShowWindow(boton, SW_HIDE);  // versión de desarrollo (native\bin): sin actualizaciones
    inicio = GetTickCount64();
}

// Botón «Buscar actualizaciones»: revisa ahora y dice el resultado en la barra de estado.
inline void buscar() {
    if (busqueda) return;
    busqueda = lanza(L"-Revisar -DesdeApp", true, true);
    if (!busqueda) { status(L"No se pudo lanzar la búsqueda de actualizaciones."); return; }
    textoBoton();
    status(L"Buscando actualizaciones en GitHub…");
}
inline void terminaBusqueda() {
    CloseHandle(busqueda); busqueda = nullptr;
    leeEstado(); textoBoton();
    if (!lista.empty()) return;  // muestra() ya avisó de la versión nueva
    try {
        auto j = pli::LectorJson(leeTexto(dataDir() / L"actualizacion" / L"ultima.json")).leer();
        status(j["mensaje"].tipo == pli::Json::Texto ? wide(j["mensaje"].s) : L"Búsqueda terminada.");
    } catch (...) { status(L"Búsqueda terminada: revisa la bitácora del actualizador."); }
}

inline void tick() {  // timer de 100 ms
    if (!activo) return;
    if (busqueda && WaitForSingleObject(busqueda, 0) == WAIT_OBJECT_0) terminaBusqueda();
    const ULONGLONG ahora = GetTickCount64();
    const bool primera = ultimaRevision == 0 && ahora - inicio > 10000;
    if (primera || (ultimaRevision && ahora - ultimaRevision > ULONGLONG(intervaloHoras * 3600000.))) { lanza(L"-Revisar -DesdeApp", true); ultimaRevision = ahora; }
    if (ahora - ultimaLectura > 30000 || ultimaLectura == 0) { ultimaLectura = ahora; leeEstado(); }
}

inline void instalar() {
    if (lista.empty()) { buscar(); return; }
    if (engine.running()) { status(L"Detén el audio (■ Detener) para instalar la actualización. Nunca se instala con el puente en vivo."); return; }
    if (MessageBoxW(window, (L"¿Instalar Play Dolby " + lista + L" ahora?\n\nPlay Dolby se cerrará, se actualizará y se volverá a abrir. "
                             L"Tu ficha del domo se conserva y, si la nueva versión no arranca, regresa sola a la anterior.").c_str(),
                    L"Play Dolby · actualización", MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    log("ACTUALIZACION instalar " + std::string(lista.begin(), lista.end()));
    lanza(L"-Instalar -Reabrir", false);
    PostMessageW(window, WM_CLOSE, 0, 0);
}

}  // namespace actualizacion
