#pragma once
// Fuente archivo en la ventana principal: opciones «▶ Archivo WAV…» y «▶ Carpeta de capas…» en
// «01 Conecta tu consola». Con una carpeta de capas71 se configuran solos los objetos de PLI Espacial.
#include <shobjidl.h>

namespace fuenteUI {

int anterior = -1;  // selección previa de la lista, para volver si se cancela

// Diálogo de Windows para elegir un WAV o una carpeta. Devuelve vacío si se cancela.
std::filesystem::path elige(bool carpeta) {
    IFileOpenDialog* d = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&d)))) return {};
    DWORD op = 0; d->GetOptions(&op);
    d->SetOptions(op | FOS_FORCEFILESYSTEM | (carpeta ? FOS_PICKFOLDERS : 0));
    if (carpeta) d->SetTitle(L"Elige la carpeta «capas» que creó capas71");
    else { COMDLG_FILTERSPEC f[] = { { L"Audio WAV", L"*.wav" } }; d->SetFileTypes(1, f); d->SetTitle(L"Elige el WAV que hará de consola"); }
    std::filesystem::path r;
    if (SUCCEEDED(d->Show(window))) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(d->GetResult(&item))) {
            PWSTR p = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &p))) { r = p; CoTaskMemFree(p); }
            item->Release();
        }
    }
    d->Release();
    return r;
}

std::wstring tiempo(double s) { int t = int(s); wchar_t b[16]; swprintf(b, 16, L"%d:%02d", t / 60, t % 60); return b; }

// Pone en la lista la entrada «♪ archivo» (reemplaza la anterior) y la selecciona.
void muestraArchivo() {
    int n = int(SendMessage(inputBox, CB_GETCOUNT, 0, 0));
    for (int i = n - 1; i >= 0; --i) if (int(SendMessage(inputBox, CB_GETITEMDATA, i, 0)) == IdArchivo) SendMessage(inputBox, CB_DELETESTRING, i, 0);
    if (!engine.archivo.cuadros()) return;
    auto s = L"♪  " + wide(engine.archivo.nombre) + L" / " + std::to_wstring(engine.archivo.canales) + L" canales · " +
             std::to_wstring(engine.archivo.rate / 1000) + L" kHz · " + tiempo(double(engine.archivo.cuadros()) / engine.archivo.rate);
    int ix = int(SendMessage(inputBox, CB_INSERTSTRING, 2, (LPARAM)s.c_str()));
    SendMessage(inputBox, CB_SETITEMDATA, ix, IdArchivo);
    SendMessage(inputBox, CB_SETCURSEL, ix, 0);
}

// Opciones fijas al inicio de la lista (las llama refresh()).
void agregaOpciones() {
    int a = int(SendMessage(inputBox, CB_INSERTSTRING, 0, (LPARAM)L"▶  Archivo WAV… (hace de consola)"));
    SendMessage(inputBox, CB_SETITEMDATA, a, IdElegirArchivo);
    int c = int(SendMessage(inputBox, CB_INSERTSTRING, 1, (LPARAM)L"▶  Carpeta de capas… (capas71 → PLI Espacial)"));
    SendMessage(inputBox, CB_SETITEMDATA, c, IdElegirCapas);
    if (engine.archivo.cuadros()) { muestraArchivo(); SendMessage(inputBox, CB_SETCURSEL, -1, 0); }
}

// Capas → objetos: voz, colchón, brillos, percusión, bajo y bombo con la coreografía de capas71.
void configuraCapas() {
    struct Conf { int entrada; float ancho; int mov; float azimut; int nivel; };
    const Conf c[6] = { { 0, 0, 10, 0, 2 },     // voz: estéreo sin apertura (= mono), vaivén ±25°, −6 dB
                        { 2, 90, 1, 0, 0 },     // colchón: abierto ±90°, gira ↻ 1,5 v/min
                        { 4, 60, 5, 180, 0 },   // brillos: ±60°, nacen atrás, giran ↺ 3 v/min
                        { 6, 45, 11, 0, 0 },    // percusión: ±45°, gira ↻ 5 v/min
                        { 8, 30, 0, 0, 0 },     // bajo: fijo al frente, L en FL y R en FR
                        { 10, 30, 0, 0, 0 } };  // bombo: igual
    for (int k = 0; k < pli::MaxObjetos; ++k) {
        auto& o = engine.core.espacio.obj[k];
        if (k >= 6) { o.entrada = -1; continue; }
        o.estereo = true; o.ancho = c[k].ancho; o.nivel = std::pow(10.f, espacialUI::nivelesDb[c[k].nivel] / 20);
        espacialUI::fijaObjeto(k, c[k].mov, c[k].azimut);
        o.entrada = c[k].entrada;
    }
    for (int r = 0; r < 8; ++r) SendMessage(routes[r], CB_SETCURSEL, 0, 0);  // todo por objetos: salidas directas en Silencio
    SendMessage(profileBox, CB_SETCURSEL, 2, 0);                             // 7.1
    if (!engine.core.espacio.activo) espacialUI::alterna();
    espacialUI::sincronizaFilas();
    enableControls(true);
}

// Al cambiar la selección de «01 Conecta tu consola».
void seleccion() {
    int i = int(SendMessage(inputBox, CB_GETCURSEL, 0, 0));
    int id = i < 0 ? -1 : int(SendMessage(inputBox, CB_GETITEMDATA, i, 0));
    if (id != IdElegirArchivo && id != IdElegirCapas) { anterior = i; return; }
    SendMessage(inputBox, CB_SETCURSEL, anterior, 0);
    if (engine.running()) { status(L"Detén el puente antes de cambiar de archivo."); return; }
    auto ruta = elige(id == IdElegirCapas);
    if (ruta.empty()) return;
    try {
        engine.archivo = id == IdElegirCapas ? fuente::leeCapas(ruta) : fuente::leeWav(ruta);
        muestraArchivo(); anterior = int(SendMessage(inputBox, CB_GETCURSEL, 0, 0));
        log("ARCHIVO " + engine.archivo.nombre + " canales=" + std::to_string(engine.archivo.canales));
        if (id == IdElegirCapas) {
            configuraCapas();
            status(L"Capas cargadas: 6 objetos de PLI Espacial (voz, colchón, brillos, percusión, bajo, bombo) y salidas directas en Silencio. "
                   L"Elige el receptor y pulsa Iniciar. Empieza con ganancia baja.");
        } else if (engine.archivo.canales == 8) {
            // Una mezcla 7.1 ya hecha (pruebas, música 7.1): suena tal cual, canal a canal, sin objetos encima.
            for (int r = 0; r < 8; ++r) SendMessage(routes[r], CB_SETCURSEL, r + 1, 0);
            SendMessage(profileBox, CB_SETCURSEL, 2, 0);
            if (engine.core.espacio.activo) espacialUI::alterna();
            enableControls(true);
            status(L"WAV 7.1 cargado: ruteo directo USB 1→FL … USB 8→SR, perfil 7.1 y PLI Espacial apagado para que suene tal cual. "
                   L"Elige el receptor y pulsa Iniciar. Se repite al terminar.");
        } else {
            status(L"Archivo cargado: sus canales son USB 1 a " + std::to_wstring(engine.archivo.canales) +
                   L". Asígnalos en el paso 03 o como objetos de PLI Espacial y pulsa Iniciar. Se repite al terminar.");
        }
    } catch (const std::exception& e) {
        status(L"No se cargó: " + wide(e.what()));
    }
    tester::sync();
}

}  // namespace fuenteUI
