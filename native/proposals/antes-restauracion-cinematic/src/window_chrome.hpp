#pragma once
#include <windows.h>
#include <cstring>
#include "app_resources.h"

namespace chrome {
inline HICON icon(bool small) {
    // LR_SHARED keeps resource icons owned by Windows for process lifetime.
    return static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr),MAKEINTRESOURCEW(IDI_PLI_APP),IMAGE_ICON,
        GetSystemMetrics(small?SM_CXSMICON:SM_CXICON),GetSystemMetrics(small?SM_CYSMICON:SM_CYICON),LR_SHARED));
}
inline void setIcons(HWND window) {
    SendMessageW(window,WM_SETICON,ICON_BIG,reinterpret_cast<LPARAM>(icon(false)));
    SendMessageW(window,WM_SETICON,ICON_SMALL,reinterpret_cast<LPARAM>(icon(true)));
}
// System DWM preserves native dragging, minimize, close and taskbar behavior.
inline void apply(HWND window) {
    static HMODULE dwm=LoadLibraryExW(L"dwmapi.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
    using SetAttribute=HRESULT (WINAPI*)(HWND,DWORD,LPCVOID,DWORD);
    static SetAttribute set=[] {
        SetAttribute fn=nullptr;
        auto address=dwm?GetProcAddress(dwm,"DwmSetWindowAttribute"):nullptr;
        static_assert(sizeof(fn)==sizeof(address));
        std::memcpy(&fn,&address,sizeof(fn));return fn;
    }();
    if(!set)return;
    HIGHCONTRASTW contrast{};contrast.cbSize=sizeof(contrast);
    SystemParametersInfoW(SPI_GETHIGHCONTRAST,sizeof(contrast),&contrast,0);
    BOOL dark=!(contrast.dwFlags&HCF_HIGHCONTRASTON);
    set(window,20,&dark,sizeof(dark)); // DWMWA_USE_IMMERSIVE_DARK_MODE
    COLORREF caption=dark?RGB(17,17,25):0xFFFFFFFF;
    COLORREF border=dark?RGB(17,17,25):0xFFFFFFFF;
    COLORREF text=dark?RGB(244,241,250):0xFFFFFFFF;
    // Windows 11 colors; older versions safely ignore unsupported attributes.
    set(window,34,&border,sizeof(border));
    set(window,35,&caption,sizeof(caption));
    set(window,36,&text,sizeof(text));
}
}
