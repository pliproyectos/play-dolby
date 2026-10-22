#pragma once
#include <windows.h>
#include <gdiplus.h>
#include <cmath>
namespace cinematicOrbit {
inline void draw(HDC dc,RECT region,double seconds){
    auto brush=CreateSolidBrush(RGB(17,17,25));FillRect(dc,&region,brush);DeleteObject(brush);
    Gdiplus::Graphics g(dc);g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetClip(Gdiplus::Rect(region.left,region.top,region.right-region.left,region.bottom-region.top));
    const float cx=(region.left+region.right)*.5f,cy=(region.top+region.bottom)*.5f;
    Gdiplus::GraphicsPath halo;halo.AddEllipse(cx-225.f,cy-80.f,450.f,160.f);
    Gdiplus::PathGradientBrush glow(&halo);glow.SetCenterColor(Gdiplus::Color(85,140,52,240));
    Gdiplus::Color edge(0,17,17,25);int count=1;glow.SetSurroundColors(&edge,&count);g.FillPath(&glow,&halo);
    auto state=g.Save();g.TranslateTransform(cx,cy);g.RotateTransform(-15.f+float(std::sin(seconds*.12)*3));
    const float breath=1.f+float(std::sin(seconds*.55))*.015f;
    for(int i=0;i<7;++i){float rx=(104.f+i*15.f)*breath,ry=(23.f+i*4.5f)*breath;
        Gdiplus::Pen soft(Gdiplus::Color(22,170,80,255),7.f);g.DrawEllipse(&soft,-rx,-ry,rx*2,ry*2);
        Gdiplus::Pen line(Gdiplus::Color(205-i*15,193,135,255),1.7f);g.DrawEllipse(&line,-rx,-ry,rx*2,ry*2);
    }
    g.Restore(state);
    state=g.Save();g.TranslateTransform(cx,cy);g.RotateTransform(28.f+float(std::sin(seconds*.17)*3));
    Gdiplus::Pen crossing(Gdiplus::Color(225,229,208,255),2.f);g.DrawEllipse(&crossing,-140.f,-29.f,280.f,58.f);
    double t=seconds*.4;Gdiplus::SolidBrush point(Gdiplus::Color(255,245,234,255));
    g.FillEllipse(&point,float(140*std::cos(t))-3,float(29*std::sin(t))-3,6.f,6.f);g.Restore(state);
}
}
