#pragma once
#include <memory>
namespace spatialVisual {
std::unique_ptr<pli::Disposicion> domo;
float brillo[pli::NumCanales]{};
void initialize() {
 domo=std::make_unique<pli::Disposicion>(std::vector<pli::Bocina>{{"FL",0,-30},{"FR",1,30},{"C",2,0},{"BL",4,-150},{"BR",5,150},{"SL",6,-90},{"SR",7,90}});
 wchar_t path[MAX_PATH]; GetModuleFileNameW(nullptr,path,MAX_PATH);
 try { domo=std::make_unique<pli::Disposicion>(pli::Disposicion::desdeFicha((std::filesystem::path(path).parent_path()/L"domo.json").u8string())); }
 catch(const std::exception&) { /* Default speaker layout remains available. */ }
}
void pintaPrincipal(HDC dc){
 const RECT r=theme::orbitBounds;const bool on=engine.running();const double t=theme::animationSeconds;
 theme::fill(dc,r,theme::paper);
 Gdiplus::Graphics g(dc);g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
 g.SetClip(Gdiplus::Rect(r.left,r.top,r.right-r.left,r.bottom-r.top));
 const float x0=(r.left+r.right)*.5f,y0=(r.top+r.bottom)*.5f+8,R=78.f*(on?1.f:1.f+float(std::sin(t*.55))*.015f);
 Gdiplus::GraphicsPath halo;halo.AddEllipse(x0-130.f,y0-112.f,260.f,224.f);
 Gdiplus::PathGradientBrush glow(&halo);glow.SetCenterColor(Gdiplus::Color(on?100:50,140,52,240));
 Gdiplus::Color edge(0,17,17,25);int count=1;glow.SetSurroundColors(&edge,&count);g.FillPath(&glow,&halo);
 Gdiplus::Pen aro(Gdiplus::Color(on?215:120,193,135,255),1.7f),guia(Gdiplus::Color(on?70:35,193,135,255),1.f);
 g.DrawEllipse(&aro,x0-R,y0-R,2*R,2*R);g.DrawEllipse(&guia,x0-R*.72f,y0-R*.72f,R*1.44f,R*1.44f);
 theme::text(dc,on?L"PLI ESPACIAL · SALIDA":L"PLI ESPACIAL · DOMO",{r.left,r.top+2,r.right,r.top+18},::smallFont,on?theme::statusLive:theme::muted,DT_CENTER|DT_SINGLELINE);
 if(domo)for(auto& b:domo->bocinas()){
  double a=b.azimut*pli::Pi/180;int x=int(x0+R*std::sin(a)),y=int(y0-R*std::cos(a));
  theme::round(dc,{x-15,y-9,x+15,y+9},RGB(32,29,46),5,on?theme::accent:theme::line);
  theme::text(dc,wide(b.id).c_str(),{x-15,y-9,x+15,y+9},::smallFont,on?theme::ink:theme::muted,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
 }
 // Punto decorativo del domo; no representa una fuente de audio.
 double a=t*.35;Gdiplus::SolidBrush punto(Gdiplus::Color(200,245,234,255));
 g.FillEllipse(&punto,float(x0+R*std::sin(a))-3,float(y0-R*std::cos(a))-3,6.f,6.f);

 g.ResetClip();
 // Barras de las tarjetas FL…SR: cuánto le manda el paneo a cada salida (potencia, suavizada).
 float objetivo[pli::NumCanales]{};
 for(int c=0;c<pli::NumCanales;++c) objetivo[c]=engine.core.peak[c].load();
 for(int c=0;c<pli::NumCanales;++c){
  brillo[c]=brillo[c]*.55f+std::min(1.f,objetivo[c])*.45f;
  const int x=30+c*146;
  theme::fill(dc,{x+12,489,x+128,500},theme::surface);
  if(!on){theme::round(dc,{x+12,493,x+43,496},theme::accent,1);continue;}
  theme::round(dc,{x+12,493,x+126,497},RGB(38,36,52),2);
  const int w=int(114*brillo[c]);
  if(w>2)theme::round(dc,{x+12,492,x+12+w,498},brillo[c]>.6f?theme::statusLive:theme::accent,2);
 }
}

}
