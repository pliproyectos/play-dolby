// Dos paneles comparten la creación del único documento de audio.
let creando=null;
async function asegurarOffscreen(){
 if(!creando)creando=(async()=>{
  if(!await chrome.offscreen.hasDocument())await chrome.offscreen.createDocument({url:'offscreen.html',reasons:['USER_MEDIA'],justification:'Procesar una pestaña y enviar ocho canales a Play Dolby o al receptor.'});
 })().finally(()=>{creando=null;});return creando;
}
chrome.runtime.onMessage.addListener((msg,_sender,responder)=>{
 if(msg.para!=='fondo')return false;
 (async()=>{try{
  if(msg.tipo==='iniciar'||msg.tipo==='probar')await asegurarOffscreen();
  if(await chrome.offscreen.hasDocument())responder(await chrome.runtime.sendMessage({...msg,para:'offscreen'}));
  else responder({ok:true,activo:false});
 }catch(e){responder({ok:false,error:e.message||String(e)});}})();return true;
});
