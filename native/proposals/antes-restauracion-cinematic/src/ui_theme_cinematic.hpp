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
inline RECT orbitBounds = { 700, 76, 1190, 250 };

inline const COLORREF bgPaper       = RGB(17, 17, 25);     // Deep cinematic night #111119
inline const COLORREF cardBg        = RGB(24, 24, 35);     // Dark card surface #181823
inline const COLORREF slotBg        = RGB(19, 19, 29);     // Sub-card slot #13131D
inline const COLORREF cardBorder    = RGB(42, 42, 58);     // Subtle stroke #2A2A3A
inline const COLORREF textLight     = RGB(244, 241, 250);  // High contrast white #F4F1FA
inline const COLORREF textMuted     = RGB(150, 155, 175);  // Subtle secondary #969BAF
inline const COLORREF accentCyan    = RGB(77, 216, 185);   // Glow cyan / green #4DD8B9
inline const COLORREF accentBlue    = RGB(115, 157, 211);  // Star blue #739DD3
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

inline void loadLogo() {
    if (companyLogo != nullptr) return;
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, sizeof(pliLogoBytes));
    if (!mem) return;
    void* p = GlobalLock(mem);
    if (p) {
        CopyMemory(p, pliLogoBytes, sizeof(pliLogoBytes));
        GlobalUnlock(mem);
        IStream* stream = nullptr;
        if (CreateStreamOnHGlobal(mem, TRUE, &stream) == S_OK) {
            companyLogo = Gdiplus::Bitmap::FromStream(stream);
            stream->Release();
        }
    }
}

inline void releaseLogo() {
    if (companyLogo) {
        delete companyLogo;
        companyLogo = nullptr;
    }
}

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
    RECT r = { 0, 0, 1220, 840 };
    fill(dc, r, bgPaper);

    Gdiplus::Graphics g(dc);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);

    // Header Logo & Branding
    loadLogo();
    if (companyLogo) {
        g.DrawImage(companyLogo, 34, 28, 48, 48);
    }

    Gdiplus::FontFamily fontFamily(L"Segoe UI");
    Gdiplus::Font fontKicker(&fontFamily, 9.5f, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
    Gdiplus::Font fontHeadline(&fontFamily, 22.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
    Gdiplus::Font fontSubtitle(&fontFamily, 10.5f, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
    Gdiplus::Font fontCardHeader(&fontFamily, 10.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
    Gdiplus::Font fontCardDetail(&fontFamily, 9.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);

    Gdiplus::SolidBrush cyanBrush(color(accentCyan));
    Gdiplus::SolidBrush whiteBrush(color(textLight));
    Gdiplus::SolidBrush mutedBrush(color(textMuted));

    // Tagline / Kicker
    g.DrawString(L"PLI · PLANETARIO / AUDIO CONTROL", -1, &fontKicker, Gdiplus::PointF(92.0f, 26.0f), &cyanBrush);
    g.DrawString(L"El sonido. En su lugar.", -1, &fontHeadline, Gdiplus::PointF(90.0f, 44.0f), &whiteBrush);
    g.DrawString(L"De tu consola al planetario. Cada canal, bajo control.", -1, &fontSubtitle, Gdiplus::PointF(92.0f, 84.0f), &mutedBrush);

    // Live Badge
    const wchar_t* badgeText = isRunning ? L"●  EN VIVO" : L"○  DETENIDO";
    Gdiplus::SolidBrush badgeBrush(isRunning ? color(statusLive) : color(statusStopped));
    g.DrawString(badgeText, -1, &fontKicker, Gdiplus::PointF(92.0f, 108.0f), &badgeBrush);

    // Celestial Orbits in Header
    cinematicOrbit::draw(dc, orbitBounds, animationSeconds);

    // 01 CONECTA TU CONSOLA (Card)
    round(dc, { 34, 142, 594, 345 }, cardBg, 10, cardBorder);
    g.DrawString(L"01   CONECTA TU CONSOLA", -1, &fontCardHeader, Gdiplus::PointF(48.0f, 156.0f), &cyanBrush);
    g.DrawString(L"Entrada ASIO multicanal o WASAPI (Allen & Heath Qu-5 USB)", -1, &fontCardDetail, Gdiplus::PointF(48.0f, 178.0f), &mutedBrush);

    // 02 ELIGE TU RECEPTOR HDMI (Card)
    round(dc, { 622, 142, 1186, 345 }, cardBg, 10, cardBorder);
    g.DrawString(L"02   ELIGE TU RECEPTOR HDMI", -1, &fontCardHeader, Gdiplus::PointF(636.0f, 156.0f), &cyanBrush);
    g.DrawString(L"Salida de 8 canales hacia Dolby / Receptor HDMI envolvente", -1, &fontCardDetail, Gdiplus::PointF(636.0f, 178.0f), &mutedBrush);

    // 03 ASIGNA CADA SALIDA (Card)
    round(dc, { 34, 436, 1186, 725 }, cardBg, 10, cardBorder);
    g.DrawString(L"03   ASIGNA CADA SALIDA", -1, &fontCardHeader, Gdiplus::PointF(48.0f, 448.0f), &cyanBrush);
    g.DrawString(L"Prueba cada altavoz antes de iniciar · Tonos de 2 s a −36 dBFS", -1, &fontCardDetail, Gdiplus::PointF(48.0f, 468.0f), &mutedBrush);

    const wchar_t* codes[] = { L"FL", L"FR", L"C", L"LFE", L"BL", L"BR", L"SL", L"SR" };
    const wchar_t* names[] = { L"Frontal izq", L"Frontal der", L"Centro", L"Graves", L"Trasero izq", L"Trasero der", L"Lateral izq", L"Lateral der" };

    for (int i = 0; i < 8; ++i) {
        int x = 46 + i * 141;
        round(dc, { x, 492, x + 130, 715 }, slotBg, 8, cardBorder);
        text(dc, codes[i], { x + 10, 502, x + 120, 532 }, channelFont, textLight);
        text(dc, names[i], { x + 10, 534, x + 120, 560 }, smallFont, textMuted);
    }

    // 04 INICIA LA EXPERIENCIA (Card)
    round(dc, { 34, 735, 1186, 792 }, cardBg, 10, cardBorder);
    g.DrawString(L"04   INICIA LA EXPERIENCIA", -1, &fontCardHeader, Gdiplus::PointF(48.0f, 752.0f), &cyanBrush);
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
