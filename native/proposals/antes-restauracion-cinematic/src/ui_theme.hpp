#pragma once
namespace theme {
constexpr COLORREF blue=RGB(0,75,151),accent=RGB(115,157,211),paper=RGB(245,248,252);
constexpr COLORREF ink=RGB(24,44,70),muted=RGB(105,122,145),line=RGB(224,232,242);
HBRUSH background=CreateSolidBrush(paper),white=CreateSolidBrush(RGB(255,255,255));
// Compose a complete frame offscreen before exposing it to the window.
class PaintBuffer {
 HDC target_,memory_;HBITMAP bitmap_;HGDIOBJ previous_;RECT bounds_;
public:
 PaintBuffer(HDC target,RECT bounds):target_(target),bounds_(bounds){
  memory_=CreateCompatibleDC(target);bitmap_=CreateCompatibleBitmap(target,bounds.right,bounds.bottom);
  previous_=SelectObject(memory_,bitmap_);
 }
 PaintBuffer(const PaintBuffer&)=delete;
 PaintBuffer& operator=(const PaintBuffer&)=delete;
 ~PaintBuffer(){SelectObject(memory_,previous_);DeleteObject(bitmap_);DeleteDC(memory_);}
 HDC dc()const{return memory_;}
 void present(){BitBlt(target_,0,0,bounds_.right,bounds_.bottom,memory_,0,0,SRCCOPY);}
};
Gdiplus::Color color(COLORREF c){return Gdiplus::Color(255,GetRValue(c),GetGValue(c),GetBValue(c));}
void fill(HDC dc,RECT r,COLORREF c){auto b=CreateSolidBrush(c);FillRect(dc,&r,b);DeleteObject(b);}
void round(HDC dc,RECT r,COLORREF c,int radius=12,COLORREF border=CLR_INVALID){
 Gdiplus::Graphics g(dc);g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
 Gdiplus::GraphicsPath p;float x=float(r.left),y=float(r.top),w=float(r.right-r.left-1),h=float(r.bottom-r.top-1),d=float(radius*2);
 p.AddArc(x,y,d,d,180,90);p.AddArc(x+w-d,y,d,d,270,90);p.AddArc(x+w-d,y+h-d,d,d,0,90);p.AddArc(x,y+h-d,d,d,90,90);p.CloseFigure();
 Gdiplus::SolidBrush brush(color(c));g.FillPath(&brush,&p);
 if(border!=CLR_INVALID){Gdiplus::Pen pen(color(border));g.DrawPath(&pen,&p);}
}
void text(HDC dc,const wchar_t* s,RECT r,HFONT f,COLORREF c,UINT flags=DT_LEFT|DT_WORDBREAK){
 auto old=SelectObject(dc,f);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,c);DrawTextW(dc,s,-1,&r,flags);SelectObject(dc,old);
}
void paint(HDC dc){
 RECT r;GetClientRect(window,&r);fill(dc,r,paper);
 fill(dc,{0,0,230,840},blue);fill(dc,{230,0,1220,144},blue);
 round(dc,{24,25,77,78},RGB(255,255,255),14);
 text(dc,L"pli",{33,27,76,72},titleFont,blue);
 text(dc,L"PROYECTOS",{91,34,220,58},font,RGB(255,255,255));
 text(dc,L"AUDIO SYSTEMS",{92,59,220,78},smallFont,RGB(183,211,245));
 round(dc,{20,120,210,166},RGB(49,101,175),12);
 text(dc,L"◉   Control de audio",{36,133,202,158},font,RGB(255,255,255));
 text(dc,L"CADENA DE SEÑAL",{28,216,215,240},smallFont,RGB(183,211,245));
 round(dc,{26,262,204,338},RGB(12,86,162),12);
 text(dc,L"01   Consola USB",{42,278,195,304},font,RGB(255,255,255));
 text(dc,L"Entrada · ASIO / WASAPI",{42,307,202,330},smallFont,RGB(198,220,248));
 text(dc,L"↓",{103,348,145,387},titleFont,accent);
 round(dc,{26,394,204,470},RGB(12,86,162),12);
 text(dc,L"02   Bridge",{42,410,195,436},font,RGB(255,255,255));
 text(dc,L"Ruteo · 48 kHz",{42,439,202,462},smallFont,RGB(198,220,248));
 text(dc,L"↓",{103,480,145,519},titleFont,accent);
 round(dc,{26,526,204,602},RGB(12,86,162),12);
 text(dc,L"03   Receptor HDMI",{42,542,202,568},font,RGB(255,255,255));
 text(dc,L"Salida · Hasta 7.1",{42,571,202,594},smallFont,RGB(198,220,248));
 text(dc,L"INTEGRACIÓN AUDIOVISUAL",{28,749,218,778},smallFont,RGB(183,211,245));
 text(dc,L"Planetario / PLI",{28,780,215,810},font,RGB(255,255,255));
 text(dc,L"ESPACIO DE TRABAJO",{264,22,700,45},smallFont,RGB(190,216,248));
 text(dc,L"Planetario Bridge",{262,49,950,94},titleFont,RGB(255,255,255));
 text(dc,L"Conecta, asigna y controla cada salida.",{264,102,950,132},font,RGB(213,230,252));
 round(dc,{1034,53,1188,92},RGB(34,100,171),18);
 text(dc,engine.running()?L"●  EN VIVO":L"○  DETENIDO",{1049,64,1180,88},smallFont,RGB(255,255,255));
 round(dc,{260,164,716,280},RGB(255,255,255),16,line);
 round(dc,{734,164,1190,280},RGB(255,255,255),16,line);
 text(dc,L"01  ENTRADA DE AUDIO",{280,182,690,205},smallFont,muted);
 text(dc,L"02  SALIDA HDMI",{754,182,1168,205},smallFont,muted);
 text(dc,L"Ruteo de canales",{262,367,680,401},channelFont,ink);
 text(dc,L"Asigna una entrada USB a cada altavoz.",{264,401,800,425},smallFont,muted);
 text(dc,L"TONO DE PRUEBA  −36 dBFS · 2 s",{943,395,1190,422},smallFont,muted);
 const wchar_t* codes[]={L"FL",L"FR",L"C",L"LFE",L"BL",L"BR",L"SL",L"SR"};
 const wchar_t* names[]={L"Frontal\nizquierdo",L"Frontal\nderecho",L"Centro",L"Graves",L"Trasero\nizquierdo",L"Trasero\nderecho",L"Lateral\nizquierdo",L"Lateral\nderecho"};
 for(int i=0;i<8;++i){
  int x=260+i*117;bool active=IsWindowEnabled(routes[i]);
  round(dc,{x,437,x+110,690},active?RGB(255,255,255):RGB(239,243,248),14,line);
  text(dc,codes[i],{x+14,451,x+100,486},channelFont,active?blue:muted);
  text(dc,names[i],{x+14,490,x+106,530},smallFont,muted);
 }
 round(dc,{260,707,1190,760},RGB(233,240,250),14);
 text(dc,L"CONTROL DEL PUENTE",{280,722,550,747},smallFont,muted);
}
}
LRESULT CALLBACK comboProc(HWND h,UINT m,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR){
 if(m==WM_PAINT){
  PAINTSTRUCT ps;auto dc=BeginPaint(h,&ps);RECT r;GetClientRect(h,&r);
  bool enabled=IsWindowEnabled(h),focus=GetFocus()==h;
  theme::fill(dc,r,RGB(255,255,255));
  theme::round(dc,r,enabled?RGB(247,249,253):RGB(239,243,248),8,focus?theme::accent:theme::line);
  int selected=int(SendMessage(h,CB_GETCURSEL,0,0));std::wstring value;
  if(selected>=0){int n=int(SendMessage(h,CB_GETLBTEXTLEN,selected,0));value.resize(n+1);SendMessage(h,CB_GETLBTEXT,selected,(LPARAM)value.data());value.resize(n);}
  else value=GetDlgCtrlID(h)==1?L"Seleccionar consola…":L"Seleccionar receptor…";
  RECT label={10,0,r.right-25,r.bottom};theme::text(dc,value.c_str(),label,GetDlgCtrlID(h)>=30?smallFont:font,enabled?theme::ink:theme::muted,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
  theme::text(dc,L"⌄",{r.right-24,0,r.right-5,r.bottom},font,theme::muted,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
  EndPaint(h,&ps);return 0;
 }
 auto result=DefSubclassProc(h,m,w,l);
 if(m==WM_SETFOCUS||m==WM_KILLFOCUS||m==WM_ENABLE||m==CB_SETCURSEL)InvalidateRect(h,nullptr,FALSE);
 return result;
}
namespace meter {
struct Reading {
 int segments=0,decibels=0;
 bool silent=true;
 bool operator==(const Reading& other)const{return segments==other.segments&&decibels==other.decibels&&silent==other.silent;}
};
Reading displayed[8];
Reading sample(int channel){
 const float peak=engine.core.peak[channel].load();
 const float db=20*std::log10(std::max(1e-6f,peak));
 return {int(std::clamp((db+60)/60,0.f,1.f)*12),int(db),peak<1e-6f};
}
void update(HWND h){
 const int channel=GetDlgCtrlID(h)-200;
 const auto next=sample(channel);
 if(next==displayed[channel])return;
 displayed[channel]=next;
 InvalidateRect(h,nullptr,FALSE);
}
void draw(HDC dc,RECT r,const Reading& reading){
  theme::fill(dc,r,RGB(255,255,255));
  for(int i=0;i<12;++i){int y=52-i*4;theme::round(dc,{13,y,37,y+3},i<reading.segments?(i>10?RGB(210,93,85):i>8?theme::accent:theme::blue):RGB(225,233,243),1);}
  theme::text(dc,L"0\n−30\n−60",{44,2,78,57},smallFont,theme::muted);
  auto value=reading.silent?std::wstring(L"−∞ dB"):std::to_wstring(reading.decibels)+L" dB";
  theme::text(dc,value.c_str(),{0,54,r.right,68},smallFont,theme::muted,DT_CENTER|DT_SINGLELINE);
}
}
LRESULT CALLBACK meterProc(HWND h,UINT m,WPARAM w,LPARAM l){
 if(m==WM_ERASEBKGND)return 1;
 if(m==WM_PAINT){
  PAINTSTRUCT ps;auto target=BeginPaint(h,&ps);RECT r;GetClientRect(h,&r);
  {
   theme::PaintBuffer frame(target,r);
   meter::draw(frame.dc(),r,meter::displayed[GetDlgCtrlID(h)-200]);
   frame.present();
  }
  EndPaint(h,&ps);return 0;
 }
 return DefWindowProc(h,m,w,l);
}
