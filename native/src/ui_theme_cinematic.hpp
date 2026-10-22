#pragma once
#include <windows.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <string>
#include <cmath>
#include <algorithm>
#include "pli_logo_data.hpp"
#include "cinematic_orbits.hpp"

namespace theme {

inline double animationSeconds = 0.0;
inline RECT orbitBounds = { 700, 76, 962, 278 };
// Dynamic Space (PLI Espacial fase 3b): si hay pintor, dibuja el domo en vivo en orbitBounds y las
// barras de las tarjetas. Todo lo que pinta cae dentro de dynamicBounds; la vista solo redibuja eso.
inline void (*dynamicPainter)(HDC) = nullptr;
inline RECT dynamicBounds = { 30, 76, 1190, 500 };

inline const COLORREF bgPaper       = RGB(17, 17, 25);     // Deep cinematic night #111119
inline const COLORREF cardBg        = RGB(24, 24, 35);     // Dark card surface #181823
inline const COLORREF slotBg        = RGB(19, 19, 29);     // Sub-card slot #13131D
inline const COLORREF cardBorder    = RGB(42, 42, 58);     // Subtle stroke #2A2A3A
inline const COLORREF textLight     = RGB(244, 241, 250);  // High contrast white #F4F1FA
inline const COLORREF textMuted     = RGB(150, 155, 175);  // Subtle secondary #969BAF
inline const COLORREF accentCyan    = RGB(193, 135, 255);   // Glow cyan / green #4DD8B9
inline const COLORREF accentBlue    = RGB(168, 53, 247);  // Star blue #739DD3
inline const COLORREF statusLive    = RGB(85, 248, 185);   // Live green #55F8B9
inline const COLORREF statusStopped = RGB(160, 165, 180);  // Idle grey

// Aliases for compatibility with channel_tester.hpp
inline const COLORREF paper   = bgPaper;
inline const COLORREF surface = cardBg;
inline const COLORREF ink     = textLight;
inline const COLORREF muted   = textMuted;
inline const COLORREF line    = cardBorder;
inline const COLORREF accent  = accentCyan;
inline const COLORREF blue    = accentBlue;

inline HBRUSH background = CreateSolidBrush(bgPaper);
inline HBRUSH cardBrush  = CreateSolidBrush(cardBg);
inline HBRUSH slotBrush  = CreateSolidBrush(slotBg);
inline HBRUSH white      = CreateSolidBrush(RGB(255, 255, 255));

inline Gdiplus::Bitmap* companyLogo = nullptr;
inline IStream* logoStream = nullptr;
inline void loadLogo() {
    if(companyLogo)return;
    HGLOBAL mem=GlobalAlloc(GMEM_MOVEABLE,sizeof(pliLogoBytes));
    if(!mem)return;
    void* bytes=GlobalLock(mem);
    if(!bytes){GlobalFree(mem);return;}
    CopyMemory(bytes,pliLogoBytes,sizeof(pliLogoBytes));GlobalUnlock(mem);
    if(FAILED(CreateStreamOnHGlobal(mem,TRUE,&logoStream))){GlobalFree(mem);return;}
    companyLogo=Gdiplus::Bitmap::FromStream(logoStream);
    if(!companyLogo||companyLogo->GetLastStatus()!=Gdiplus::Ok){delete companyLogo;companyLogo=nullptr;logoStream->Release();logoStream=nullptr;}
}
inline void releaseLogo(){delete companyLogo;companyLogo=nullptr;if(logoStream){logoStream->Release();logoStream=nullptr;}}

class PaintBuffer {
    HDC target_, memory_;
    HBITMAP bitmap_;
    HGDIOBJ previous_;
    RECT bounds_;
public:
    PaintBuffer(HDC target, RECT bounds) : target_(target), bounds_(bounds) {
        memory_ = CreateCompatibleDC(target);
        bitmap_ = CreateCompatibleBitmap(target, bounds.right, bounds.bottom);
        previous_ = SelectObject(memory_, bitmap_);
    }
    PaintBuffer(const PaintBuffer&) = delete;
    PaintBuffer& operator=(const PaintBuffer&) = delete;
    ~PaintBuffer() {
        SelectObject(memory_, previous_);
        DeleteObject(bitmap_);
        DeleteDC(memory_);
    }
    HDC dc() const { return memory_; }
    void present() {
        BitBlt(target_, 0, 0, bounds_.right, bounds_.bottom, memory_, 0, 0, SRCCOPY);
    }
};

inline Gdiplus::Color color(COLORREF c) {
    return Gdiplus::Color(255, GetRValue(c), GetGValue(c), GetBValue(c));
}

inline void fill(HDC dc, RECT r, COLORREF c) {
    auto b = CreateSolidBrush(c);
    FillRect(dc, &r, b);
    DeleteObject(b);
}

inline void round(HDC dc, RECT r, COLORREF c, int radius = 8, COLORREF border = CLR_INVALID) {
    Gdiplus::Graphics g(dc);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    Gdiplus::GraphicsPath p;
    float x = float(r.left), y = float(r.top);
    float w = float(r.right - r.left - 1), h = float(r.bottom - r.top - 1);
    float d = float(radius * 2);
    p.AddArc(x, y, d, d, 180, 90);
    p.AddArc(x + w - d, y, d, d, 270, 90);
    p.AddArc(x + w - d, y + h - d, d, d, 0, 90);
    p.AddArc(x, y + h - d, d, d, 90, 90);
    p.CloseFigure();
    Gdiplus::SolidBrush brush(color(c));
    g.FillPath(&brush, &p);
    if (border != CLR_INVALID) {
        Gdiplus::Pen pen(color(border));
        g.DrawPath(&pen, &p);
    }
}

inline void text(HDC dc, const wchar_t* s, RECT r, HFONT f, COLORREF c, UINT flags = DT_LEFT | DT_WORDBREAK) {
    auto old = SelectObject(dc, f);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, c);
    DrawTextW(dc, s, -1, &r, flags);
    SelectObject(dc, old);
}

inline void paint(HDC dc, bool isRunning, HFONT font, HFONT titleFont, HFONT smallFont, HFONT channelFont) {
    fill(dc,{0,0,1220,840},paper);
    fill(dc,{0,0,1220,70},RGB(13,13,20));
    Gdiplus::Graphics g(dc);g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    loadLogo();
    if(companyLogo){
        const float white[5][5]={{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,1,0},{1,1,1,0,1}};
        Gdiplus::ColorMatrix matrix;CopyMemory(matrix.m,white,sizeof(white));
        Gdiplus::ImageAttributes attrs;attrs.SetColorMatrix(&matrix);
        int height=int(160.0*companyLogo->GetHeight()/companyLogo->GetWidth());
        g.DrawImage(companyLogo,Gdiplus::Rect(30,(70-height)/2,160,height),0,0,companyLogo->GetWidth(),companyLogo->GetHeight(),Gdiplus::UnitPixel,&attrs);
    }
    text(dc,L"PLAY DOLBY / AUDIO CONTROL",{220,24,700,50},smallFont,ink);
    round(dc,{1024,18,1190,53},isRunning?RGB(38,66,60):RGB(46,41,62),18);
    text(dc,isRunning?L"●  EN VIVO":L"○  DETENIDO",{1036,18,1180,53},smallFont,isRunning?statusLive:ink,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    text(dc,L"EL ESPACIO ES TUYO",{30,92,600,113},smallFont,accent);
    text(dc,L"El sonido.",{30,120,650,177},titleFont,ink,DT_SINGLELINE);
    text(dc,L"En su lugar.",{30,179,650,236},titleFont,ink,DT_SINGLELINE);
    text(dc,L"De tu consola al planetario. Cada canal, bajo control.",{32,244,690,268},font,muted,DT_SINGLELINE);
    if(!dynamicPainter)cinematicOrbit::draw(dc,orbitBounds,animationSeconds);
    round(dc,{30,280,600,367},surface,12,line);
    round(dc,{618,280,1190,367},surface,12,line);
    text(dc,L"01   CONECTA TU CONSOLA",{48,293,580,315},smallFont,ink);
    text(dc,L"02   ELIGE TU RECEPTOR HDMI",{636,293,1172,315},smallFont,ink);
    text(dc,L"03   ASIGNA CADA SALIDA",{30,441,680,473},channelFont,ink,DT_SINGLELINE);
    text(dc,L"Prueba cada altavoz antes de iniciar · Tonos de 2 s a −36 dBFS",{720,447,1190,470},smallFont,muted,DT_RIGHT|DT_SINGLELINE);
    const wchar_t* codes[]={L"FL",L"FR",L"C",L"LFE",L"BL",L"BR",L"SL",L"SR"};
    const wchar_t* names[]={L"Frontal izquierdo",L"Frontal derecho",L"Centro",L"Graves",L"Trasero izquierdo",L"Trasero derecho",L"Lateral izquierdo",L"Lateral derecho"};
    for(int c=0;c<8;++c){int x=30+c*146;
        round(dc,{x,480,x+138,718},surface,12,line);
        round(dc,{x+12,493,x+43,496},accent,1);
        text(dc,codes[c],{x+12,510,x+126,541},channelFont,ink,DT_SINGLELINE);
        text(dc,names[c],{x+12,546,x+131,566},smallFont,muted,DT_SINGLELINE);
    }
    round(dc,{30,733,1190,791},surface,12,line);
    text(dc,L"04   INICIA LA EXPERIENCIA",{48,752,700,774},smallFont,ink,DT_SINGLELINE);
    if(dynamicPainter)dynamicPainter(dc);
}

} // namespace theme

inline LRESULT CALLBACK comboProc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR) {
    if (m == WM_PAINT) {
        PAINTSTRUCT ps;
        auto dc = BeginPaint(h, &ps);
        RECT r;
        GetClientRect(h, &r);
        bool enabled = IsWindowEnabled(h);
        bool focus = GetFocus() == h;
        theme::fill(dc, r, theme::bgPaper);
        theme::round(dc, r, enabled ? theme::cardBg : RGB(20, 20, 28), 6, focus ? theme::accentCyan : theme::cardBorder);
        int selected = int(SendMessage(h, CB_GETCURSEL, 0, 0));
        std::wstring value;
        if (selected >= 0) {
            int n = int(SendMessage(h, CB_GETLBTEXTLEN, selected, 0));
            value.resize(n + 1);
            SendMessage(h, CB_GETLBTEXT, selected, (LPARAM)value.data());
            value.resize(n);
        } else {
            value = GetDlgCtrlID(h) == 1 ? L"Seleccionar consola…" : L"Seleccionar receptor…";
        }
        RECT label = { 10, 0, r.right - 25, r.bottom };
        extern HFONT font, smallFont;
        theme::text(dc, value.c_str(), label, GetDlgCtrlID(h) >= 30 ? smallFont : font, enabled ? theme::textLight : theme::textMuted, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        theme::text(dc, L"⌄", { r.right - 24, 0, r.right - 5, r.bottom }, font, theme::textMuted, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        EndPaint(h, &ps);
        return 0;
    }
    auto result = DefSubclassProc(h, m, w, l);
    if (m == WM_SETFOCUS || m == WM_KILLFOCUS || m == WM_ENABLE || m == CB_SETCURSEL) {
        InvalidateRect(h, nullptr, FALSE);
    }
    return result;
}

namespace meter {

struct Reading {
    int segments = 0, decibels = 0;
    bool silent = true;
    bool operator==(const Reading& other) const {
        return segments == other.segments && decibels == other.decibels && silent == other.silent;
    }
};

inline Reading displayed[8];

inline Reading sample(int channel) {
    const float peak = ::engine.core.peak[channel].load();
    const float db = 20 * std::log10(std::max(1e-6f, peak));
    return { int(std::clamp((db + 60) / 60, 0.f, 1.f) * 12), int(db), peak < 1e-6f };
}

inline void update(HWND h) {
    const int channel = GetDlgCtrlID(h) - 200;
    if (channel < 0 || channel >= 8) return;
    const auto next = sample(channel);
    if (next == displayed[channel]) return;
    displayed[channel] = next;
    InvalidateRect(h, nullptr, FALSE);
}

inline void draw(HDC dc, RECT r, const Reading& reading) {
    theme::fill(dc, r, theme::slotBg);
    for (int i = 0; i < 12; ++i) {
        int y = 48 - i * 4;
        COLORREF segColor = i < reading.segments ? (i > 10 ? RGB(235, 87, 87) : i > 8 ? theme::accentCyan : theme::accentBlue) : RGB(35, 35, 48);
        theme::round(dc, { 10, y, 38, y + 3 }, segColor, 1);
    }
    theme::text(dc, L"0\n−30\n−60", { 44, 2, 80, 52 }, ::smallFont, theme::textMuted);
    auto value = reading.silent ? std::wstring(L"−∞ dB") : std::to_wstring(reading.decibels) + L" dB";
    theme::text(dc, value.c_str(), { 0, 50, r.right, 66 }, ::smallFont, reading.silent ? theme::textMuted : theme::accentCyan, DT_CENTER | DT_SINGLELINE);
}

} // namespace meter

inline LRESULT CALLBACK meterProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_ERASEBKGND) return 1;
    if (m == WM_PAINT) {
        PAINTSTRUCT ps;
        auto target = BeginPaint(h, &ps);
        RECT r;
        GetClientRect(h, &r);
        {
            theme::PaintBuffer frame(target, r);
            meter::draw(frame.dc(), r, meter::displayed[GetDlgCtrlID(h) - 200]);
            frame.present();
        }
        EndPaint(h, &ps);
        return 0;
    }
    return DefWindowProc(h, m, w, l);
}
