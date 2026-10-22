#pragma once
#include <vector>

// A single logical layout keeps the restored design intact at every window size.
namespace cinematicView {
constexpr int width=1220,height=840;
struct Child { HWND handle; RECT bounds; bool combo; bool compact; };
inline std::vector<Child> children;
inline double scale=1;
inline int left=0,top=0;
inline HFONT regular=nullptr,small=nullptr;
inline bool ready=false,fullscreen=false;
inline WINDOWPLACEMENT placement{};
inline LONG_PTR savedStyle=0,savedExStyle=0;
inline HDC canvas=nullptr;
inline HBITMAP bitmap=nullptr;
inline HGDIOBJ oldBitmap=nullptr;
inline bool validCanvas=false;
// Final presentation buffer: compose the background and scaled canvas offscreen.
// Never clear the visible window between animation frames.
inline HDC presentation=nullptr;
inline HBITMAP presentationBitmap=nullptr;
inline HGDIOBJ oldPresentationBitmap=nullptr;
inline int presentationWidth=0,presentationHeight=0;
inline void releasePresentation(){
 if(presentation){SelectObject(presentation,oldPresentationBitmap);DeleteObject(presentationBitmap);DeleteDC(presentation);}
 presentation=nullptr;presentationBitmap=nullptr;oldPresentationBitmap=nullptr;
 presentationWidth=presentationHeight=0;
}
inline bool preparePresentation(HDC target,int w,int h){
 if(presentation&&w==presentationWidth&&h==presentationHeight)return true;
 releasePresentation();
 if(w<=0||h<=0)return false;
 presentation=CreateCompatibleDC(target);presentationBitmap=CreateCompatibleBitmap(target,w,h);
 if(!presentation||!presentationBitmap){
  if(presentationBitmap)DeleteObject(presentationBitmap);
  if(presentation)DeleteDC(presentation);
  presentation=nullptr;presentationBitmap=nullptr;return false;
 }
 oldPresentationBitmap=SelectObject(presentation,presentationBitmap);
 presentationWidth=w;presentationHeight=h;
 return true;
}
inline RECT map(RECT r){return {left+int(std::lround(r.left*scale)),top+int(std::lround(r.top*scale)),left+int(std::lround(r.right*scale)),top+int(std::lround(r.bottom*scale))};}
inline void invalidate(HWND h,RECT logical){auto r=map(logical);InvalidateRect(h,&r,FALSE);}
inline void remember(HWND h,int x,int y,int w,int ht,bool combo,bool compact){children.push_back({h,{x,y,x+w,y+ht},combo,compact});}
inline HFONT makeFont(int pixels){return CreateFontW(-pixels,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");}
inline void fit(HWND h){
 if(!ready||IsIconic(h))return;
 RECT client;GetClientRect(h,&client);
 scale=std::max(.1,std::min(double(client.right)/width,double(client.bottom)/height));
 left=(client.right-int(std::lround(width*scale)))/2;top=(client.bottom-int(std::lround(height*scale)))/2;
 auto previousRegular=regular,previousSmall=small;
 regular=makeFont(std::max(9,int(std::lround(15*scale))));small=makeFont(std::max(8,int(std::lround(12*scale))));
 auto batch=BeginDeferWindowPos(int(children.size()));
 for(const auto& child:children){
  auto r=map(child.bounds);
  if(batch)batch=DeferWindowPos(batch,child.handle,nullptr,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_NOZORDER|SWP_NOACTIVATE);
  else SetWindowPos(child.handle,nullptr,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_NOZORDER|SWP_NOACTIVATE);
  SendMessageW(child.handle,WM_SETFONT,(WPARAM)(child.compact?small:regular),FALSE);
  if(child.combo){SendMessageW(child.handle,CB_SETITEMHEIGHT,-1,std::max(18,int(std::lround(34*scale))));SendMessageW(child.handle,CB_SETITEMHEIGHT,0,std::max(18,int(std::lround(32*scale))));}
 }
 if(batch)EndDeferWindowPos(batch);
 if(previousRegular)DeleteObject(previousRegular);if(previousSmall)DeleteObject(previousSmall);
 validCanvas=false;InvalidateRect(h,nullptr,FALSE);
}
inline void toggle(HWND h){
 if(!fullscreen){
  placement.length=sizeof(placement);GetWindowPlacement(h,&placement);
  savedStyle=GetWindowLongPtrW(h,GWL_STYLE);savedExStyle=GetWindowLongPtrW(h,GWL_EXSTYLE);
  MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);GetMonitorInfoW(MonitorFromWindow(h,MONITOR_DEFAULTTONEAREST),&monitor);
  fullscreen=true;
  SetWindowLongPtrW(h,GWL_STYLE,(savedStyle&~WS_OVERLAPPEDWINDOW)|WS_POPUP);
  SetWindowLongPtrW(h,GWL_EXSTYLE,savedExStyle&~(WS_EX_WINDOWEDGE|WS_EX_CLIENTEDGE));
  SetWindowPos(h,nullptr,monitor.rcMonitor.left,monitor.rcMonitor.top,monitor.rcMonitor.right-monitor.rcMonitor.left,monitor.rcMonitor.bottom-monitor.rcMonitor.top,SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
 }else{
  fullscreen=false;SetWindowLongPtrW(h,GWL_STYLE,savedStyle);SetWindowLongPtrW(h,GWL_EXSTYLE,savedExStyle);
  SetWindowPlacement(h,&placement);SetWindowPos(h,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
 }
 SetWindowTextW(GetDlgItem(h,17),fullscreen?L"Salir de pantalla · F11":L"Pantalla completa · F11");
 chrome::apply(h);fit(h);
}
inline void paint(HDC target,RECT client,RECT dirty,bool running){
 if(!canvas){canvas=CreateCompatibleDC(target);bitmap=CreateCompatibleBitmap(target,width,height);oldBitmap=SelectObject(canvas,bitmap);}
 // Camino rápido: si solo cambió la zona animada, se repinta esa zona sobre el lienzo ya armado.
 RECT orbit=map(theme::dynamicPainter?theme::dynamicBounds:theme::orbitBounds);
 bool orbitOnly=validCanvas&&dirty.left>=orbit.left&&dirty.top>=orbit.top&&dirty.right<=orbit.right&&dirty.bottom<=orbit.bottom;
 if(orbitOnly){if(theme::dynamicPainter)theme::dynamicPainter(canvas);else cinematicOrbit::draw(canvas,theme::orbitBounds,theme::animationSeconds);}
 else{theme::paint(canvas,running,::font,::titleFont,::smallFont,::channelFont);validCanvas=true;}
 auto viewport=map({0,0,width,height});
 if(preparePresentation(target,client.right,client.bottom)){
  int saved=SaveDC(presentation);
  IntersectClipRect(presentation,dirty.left,dirty.top,dirty.right,dirty.bottom);
  theme::fill(presentation,dirty,theme::bgPaper);
  SetStretchBltMode(presentation,HALFTONE);SetBrushOrgEx(presentation,0,0,nullptr);
  StretchBlt(presentation,viewport.left,viewport.top,viewport.right-viewport.left,viewport.bottom-viewport.top,canvas,0,0,width,height,SRCCOPY);
  RestoreDC(presentation,saved);
  BitBlt(target,dirty.left,dirty.top,dirty.right-dirty.left,dirty.bottom-dirty.top,presentation,dirty.left,dirty.top,SRCCOPY);
 }else{
  // Allocation failure: retain a single visible drawing operation.
  SetStretchBltMode(target,HALFTONE);SetBrushOrgEx(target,0,0,nullptr);
  StretchBlt(target,viewport.left,viewport.top,viewport.right-viewport.left,viewport.bottom-viewport.top,canvas,0,0,width,height,SRCCOPY);
 }
}
inline void release(){releasePresentation();if(canvas){SelectObject(canvas,oldBitmap);DeleteObject(bitmap);DeleteDC(canvas);}if(regular)DeleteObject(regular);if(small)DeleteObject(small);}

inline LRESULT CALLBACK comboPaint(HWND h,UINT m,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR){
 if(m==WM_PAINT){
  PAINTSTRUCT ps;auto dc=BeginPaint(h,&ps);RECT r;GetClientRect(h,&r);bool enabled=IsWindowEnabled(h);
  theme::fill(dc,r,theme::bgPaper);theme::round(dc,r,enabled?theme::cardBg:RGB(20,20,28),6,GetFocus()==h?theme::accentCyan:theme::cardBorder);
  int selected=int(SendMessageW(h,CB_GETCURSEL,0,0));std::wstring value;
  if(selected>=0){int n=int(SendMessageW(h,CB_GETLBTEXTLEN,selected,0));value.resize(n+1);SendMessageW(h,CB_GETLBTEXT,selected,(LPARAM)value.data());}
  else value=GetDlgCtrlID(h)==1?L"Seleccionar consola…":L"Seleccionar receptor…";
  int inset=std::max(5,int(std::lround(10*scale))),arrow=std::max(16,int(std::lround(25*scale)));
  HFONT label=(HFONT)SendMessageW(h,WM_GETFONT,0,0);if(!label)label=::font;
  theme::text(dc,value.c_str(),{inset,0,r.right-arrow,r.bottom},label,enabled?theme::textLight:theme::textMuted,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
  theme::text(dc,L"⌄",{r.right-arrow,0,r.right-3,r.bottom},label,theme::textMuted,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
  EndPaint(h,&ps);return 0;
 }
 return DefSubclassProc(h,m,w,l);
}
inline LRESULT CALLBACK meterPaint(HWND h,UINT m,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR){
 if(m==WM_PAINT){
  PAINTSTRUCT ps;auto target=BeginPaint(h,&ps);RECT r;GetClientRect(h,&r);
  theme::PaintBuffer frame(target,{0,0,86,66});meter::draw(frame.dc(),{0,0,86,66},meter::displayed[GetDlgCtrlID(h)-200]);
  SetStretchBltMode(target,HALFTONE);StretchBlt(target,0,0,r.right,r.bottom,frame.dc(),0,0,86,66,SRCCOPY);EndPaint(h,&ps);return 0;
 }
 return DefSubclassProc(h,m,w,l);
}
}
