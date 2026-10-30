'use strict';
const $=id=>document.getElementById(id),enviar=msg=>chrome.runtime.sendMessage({...msg,para:'fondo'});
let e={ok:true,activo:false},tab=null,ocupado=false;
const defaults={destino:'app',modo:'envolvente',rpm:3,nivelDb:-6,lfe:false};
function ajustes(){return {destino:$('destino').value,modo:$('modo').value,rpm:Number($('rpm').value),nivelDb:Number($('nivel').value),lfe:$('lfe').checked};}
function cargar(a){$('destino').value=a.destino;$('modo').value=a.modo;$('rpm').value=String(a.rpm);$('nivel').value=a.nivelDb;$('lfe').checked=!!a.lfe;$('nivelTxt').textContent=a.nivelDb+' dB';}
function pinta(){
 $('activar').disabled=ocupado;
 $('destino').disabled=ocupado;
 for(const id of ['modo','rpm','nivel','lfe','recuperar'])$(id).disabled=ocupado;
 $('recuperar').hidden=!e.activo;
 $('probar').disabled=ocupado||e.activo||$('destino').value!=='directo';
 $('activar').textContent=e.activo&&e.tabId===tab?.id?'■ Detener':'▶ Activar en esta pestaña';
 const el=$('estado');el.className='estado';
 if(!e.ok){el.textContent=e.error||'No se recibió respuesta.';el.classList.add('aviso');return;}
 if(!e.activo){el.textContent='Apagado. Abre la música y pulsa Activar.';return;}
 if(e.ultimoError){el.textContent=e.ultimoError;el.classList.add('aviso');return;}
 if(e.contexto!=='running'){el.textContent='Recuperando el procesamiento de Chrome…';el.classList.add('aviso');return;}
 if(e.destino==='app'){
  if(e.conexion==='conectado'){el.textContent='Enlace con Play Dolby activo · 8 canales. La salida y la mezcla con Qu-5 se controlan en el EXE.';el.classList.add('vivo');}
  else{
   el.textContent=e.idExtension&&e.idExtension!==e.idEsperado?
    'Sin conexión. El EXE recibido acepta otro identificador de extensión. Consulta el diagnóstico; no cambies a Solo Chrome para mezclar con la Qu-5.':
    'Esperando Play Dolby. Inicia el puente del EXE; la conexión se reintentará automáticamente.';
   el.classList.add('aviso');
  }
 }else{
  el.textContent=e.canalesMax>=8?'Solo Chrome · salida directa de 8 canales. Para sumar la Qu-5, selecciona Con Qu-5 / Play Dolby.':'Solo Chrome · Windows ofrece '+e.canalesMax+' canales. Configura el receptor en 7.1.';
  el.classList.add(e.canalesMax>=8?'vivo':'aviso');
 }
}
async function ejecutar(fn){
 if(ocupado)return;ocupado=true;pinta();
 try{await fn();}catch(err){e={...e,ok:false,error:err.message||String(err)};}
 finally{ocupado=false;pinta();}
}
async function guardar(a){const {ajustes:prev={}}=await chrome.storage.local.get('ajustes');await chrome.storage.local.set({ajustes:{...prev,...a}});}
async function aplicar(){
 const a=ajustes();
 await ejecutar(async()=>{
  if(e.activo){const r=await enviar({tipo:'ajustes',ajustes:a});if(!r.ok){if(e.ajustes)cargar(e.ajustes);throw Error(r.error);}e=r;}
  await guardar(a);$('nivelTxt').textContent=a.nivelDb+' dB';
 });
}
(async()=>{
 try{
  [tab]=await chrome.tabs.query({active:true,currentWindow:true});
  let {ajustes:a,conexionConsolaV3}=await chrome.storage.local.get(['ajustes','conexionConsolaV3']);
  e=await enviar({tipo:'estado'});
  a={...defaults,...a};
  if(!conexionConsolaV3){
   if(!e.activo)a.destino='app';
   await chrome.storage.local.set({ajustes:a,conexionConsolaV3:true});
  }
  cargar(e.activo&&e.ajustes?e.ajustes:a);pinta();
 }catch(err){e={ok:false,error:err.message};pinta();}
 $('espacial').onclick=()=>chrome.tabs.create({url:chrome.runtime.getURL('espacial.html')});
 $('activar').onclick=()=>ejecutar(async()=>{
  if(e.activo&&e.tabId===tab.id){e=await enviar({tipo:'detener'});return;}
  if(!tab?.id||(tab.url&&!/^(https?:)/.test(tab.url)))throw Error('Abre primero una pestaña de música, por ejemplo YouTube.');
  // El token se obtiene dentro del gesto del usuario, antes de esperar al almacenamiento.
  const streamId=await chrome.tabCapture.getMediaStreamId({targetTabId:tab.id});
  const {ajustes:prev={}}=await chrome.storage.local.get('ajustes');
  e=await enviar({tipo:'iniciar',streamId,tabId:tab.id,ajustes:{...prev,...ajustes()}});
 });
 $('recuperar').onclick=()=>ejecutar(async()=>{e=await enviar({tipo:'reconectar'});});
 for(const id of ['destino','modo','rpm','lfe','nivel'])$(id).onchange=aplicar;
 $('nivel').oninput=()=>{$('nivelTxt').textContent=$('nivel').value+' dB';};
 $('probar').onclick=()=>ejecutar(async()=>{
  const r=await enviar({tipo:'probar',destino:$('destino').value});
  if(!r.ok)throw Error(r.error);
  $('detalle').hidden=false;$('detalle').textContent='Probando '+r.canales+' canales: FL FR C SUB BL BR SL SR.';
 });
 $('diagnostico').onclick=async()=>{
  try{
   const d=await enviar({tipo:'diagnostico'});$('detalle').hidden=false;
   $('detalle').textContent=JSON.stringify({extension:chrome.runtime.getManifest().version,id:chrome.runtime.id,...d},null,2);
  }catch(err){$('detalle').hidden=false;$('detalle').textContent=err.message;}
 };
 setInterval(async()=>{
  if(ocupado)return;
  try{e=await enviar({tipo:'estado'});if(e.activo&&e.ajustes&&document.activeElement?.tagName!=='SELECT'&&document.activeElement?.tagName!=='INPUT')cargar(e.ajustes);pinta();}catch(_){}
 },1000);
})();
