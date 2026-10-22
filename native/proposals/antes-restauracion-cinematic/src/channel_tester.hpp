#pragma once
#include "channel_sequence.hpp"

// Both windows share one engine and the main configuration controls.
// A sequence never resumes the live bridge automatically.
namespace tester {
HWND panel=nullptr,configuration=nullptr,feedback=nullptr;
HWND speakers[8]{},sequenceButton=nullptr,liveButton=nullptr;
bridge::ChannelSequence sequence;
bool ownsPlayback=false;
int lastVisual=-999;
const wchar_t* codes[]={L"FL",L"FR",L"C",L"LFE",L"BL",L"BR",L"SL",L"SR"};

void tick();
void cancel(){sequence.cancel();ownsPlayback=false;}

void setTextIfChanged(HWND h,const std::wstring& value){
 if(boxText(h)!=value)SetWindowTextW(h,value.c_str());
}
void sync(){
 if(!panel)return;
 const unsigned count=channelCount();
 const bool busy=engine.running()||sequence.active();
 std::wstring config=L"ENTRADA: "+boxText(inputBox)+L"\r\nSALIDA: "+boxText(outputBox)+
     L"\r\n"+boxText(profileBox)+L" · "+boxText(gainBox)+L" · 48 kHz";
 setTextIfChanged(configuration,config);
 const int active=engine.running()&&engine.test?engine.toneChannel.load():-1;
 for(unsigned c=0;c<8;++c){
  EnableWindow(speakers[c],!busy&&c<count);
  std::wstring caption=std::wstring(codes[c])+L" · "+boxText(routes[c]);
  if(int(c)==active)caption=L"▶ "+caption;
  setTextIfChanged(speakers[c],caption);
 }
 EnableWindow(sequenceButton,!busy);
 EnableWindow(liveButton,!busy);
 setTextIfChanged(feedback,boxText(statusLabel));
 int visual=int(count)*100+(busy?20:0)+active;
 if(visual!=lastVisual){lastVisual=visual;InvalidateRect(panel,nullptr,FALSE);}
}

void play(unsigned channel){
 if(engine.running()||channel>=channelCount())return;
 ownsPlayback=true;
 if(!start(true,int(channel))){cancel();sync();}
}
void beginSequence(){
 if(engine.running()||sequence.active())return;
 sequence.begin(channelCount());
 tick();
}
void tick(){
 if(engine.running())return;
 const int channel=sequence.takeReady(GetTickCount64());
 if(channel>=0)play(unsigned(channel));
}
void finishTone(){
 engine.stop();
 if(sequence.active()){
  sequence.toneFinished(GetTickCount64());
  if(sequence.active()){
   enableControls(false);
   status(L"Secuencia: pausa entre altavoces. Detener / Esc cancela.");
  }else{
   cancel();enableControls(true);status(L"Secuencia terminada. Salida en silencio.");
  }
 }else{
  cancel();enableControls(true);status(L"Tono terminado. Salida en silencio.");
 }
 for(auto h:meters)meter::update(h);
 sync();
}

void paint(HDC dc){
 RECT r;GetClientRect(panel,&r);theme::fill(dc,r,theme::paper);
 theme::text(dc,L"Prueba tu sala",{28,20,860,62},channelFont,theme::ink);
 theme::text(dc,L"Configuración compartida con Planetario Bridge",{30,61,890,87},font,theme::muted);
 theme::round(dc,{24,94,916,190},theme::surface,12,theme::line);
 theme::round(dc,{24,205,916,650},theme::surface,18,theme::line);
 theme::round(dc,{218,220,722,225},theme::accent,2);
 theme::text(dc,L"PANTALLA / FRENTE",{340,231,650,258},smallFont,theme::muted);
 theme::round(dc,{406,377,534,466},RGB(47,32,67),30);
 theme::text(dc,L"◎",{444,386,520,427},channelFont,theme::accent);
 theme::text(dc,L"OYENTE",{444,431,525,455},smallFont,theme::ink);
 theme::text(dc,L"Tonos: −36 dBFS · 440 Hz / LFE 60 Hz · 2 segundos por salida",{32,624,900,646},smallFont,theme::muted);
}

LRESULT CALLBACK windowProc(HWND h,UINT m,WPARAM w,LPARAM l){
 if(m==WM_THEMECHANGED||m==WM_SETTINGCHANGE||m==WM_DWMCOMPOSITIONCHANGED)chrome::apply(h);
 if(m==WM_ERASEBKGND)return 1;
 if(m==WM_PAINT){
  PAINTSTRUCT ps;auto dc=BeginPaint(h,&ps);RECT r;GetClientRect(h,&r);
  {theme::PaintBuffer frame(dc,r);paint(frame.dc());frame.present();}
  EndPaint(h,&ps);return 0;
 }
 if(m==WM_CTLCOLORSTATIC){SetTextColor((HDC)w,theme::muted);SetBkColor((HDC)w,theme::paper);return (LRESULT)theme::background;}
 if(m==WM_DRAWITEM){
  auto d=(DRAWITEMSTRUCT*)l;
  if(d->CtlType!=ODT_BUTTON)return FALSE;
  bool active=d->CtlID>=300&&d->CtlID<308&&engine.running()&&engine.test&&int(d->CtlID-300)==engine.toneChannel.load();
  bool disabled=d->itemState&ODS_DISABLED,pressed=d->itemState&ODS_SELECTED;
  theme::fill(d->hDC,d->rcItem,theme::paper);
  theme::round(d->hDC,d->rcItem,active||pressed?theme::blue:theme::surface,10,active?theme::accent:theme::line);
  theme::text(d->hDC,boxText(d->hwndItem).c_str(),d->rcItem,font,disabled&&!active?theme::muted:theme::ink,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
  if(d->itemState&ODS_FOCUS){RECT r=d->rcItem;InflateRect(&r,-4,-4);DrawFocusRect(d->hDC,&r);}
  return TRUE;
 }
 if(m==WM_COMMAND){
  int id=LOWORD(w);
  if(id>=300&&id<308)play(unsigned(id-300));
  if(id==400)beginSequence();
  if(id==401&&!engine.running()&&!sequence.active()){ownsPlayback=true;if(!start(false))ownsPlayback=false;sync();}
  if(id==402)stop();
  return 0;
 }
 if(m==WM_CLOSE){
  if(ownsPlayback||sequence.active())stop();
  DestroyWindow(h);return 0;
 }
 if(m==WM_DESTROY){panel=nullptr;lastVisual=-999;return 0;}
 return DefWindowProc(h,m,w,l);
}

void open(){
 if(panel){ShowWindow(panel,SW_RESTORE);SetForegroundWindow(panel);sync();return;}
 auto instance=GetModuleHandle(nullptr);
 WNDCLASSW wc{};wc.lpfnWndProc=windowProc;wc.hInstance=instance;wc.lpszClassName=L"BridgeChannelTester";wc.hbrBackground=theme::background;wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hIcon=chrome::icon(false);
 RegisterClassW(&wc);
 DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN;
 RECT r={0,0,940,770};AdjustWindowRect(&r,style,FALSE);
 panel=CreateWindowW(wc.lpszClassName,L"PLI · Probador de canales",style,CW_USEDEFAULT,CW_USEDEFAULT,r.right-r.left,r.bottom-r.top,window,nullptr,instance,nullptr);
 chrome::apply(panel);chrome::setIcons(panel);
 auto child=[&](const wchar_t* cls,const wchar_t* text,int x,int y,int width,int height,int id){
  DWORD extra=std::wcscmp(cls,L"BUTTON")==0?WS_TABSTOP|BS_OWNERDRAW:0;
  auto c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|extra,x,y,width,height,panel,(HMENU)(INT_PTR)id,instance,nullptr);
  SendMessage(c,WM_SETFONT,(WPARAM)font,TRUE);return c;
 };
 configuration=child(L"STATIC",L"",40,104,856,78,0);
 const int x[]={80,680,380,380,150,610,40,720};
 const int y[]={275,275,265,550,550,550,405,405};
 for(int c=0;c<8;++c)speakers[c]=child(L"BUTTON",codes[c],x[c],y[c],180,52,300+c);
 sequenceButton=child(L"BUTTON",L"▶  Secuencia por canales",24,668,280,40,400);
 liveButton=child(L"BUTTON",L"Escuchar entrada / ruteo real",320,668,300,40,401);
 child(L"BUTTON",L"■  Detener · Esc",636,668,280,40,402);
 feedback=child(L"STATIC",L"",28,722,886,42,0);
 sync();ShowWindow(panel,SW_SHOW);
}
}
