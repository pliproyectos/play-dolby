#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <cmath>

namespace cinematicOrbit {

inline void draw(HDC dc, RECT region, double seconds) {
    int w = region.right - region.left;
    int h = region.bottom - region.top;
    if (w <= 0 || h <= 0) return;

    Gdiplus::Graphics g(dc);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

    float cx = region.left + w * 0.5f;
    float cy = region.top + h * 0.5f;

    // Glowing subtle core (halo)
    float coreRadius = 42.0f + 3.0f * (float)sin(seconds * 1.8);
    Gdiplus::GraphicsPath corePath;
    corePath.AddEllipse(cx - coreRadius, cy - coreRadius, coreRadius * 2.0f, coreRadius * 2.0f);
    
    Gdiplus::PathGradientBrush corePgb(&corePath);
    Gdiplus::Color centerColor(220, 115, 180, 240);
    int surroundCount = 1;
    Gdiplus::Color surroundColor(0, 17, 17, 25);
    corePgb.SetCenterColor(centerColor);
    corePgb.SetSurroundColors(&surroundColor, &surroundCount);
    g.FillPath(&corePgb, &corePath);

    // Inner bright point
    Gdiplus::SolidBrush pointBrush(Gdiplus::Color(255, 230, 245, 255));
    g.FillEllipse(&pointBrush, cx - 4.0f, cy - 4.0f, 8.0f, 8.0f);

    // Multiple celestial orbital rings
    struct RingDef {
        float rx;
        float ry;
        float tilt;
        float speed;
        Gdiplus::Color ringColor;
        Gdiplus::Color planetColor;
        float planetSize;
    };

    RingDef rings[] = {
        { 90.0f,  35.0f, -0.35f,  0.8f, Gdiplus::Color(65, 77, 216, 185), Gdiplus::Color(240, 85, 248, 185), 5.5f },
        { 150.0f, 55.0f,  0.42f, -0.5f, Gdiplus::Color(55, 115, 157, 230), Gdiplus::Color(240, 140, 190, 255), 6.5f },
        { 210.0f, 72.0f, -0.18f,  0.3f, Gdiplus::Color(45, 175, 130, 245), Gdiplus::Color(240, 210, 170, 255), 5.0f },
        { 260.0f, 88.0f,  0.25f, -0.2f, Gdiplus::Color(35, 77, 216, 185), Gdiplus::Color(220, 77, 216, 185), 4.5f }
    };

    for (const auto& ring : rings) {
        Gdiplus::GraphicsState state = g.Save();
        g.TranslateTransform(cx, cy);
        g.RotateTransform(ring.tilt * 57.2957795f);

        // Draw orbital track
        Gdiplus::Pen orbitPen(ring.ringColor, 1.2f);
        g.DrawEllipse(&orbitPen, -ring.rx, -ring.ry, ring.rx * 2.0f, ring.ry * 2.0f);

        // Calculate planet position along ellipse
        double angle = seconds * ring.speed;
        float px = ring.rx * (float)cos(angle);
        float py = ring.ry * (float)sin(angle);

        // Planet glow
        Gdiplus::GraphicsPath pPath;
        float pGlow = ring.planetSize * 2.6f;
        pPath.AddEllipse(px - pGlow, py - pGlow, pGlow * 2.0f, pGlow * 2.0f);
        Gdiplus::PathGradientBrush pPgb(&pPath);
        pPgb.SetCenterColor(ring.planetColor);
        Gdiplus::Color pSurround(0, 0, 0, 0);
        int sc = 1;
        pPgb.SetSurroundColors(&pSurround, &sc);
        g.FillPath(&pPgb, &pPath);

        // Planet body
        Gdiplus::SolidBrush pBrush(Gdiplus::Color(255, 255, 255, 255));
        g.FillEllipse(&pBrush, px - ring.planetSize * 0.5f, py - ring.planetSize * 0.5f, ring.planetSize, ring.planetSize);

        g.Restore(state);
    }
}

} // namespace cinematicOrbit
