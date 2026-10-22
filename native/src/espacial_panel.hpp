#pragma once
// Panel «PLI Espacial» de Play Dolby (fase 3): domo visto desde arriba con las bocinas de la
// ficha y hasta 8 objetos en vivo. Cada objeto toma un canal USB de la consola; se arrastra con el
// ratón o se pone a girar/mecer. Los objetos se SUMAN a las salidas directas del paso 03.
// Comparte el motor con la ventana principal (engine.core.espacio).
#include <windowsx.h>
#include <iterator>
#include <memory>

namespace espacialUI {
HWND panel=nullptr,info=nullptr,toggle=nullptr;
HWND entradas[pli::MaxObjetos]{},movimientos[pli::MaxObjetos]{},niveles[pli::MaxObjetos]{};
std::unique_ptr<pli::Disposicion> domo;
std::wstring origen;          // de dónde salió la disposición (para el usuario)
int arrastrando=-1;
const RECT vista={24,96,524,596};
const int cx=274,cy=346,radio=196;
const COLORREF colores[pli::MaxObjetos]={RGB(193,135,255),RGB(85,248,185),RGB(255,184,77),RGB(98,180,255),
                                         RGB(255,110,150),RGB(240,240,120),RGB(140,255,255),RGB(255,150,90)};

struct Mov{const wchar_t* texto;pli::Modo modo;float rpm,amplitud,periodo;};
const Mov movs[]={
 {L"Fijo",pli::Modo::Fijo,0,0,8},
 {L"Giro ↻ lento · 1,5 v/min",pli::Modo::Giro,1.5f,0,8},{L"Giro ↻ medio · 3 v/min",pli::Modo::Giro,3,0,8},{L"Giro ↻ rápido · 6 v/min",pli::Modo::Giro,6,0,8},
 {L"Giro ↺ lento · 1,5 v/min",pli::Modo::Giro,-1.5f,0,8},{L"Giro ↺ medio · 3 v/min",pli::Modo::Giro,-3,0,8},{L"Giro ↺ rápido · 6 v/min",pli::Modo::Giro,-6,0,8},
 {L"Vaivén ±30° · 8 s",pli::Modo::Vaiven,0,30,8},{L"Vaivén ±60° · 12 s",pli::Modo::Vaiven,0,60,12},{L"Vaivén ±90° · 16 s",pli::Modo::Vaiven,0,90,16},
 {L"Vaivén ±25° · 24 s (voz)",pli::Modo::Vaiven,0,25,24},{L"Giro ↻ 5 v/min (percusión)",pli::Modo::Giro,5,0,8},
};
const float nivelesDb[]={0,-3,-6,-12,-18};
int movSel[pli::MaxObjetos]{};  // movimiento elegido por objeto (vive aunque el panel esté cerrado)

std::string utf8(const std::wstring& w){
 if(w.empty())return {};
 int n=WideCharToMultiByte(CP_UTF8,0,w.c_str(),-1,nullptr,0,nullptr,nullptr);
 std::string s(n,'\0');WideCharToMultiByte(CP_UTF8,0,w.c_str(),-1,s.data(),n,nullptr,nullptr);
 if(!s.empty())s.pop_back();return s;
}

// La ficha del domo: junto al ejecutable (versión instalada o USB) o, en el proyecto,
// <proyecto>\pruebas-domo\domo.json, dos carpetas arriba de native\bin.
std::filesystem::path rutaFicha(){
 wchar_t exe[MAX_PATH];GetModuleFileNameW(nullptr,exe,MAX_PATH);
 auto dir=std::filesystem::path(exe).parent_path();
 if(std::filesystem::exists(dir/L"domo.json"))return dir/L"domo.json";
 return (dir/L".."/L".."/L"pruebas-domo"/L"domo.json").lexically_normal();
}

// Solo con el puente detenido: el audio lee la disposición.
bool cargaFicha(){
 if(engine.running()){status(L"Detén el puente para recargar la ficha del domo.");return false;}
 auto ruta=rutaFicha();
 try{
  domo=std::make_unique<pli::Disposicion>(pli::Disposicion::desdeFicha(utf8(ruta.wstring())));
  origen=L"Ficha: "+ruta.wstring();
 }catch(const std::exception& e){
  domo=std::make_unique<pli::Disposicion>(pli::Disposicion::anilloClasico());
  origen=L"Sin ficha válida ("+wide(e.what())+L"). Se usa el anillo clásico.";
 }
 engine.core.espacio.prepara(domo.get(),engine.rate);
 return true;
}

void aplica(int k){
 auto& o=engine.core.espacio.obj[k];
 int e=int(SendMessage(entradas[k],CB_GETCURSEL,0,0));  // 0 apagado · 1..32 USB n · 33..48 par estéreo
 int m=std::clamp(int(SendMessage(movimientos[k],CB_GETCURSEL,0,0)),0,int(std::size(movs))-1);
 movSel[k]=m;
 int n=std::clamp(int(SendMessage(niveles[k],CB_GETCURSEL,0,0)),0,int(std::size(nivelesDb))-1);
 o.modo=int(movs[m].modo);o.rpm=movs[m].rpm;o.amplitud=movs[m].amplitud;o.periodo=movs[m].periodo;
 o.nivel=std::pow(10.f,nivelesDb[n]/20);
 o.estereo=e>=33;
 o.entrada=e>=33?(e-33)*2:e-1;  // al final: el audio ve la entrada ya con sus parámetros
 if(!engine.running())o.posicion=float(pli::normaliza(o.azimut));
}

// Pone las filas del panel (si está abierto) según el estado del motor.
int indiceEntrada(const pli::ObjetoVivo& o){int e=o.entrada;return e<0?0:o.estereo?33+e/2:e+1;}
void sincronizaFilas(){
 if(!panel)return;
 for(int k=0;k<pli::MaxObjetos;++k){
  auto& o=engine.core.espacio.obj[k];
  SendMessage(entradas[k],CB_SETCURSEL,indiceEntrada(o),0);
  SendMessage(movimientos[k],CB_SETCURSEL,movSel[k],0);
  int nv=0;for(int i=0;i<int(std::size(nivelesDb));++i)if(std::fabs(std::pow(10.f,nivelesDb[i]/20)-o.nivel)<1e-4f)nv=i;
  SendMessage(niveles[k],CB_SETCURSEL,nv,0);
 }
 InvalidateRect(panel,nullptr,FALSE);
}

void sync(){
 if(!panel)return;
 const bool on=engine.core.espacio.activo;
 std::wstring t=on?L"✦  Espacial ENCENDIDO":L"○  Espacial apagado";
 if(boxText(toggle)!=t){SetWindowTextW(toggle,t.c_str());InvalidateRect(toggle,nullptr,FALSE);}
 std::wstring s=origen+L"\r\n";
 int faltan=0;
 for(auto& o:engine.core.espacio.obj){int e=o.entrada;if(engine.running()&&e>=0&&unsigned(e)>=engine.inputChannels)++faltan;}
 if(faltan)s+=L"⚠ "+std::to_wstring(faltan)+L" objeto(s) usan una entrada USB que el puente no está capturando: detén e inicia el puente para tomarla.";
 else if(on)s+=L"Los objetos se suman a las salidas directas del paso 03: pon en «Silencio» las que no quieras duplicar.";
 else s+=L"Enciende «Espacial» para que los objetos suenen. Se puede encender y apagar con el puente en vivo.";
 if(boxText(info)!=s)SetWindowTextW(info,s.c_str());
}

void punto(int k,double& x,double& y){
 double a=engine.core.espacio.obj[k].posicion.load()*pli::Pi/180;
 x=cx+radio*.78*std::sin(a);y=cy-radio*.78*std::cos(a);
}

void paint(HDC dc){
 RECT r;GetClientRect(panel,&r);theme::fill(dc,r,theme::paper);
 theme::text(dc,L"PLI Espacial",{28,18,700,60},channelFont,theme::ink);
 theme::text(dc,L"Objetos en vivo: cada uno toma un canal de la consola y se mueve por el domo",{30,58,900,82},font,theme::muted);
 theme::round(dc,vista,theme::surface,18,theme::line);
 Gdiplus::Graphics g(dc);g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
 Gdiplus::Pen aro(theme::color(theme::line),2),guia(theme::color(RGB(36,36,52)),1);
 g.DrawEllipse(&aro,cx-radio,cy-radio,2*radio,2*radio);
 g.DrawEllipse(&guia,cx-radio*.78f,cy-radio*.78f,radio*1.56f,radio*1.56f);
 g.DrawLine(&guia,cx,cy-radio,cx,cy+radio);g.DrawLine(&guia,cx-radio,cy,cx+radio,cy);
 theme::text(dc,L"PANTALLA / FRENTE",{cx-120,vista.top+8,cx+120,vista.top+30},smallFont,theme::accent,DT_CENTER|DT_SINGLELINE);
 theme::text(dc,L"◎",{cx-20,cy-16,cx+20,cy+16},font,theme::muted,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
 if(domo)for(auto& b:domo->bocinas()){
  double a=b.azimut*pli::Pi/180;int x=int(cx+radio*std::sin(a)),y=int(cy-radio*std::cos(a));
  theme::round(dc,{x-24,y-13,x+24,y+13},RGB(38,34,54),6,theme::accent);
  theme::text(dc,wide(b.id).c_str(),{x-24,y-13,x+24,y+13},smallFont,theme::ink,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
 }
 for(int k=0;k<pli::MaxObjetos;++k){
  if(engine.core.espacio.obj[k].entrada<0)continue;
  double x,y;punto(k,x,y);
  Gdiplus::SolidBrush relleno(theme::color(colores[k]));
  g.FillEllipse(&relleno,float(x-13),float(y-13),26.f,26.f);
  theme::text(dc,std::to_wstring(k+1).c_str(),{int(x)-13,int(y)-13,int(x)+13,int(y)+13},smallFont,RGB(17,17,25),DT_CENTER|DT_VCENTER|DT_SINGLELINE);
 }
 theme::text(dc,L"Arrastra un objeto para moverlo · el LFE solo recibe rutas directas",{vista.left+16,vista.bottom-28,vista.right-16,vista.bottom-8},smallFont,theme::muted,DT_CENTER|DT_SINGLELINE);
 theme::text(dc,L"OBJ     ENTRADA                MOVIMIENTO                                   NIVEL",{552,100,990,122},smallFont,theme::muted,DT_SINGLELINE);
 for(int k=0;k<pli::MaxObjetos;++k){
  int y=134+k*50;
  Gdiplus::SolidBrush relleno(theme::color(colores[k]));
  g.FillEllipse(&relleno,556.f,float(y+4),24.f,24.f);
  theme::text(dc,std::to_wstring(k+1).c_str(),{556,y+4,580,y+28},smallFont,RGB(17,17,25),DT_CENTER|DT_VCENTER|DT_SINGLELINE);
 }
}

void guarda(){
 std::wofstream f(dataDir()/L"espacial.txt");
 for(int k=0;k<pli::MaxObjetos;++k)
  f<<SendMessage(entradas[k],CB_GETCURSEL,0,0)<<L' '<<SendMessage(movimientos[k],CB_GETCURSEL,0,0)<<L' '<<SendMessage(niveles[k],CB_GETCURSEL,0,0)<<L' '<<engine.core.espacio.obj[k].azimut.load()<<L'\n';
 status(L"Objetos de PLI Espacial guardados en %LOCALAPPDATA%\\PlanetarioBridge\\espacial.txt.");
}

void carga(){
 std::wifstream f(dataDir()/L"espacial.txt");
 if(!f){status(L"No hay objetos de PLI Espacial guardados.");return;}
 int e[pli::MaxObjetos],m[pli::MaxObjetos],n[pli::MaxObjetos];float a[pli::MaxObjetos];
 for(int k=0;k<pli::MaxObjetos;++k)
  if(!(f>>e[k]>>m[k]>>n[k]>>a[k])||e[k]<0||e[k]>48||m[k]<0||m[k]>=int(std::size(movs))||n[k]<0||n[k]>=int(std::size(nivelesDb))||!std::isfinite(a[k])){status(L"Archivo de PLI Espacial inválido; no se cambió nada.");return;}
 for(int k=0;k<pli::MaxObjetos;++k){
  SendMessage(entradas[k],CB_SETCURSEL,e[k],0);SendMessage(movimientos[k],CB_SETCURSEL,m[k],0);SendMessage(niveles[k],CB_SETCURSEL,n[k],0);
  engine.core.espacio.obj[k].azimut=a[k];aplica(k);
 }
 status(L"Objetos de PLI Espacial cargados. «Espacial» sigue como estaba: no se enciende solo.");
 InvalidateRect(panel,nullptr,FALSE);
}

// ---------- Dynamic Space en la ventana principal (fase 3b) ----------
// Lee las posiciones que ya publica el motor: no agrega trabajo al audio.
float brillo[pli::NumCanales]{};

void alterna(){
 engine.core.espacio.activo=!engine.core.espacio.activo;
 log(engine.core.espacio.activo?"ESPACIAL ON":"ESPACIAL OFF");
 cinematicView::invalidate(window,theme::dynamicBounds);
 InvalidateRect(GetDlgItem(window,19),nullptr,FALSE);
 if(panel){sync();InvalidateRect(toggle,nullptr,FALSE);}
}

void fijaObjeto(int k,int mov,float azimut){
 auto& o=engine.core.espacio.obj[k];const Mov& mv=movs[mov];
 movSel[k]=mov;
 o.modo=int(mv.modo);o.rpm=mv.rpm;o.amplitud=mv.amplitud;o.periodo=mv.periodo;o.azimut=azimut;
 if(!engine.running())o.posicion=float(pli::normaliza(azimut));
 if(panel)SendMessage(movimientos[k],CB_SETCURSEL,mov,0);
}

// Escenas rápidas sobre los objetos que tienen entrada asignada.
void escenaRapida(int cual){
 std::vector<int> act;
 for(int k=0;k<pli::MaxObjetos;++k)if(engine.core.espacio.obj[k].entrada>=0)act.push_back(k);
 if(act.empty()){status(L"Dynamic Space: asigna entradas USB a los objetos en «✦ PLI Espacial» antes de usar una escena.");return;}
 const int n=int(act.size());
 for(int i=0;i<n;++i){
  const int k=act[i];const float az=float(i*360.0/n);
  if(cual==0)fijaObjeto(k,0,az);                 // Quieto: repartidos alrededor
  else if(cual==1)fijaObjeto(k,i%2?4:2,az);      // Órbita: giros alternados ↻ 3 / ↺ 1,5
  else if(cual==2)fijaObjeto(k,7,az);            // Respira: vaivenes ±30°
  else{                                          // La de 1:45: la coreografía de capas71 a velocidad normal
   const int mv[]={10,1,5,11};const float a[]={0,0,180,0};
   if(i<4)fijaObjeto(k,mv[i],a[i]);else fijaObjeto(k,0,0);  // del 5.º en adelante (bajo, bombo): fijos al frente
  }
 }
 const wchar_t* nombres[]={L"Quieto",L"Órbita",L"Respira",L"La de 1:45"};
 status(std::wstring(L"Dynamic Space · escena «")+nombres[cual]+L"» en "+std::to_wstring(n)+L" objeto(s)."+
        (engine.core.espacio.activo?L"":L" Enciende DYNAMIC SPACE para oírla."));
 log("ESCENA "+std::to_string(cual)+" objetos="+std::to_string(n));
 if(panel)InvalidateRect(panel,&vista,FALSE);
}

int objetosActivos(){int n=0;for(auto& o:engine.core.espacio.obj)if(o.entrada>=0)++n;return n;}

void syncPrincipal(){
 const bool on=engine.core.espacio.activo;
 std::wstring t=on?L"✦  DYNAMIC SPACE · "+std::to_wstring(objetosActivos()):std::wstring(L"○  DYNAMIC SPACE");
 HWND b=GetDlgItem(window,19);
 if(boxText(b)!=t){SetWindowTextW(b,t.c_str());InvalidateRect(b,nullptr,FALSE);}
}

// Vista previa: con el puente detenido nadie calcula posiciones, así que la interfaz las mueve
// (mismas fórmulas que el motor) para ensayar coreografías sin consola. Con el puente en vivo, manda el audio.
void vistaPrevia(){
 if(engine.running()||!engine.core.espacio.activo)return;
 static double antes=theme::animationSeconds;static double fase[pli::MaxObjetos]{};
 const double dt=std::clamp(theme::animationSeconds-antes,0.,.25)*std::max(0.f,engine.core.espacio.velocidad.load());
 antes=theme::animationSeconds;
 for(int k=0;k<pli::MaxObjetos;++k){
  auto& o=engine.core.espacio.obj[k];double az=o.azimut;
  if(o.modo==int(pli::Modo::Giro)){fase[k]=std::fmod(fase[k]+o.rpm*6.*dt,360.);az+=fase[k];}
  else if(o.modo==int(pli::Modo::Vaiven)){const double per=std::max(.1f,o.periodo.load());fase[k]=std::fmod(fase[k]+dt,per);az+=o.amplitud*std::sin(2*pli::Pi*fase[k]/per);}
  else fase[k]=0;
  o.posicion=float(pli::normaliza(az));
 }
}

void pintaPrincipal(HDC dc){
 vistaPrevia();
 const RECT r=theme::orbitBounds;const bool on=engine.core.espacio.activo;const double t=theme::animationSeconds;
 theme::fill(dc,r,theme::paper);
 Gdiplus::Graphics g(dc);g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
 g.SetClip(Gdiplus::Rect(r.left,r.top,r.right-r.left,r.bottom-r.top));
 const float x0=(r.left+r.right)*.5f,y0=(r.top+r.bottom)*.5f+8,R=78.f*(on?1.f:1.f+float(std::sin(t*.55))*.015f);
 Gdiplus::GraphicsPath halo;halo.AddEllipse(x0-130.f,y0-112.f,260.f,224.f);
 Gdiplus::PathGradientBrush glow(&halo);glow.SetCenterColor(Gdiplus::Color(on?100:50,140,52,240));
 Gdiplus::Color edge(0,17,17,25);int count=1;glow.SetSurroundColors(&edge,&count);g.FillPath(&glow,&halo);
 Gdiplus::Pen aro(Gdiplus::Color(on?215:120,193,135,255),1.7f),guia(Gdiplus::Color(on?70:35,193,135,255),1.f);
 g.DrawEllipse(&aro,x0-R,y0-R,2*R,2*R);g.DrawEllipse(&guia,x0-R*.72f,y0-R*.72f,R*1.44f,R*1.44f);
 theme::text(dc,on?L"DYNAMIC SPACE · EN VIVO":L"DYNAMIC SPACE · APAGADO",{r.left,r.top+2,r.right,r.top+18},::smallFont,on?theme::statusLive:theme::muted,DT_CENTER|DT_SINGLELINE);
 if(domo)for(auto& b:domo->bocinas()){
  double a=b.azimut*pli::Pi/180;int x=int(x0+R*std::sin(a)),y=int(y0-R*std::cos(a));
  theme::round(dc,{x-15,y-9,x+15,y+9},RGB(32,29,46),5,on?theme::accent:theme::line);
  theme::text(dc,wide(b.id).c_str(),{x-15,y-9,x+15,y+9},::smallFont,on?theme::ink:theme::muted,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
 }
 if(on){
  for(int k=0;k<pli::MaxObjetos;++k){
   const auto& o=engine.core.espacio.obj[k];if(o.entrada<0)continue;
   double a=o.posicion.load()*pli::Pi/180;float x=float(x0+R*.72*std::sin(a)),y=float(y0-R*.72*std::cos(a));
   Gdiplus::SolidBrush aura(Gdiplus::Color(70,GetRValue(colores[k]),GetGValue(colores[k]),GetBValue(colores[k])));
   g.FillEllipse(&aura,x-12,y-12,24.f,24.f);
   Gdiplus::SolidBrush punto(theme::color(colores[k]));g.FillEllipse(&punto,x-7,y-7,14.f,14.f);
  }
 }else{  // apagado: un punto lento recorre el aro, como la órbita decorativa
  double a=t*.35;Gdiplus::SolidBrush punto(Gdiplus::Color(200,245,234,255));
  g.FillEllipse(&punto,float(x0+R*std::sin(a))-3,float(y0-R*std::cos(a))-3,6.f,6.f);
 }
 g.ResetClip();
 // Barras de las tarjetas FL…SR: cuánto le manda el paneo a cada salida (potencia, suavizada).
 float objetivo[pli::NumCanales]{};
 if(on&&domo)for(const auto& o:engine.core.espacio.obj){
  if(o.entrada<0)continue;
  auto gs=domo->ganancias(o.posicion.load());const float nv=o.nivel.load();
  for(int c=0;c<pli::NumCanales;++c)objetivo[c]+=gs[c]*gs[c]*nv*nv;
 }
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

LRESULT CALLBACK windowProc(HWND h,UINT m,WPARAM w,LPARAM l){
 if(m==WM_THEMECHANGED||m==WM_SETTINGCHANGE||m==WM_DWMCOMPOSITIONCHANGED)chrome::apply(h);
 if(m==WM_ERASEBKGND)return 1;
 if(m==WM_PAINT){
  PAINTSTRUCT ps;auto dc=BeginPaint(h,&ps);RECT r;GetClientRect(h,&r);
  {theme::PaintBuffer frame(dc,r);paint(frame.dc());frame.present();}
  EndPaint(h,&ps);return 0;
 }
 if(m==WM_CTLCOLORSTATIC){SetTextColor((HDC)w,theme::muted);SetBkColor((HDC)w,theme::paper);return (LRESULT)theme::background;}
 if(m==WM_CTLCOLORLISTBOX){SetTextColor((HDC)w,theme::textLight);SetBkColor((HDC)w,theme::cardBg);return (LRESULT)theme::cardBrush;}
 if(m==WM_MEASUREITEM){((MEASUREITEMSTRUCT*)l)->itemHeight=26;return TRUE;}
 if(m==WM_DRAWITEM){
  auto d=(DRAWITEMSTRUCT*)l;
  if(d->CtlType==ODT_COMBOBOX){
   theme::fill(d->hDC,d->rcItem,d->itemState&ODS_SELECTED?RGB(38,52,74):theme::cardBg);
   if(d->itemID!=UINT(-1)){
    int n=int(SendMessage(d->hwndItem,CB_GETLBTEXTLEN,d->itemID,0));std::wstring v(n+1,0);
    SendMessage(d->hwndItem,CB_GETLBTEXT,d->itemID,(LPARAM)v.data());v.resize(n);
    RECT r=d->rcItem;r.left+=10;theme::text(d->hDC,v.c_str(),r,smallFont,theme::textLight,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
   }
   return TRUE;
  }
  if(d->CtlType!=ODT_BUTTON)return FALSE;
  const bool encendido=d->CtlID==600&&engine.core.espacio.activo,pressed=d->itemState&ODS_SELECTED,disabled=d->itemState&ODS_DISABLED;
  theme::fill(d->hDC,d->rcItem,theme::paper);
  theme::round(d->hDC,d->rcItem,encendido||pressed?theme::blue:theme::surface,10,encendido?theme::accent:theme::line);
  theme::text(d->hDC,boxText(d->hwndItem).c_str(),d->rcItem,font,disabled?theme::muted:theme::ink,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
  if(d->itemState&ODS_FOCUS){RECT r=d->rcItem;InflateRect(&r,-4,-4);DrawFocusRect(d->hDC,&r);}
  return TRUE;
 }
 if(m==WM_COMMAND){
  int id=LOWORD(w),code=HIWORD(w);
  if(code==CBN_SELCHANGE&&id>=500&&id<530){aplica((id-500)%10);InvalidateRect(h,&vista,FALSE);sync();}
  if(id==600)alterna();
  if(id==601&&cargaFicha()){InvalidateRect(h,nullptr,FALSE);sync();status(L"Ficha del domo recargada.");}
  if(id==602)guarda();
  if(id==603)carga();
  return 0;
 }
 if(m==WM_LBUTTONDOWN){
  int x=GET_X_LPARAM(l),y=GET_Y_LPARAM(l);double mejor=20*20;arrastrando=-1;
  for(int k=0;k<pli::MaxObjetos;++k){
   if(engine.core.espacio.obj[k].entrada<0)continue;
   double px,py;punto(k,px,py);double d2=(px-x)*(px-x)+(py-y)*(py-y);
   if(d2<mejor){mejor=d2;arrastrando=k;}
  }
  if(arrastrando>=0)SetCapture(h);  return 0;
 }
 if(m==WM_MOUSEMOVE&&arrastrando>=0&&(w&MK_LBUTTON)){
  double dx=GET_X_LPARAM(l)-cx,dy=GET_Y_LPARAM(l)-cy;
  if(dx*dx+dy*dy<1)return 0;
  double a=std::atan2(dx,-dy)*180/pli::Pi;
  auto& o=engine.core.espacio.obj[arrastrando];
  // En giro o vaivén el punto queda donde se suelta: se corrige el desfase que lleva el movimiento.
  double desfase=std::remainder(double(o.posicion)-double(o.azimut),360.);
  if(!engine.running()&&!engine.core.espacio.activo)desfase=0;  // sin vista previa, posición = azimut
  o.azimut=float(pli::normaliza(a-desfase));
  o.posicion=float(pli::normaliza(a));
  InvalidateRect(h,&vista,FALSE);
  return 0;
 }
 if(m==WM_LBUTTONUP){int k=arrastrando;arrastrando=-1;if(k>=0&&GetCapture()==h)ReleaseCapture();return 0;}
 if(m==WM_TIMER){
  if(engine.core.espacio.activo)InvalidateRect(h,&vista,FALSE);
  sync();return 0;
 }
 if(m==WM_CLOSE){DestroyWindow(h);return 0;}
 if(m==WM_DESTROY){KillTimer(h,1);panel=nullptr;arrastrando=-1;return 0;}
 return DefWindowProc(h,m,w,l);
}

void open(){
 if(panel){ShowWindow(panel,SW_RESTORE);SetForegroundWindow(panel);sync();return;}
 static bool iniciado=false;
 if(!iniciado){iniciado=true;for(int k=0;k<pli::MaxObjetos;++k)engine.core.espacio.obj[k].azimut=float(k*45);}
 auto instance=GetModuleHandle(nullptr);
 WNDCLASSW wc{};wc.lpfnWndProc=windowProc;wc.hInstance=instance;wc.lpszClassName=L"PliEspacial";wc.hbrBackground=theme::background;wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hIcon=chrome::icon(false);
 RegisterClassW(&wc);
 DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN;
 RECT r={0,0,1010,720};AdjustWindowRect(&r,style,FALSE);
 panel=CreateWindowW(wc.lpszClassName,L"Play Dolby · PLI Espacial",style,CW_USEDEFAULT,CW_USEDEFAULT,r.right-r.left,r.bottom-r.top,window,nullptr,instance,nullptr);
 chrome::apply(panel);chrome::setIcons(panel);
 auto child=[&](const wchar_t* cls,const wchar_t* text,int x,int y,int width,int height,int id,DWORD extra){
  auto c=CreateWindowW(cls,text,WS_CHILD|WS_VISIBLE|extra,x,y,width,height,panel,(HMENU)(INT_PTR)id,instance,nullptr);
  SendMessage(c,WM_SETFONT,(WPARAM)(id>=500&&id<530?smallFont:font),TRUE);return c;
 };
 auto combo=[&](int x,int y,int width,int id){
  auto c=child(L"COMBOBOX",L"",x,y,width,320,id,CBS_DROPDOWNLIST|CBS_OWNERDRAWFIXED|CBS_HASSTRINGS|WS_VSCROLL|WS_TABSTOP);
  SendMessage(c,CB_SETITEMHEIGHT,-1,30);SetWindowSubclass(c,comboProc,1,0);return c;
 };
 for(int k=0;k<pli::MaxObjetos;++k){
  int y=134+k*50;
  entradas[k]=combo(592,y,128,500+k);
  SendMessage(entradas[k],CB_ADDSTRING,0,(LPARAM)L"Apagado");
  for(int j=1;j<=32;++j){auto s=L"USB "+std::to_wstring(j);SendMessage(entradas[k],CB_ADDSTRING,0,(LPARAM)s.c_str());}
  for(int j=1;j<=31;j+=2){auto s=L"USB "+std::to_wstring(j)+L"-"+std::to_wstring(j+1)+L" estéreo";SendMessage(entradas[k],CB_ADDSTRING,0,(LPARAM)s.c_str());}
  movimientos[k]=combo(732,y,170,510+k);
  for(auto& mv:movs)SendMessage(movimientos[k],CB_ADDSTRING,0,(LPARAM)mv.texto);
  niveles[k]=combo(912,y,78,520+k);
  for(float db:nivelesDb){auto s=db==0?std::wstring(L"0 dB"):L"−"+std::to_wstring(int(-db))+L" dB";SendMessage(niveles[k],CB_ADDSTRING,0,(LPARAM)s.c_str());}
  auto& o=engine.core.espacio.obj[k];
  if(!engine.running())o.posicion=float(pli::normaliza(o.azimut));
 }
 toggle=child(L"BUTTON",L"",552,540,438,44,600,BS_OWNERDRAW|WS_TABSTOP);
 child(L"BUTTON",L"↻  Recargar ficha",24,612,200,40,601,BS_OWNERDRAW|WS_TABSTOP);
 child(L"BUTTON",L"Guardar objetos",236,612,180,40,602,BS_OWNERDRAW|WS_TABSTOP);
 child(L"BUTTON",L"Cargar objetos",428,612,180,40,603,BS_OWNERDRAW|WS_TABSTOP);
 sincronizaFilas();
 info=child(L"STATIC",L"",624,600,370,112,0,0);
 SendMessage(info,WM_SETFONT,(WPARAM)smallFont,TRUE);
 SetTimer(panel,1,40,nullptr);
 sync();ShowWindow(panel,SW_SHOW);
}
}
