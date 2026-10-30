'use strict';
const $=id=>document.getElementById(id);
const enviar=msg=>chrome.runtime.sendMessage({...msg,para:'fondo'});
let estado=null,ocupado=false;
for(const [nombre,a] of [['FL',-30],['C',0],['FR',30],['SR',100],['BR',145],['BL',-145],['SL',-100]]){
  const el=document.createElement('span');el.className='speaker';el.textContent=nombre;
  el.style.left=(50+46*Math.sin(a*Math.PI/180))+'%';el.style.top=(50-46*Math.cos(a*Math.PI/180))+'%';$('domo').append(el);
}
async function aplicar(cambios){
  ocupado=true;
  try{
    const {ajustes:prev={}}=await chrome.storage.local.get('ajustes');
    const ajustes={destino:prev.destino||'app',modo:'inteligente',rpm:3,nivelDb:-6,...prev,...cambios};
    if(estado?.activo)ajustes.destino=estado.destino;
    if(estado?.activo){const r=await enviar({tipo:'ajustes',ajustes});if(!r.ok)throw new Error(r.error);}
    await chrome.storage.local.set({ajustes});await refresca();
  }catch(e){$('estado').textContent='No se pudo aplicar: '+e.message;}finally{ocupado=false;}
}
async function refresca(){
  estado=await enviar({tipo:'estado'});if(!estado?.ok)throw new Error(estado?.error||'Sin respuesta');
  const activo=estado.activo,smart=activo&&estado.modo==='inteligente',d=estado.espacial;
  $('estado').textContent=!activo?'Abre YouTube, pulsa la extensión y activa la captura. Puedes dejar preparado este modo aquí.':
    (estado.ultimoError?estado.ultimoError+' ':estado.destino==='app'&&estado.conexion!=='conectado'?'Esperando conexión con Play Dolby. Consulta el diagnóstico en la extensión. ':estado.destino==='directo'&&estado.canalesMax<8?'La salida de Windows no tiene 8 canales. ':'')+(smart?'PLI Espacial activo · voz al frente':'Captura activa · modo '+estado.modo);
  $('giro').disabled=!smart;$('frente').disabled=!activo;
  $('bpm').textContent=d?.bpm||'—';$('confianza').textContent=d?.bpm?Math.round(d.confianza*100)+'%':'—';
  $('buffer').textContent=(d?.buffer||0).toFixed(1)+' s';$('lateral').textContent=Math.round((d?.lateral||0)*100)+'%';
  for(let i=0;i<3;i++)$('b'+i).value=d?.bandas?.[i]||0;
  const a=(d?.angulo||0)*Math.PI/180;
  $('foco').style.left=(50+35*Math.sin(a))+'%';$('foco').style.top=(50-35*Math.cos(a))+'%';$('foco').style.opacity=smart&&d?.girando?1:0;
  $('movimiento').textContent=smart&&d?.girando?'Ambiente en movimiento · señal frontal estable':'Ambiente en reposo';
}
$('activar').onclick=()=>aplicar({modo:'inteligente'});
$('frente').onclick=()=>aplicar({modo:'envolvente'});
$('automatico').onchange=()=>aplicar({automatico:$('automatico').checked});
$('intensidad').oninput=()=>{$('cantidad').textContent=$('intensidad').value+'%';};
$('intensidad').onchange=()=>aplicar({intensidad:Number($('intensidad').value)/100});
$('giro').onclick=async()=>{try{const r=await enviar({tipo:'giro'});if(!r.ok)throw new Error(r.error);await refresca();}catch(e){$('estado').textContent=e.message;}};
(async()=>{try{const {ajustes:a={}}=await chrome.storage.local.get('ajustes');$('intensidad').value=Math.round((a.intensidad??.45)*100);$('cantidad').textContent=$('intensidad').value+'%';$('automatico').checked=a.automatico!==false;await refresca();}catch(e){$('estado').textContent=e.message;}})();
setInterval(()=>{if(!ocupado)refresca().catch(e=>{$('estado').textContent=e.message;});},250);
