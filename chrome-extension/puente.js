// Transporte local PLI1: una conexión, sin cola propia de audio.
(function(global){
'use strict';
const URL='ws://127.0.0.1:19771/pli-audio',ID_LEGADO='blhfjpfedgoeolfcpejpehpikainihpd';
function paquete(pcm,rate,secuencia){
 if(!(pcm instanceof Float32Array)||pcm.length%8||pcm.length<128*8||pcm.length>2048*8||!Number.isInteger(rate)||rate<8000||rate>192000)throw Error('Bloque PCM no válido');
 const b=new ArrayBuffer(16+pcm.byteLength),v=new DataView(b);
 [80,76,73,49].forEach((x,i)=>v.setUint8(i,x));v.setUint32(4,rate,true);v.setUint32(8,pcm.length/8,true);v.setUint32(12,secuencia>>>0,true);
 for(let i=0;i<pcm.length;i++)v.setFloat32(16+i*4,Number.isFinite(pcm[i])?pcm[i]:0,true);return b;
}
class Conexion{
 constructor(){this.socket=null;this.timer=null;this.plazo=null;this.cerrada=false;this.intentos=0;this.estado='conectando';this.descartados=0;this.secuencia=0;this.enviados=0;this.reconexiones=0;this.bloqueadoDesde=null;this.ultimoEnvio=0;this.conecta();}
 conecta(){
  if(this.cerrada)return;clearTimeout(this.timer);this.timer=null;this.estado=this.intentos?'reconectando':'conectando';
  let s;try{s=this.socket=new WebSocket(URL);}catch(_){this.reintenta();return;}
  this.plazo=setTimeout(()=>{if(this.socket===s&&s.readyState!==1)this.falla(s);},4000);
  s.onopen=()=>{if(this.cerrada||this.socket!==s){s.close();return;}clearTimeout(this.plazo);this.plazo=null;this.intentos=0;this.estado='conectado';this.bloqueadoDesde=null;};
  s.onerror=()=>this.falla(s);s.onclose=()=>this.falla(s);
 }
 falla(s){
  if(this.cerrada||this.socket!==s)return;clearTimeout(this.plazo);this.plazo=null;this.socket=null;
  s.onopen=s.onclose=s.onerror=null;try{s.close();}catch(_){}this.reintenta();
 }
 reintenta(){
  if(this.cerrada||this.timer)return;this.estado='reconectando';this.reconexiones++;
  const demora=Math.min(500*2**this.intentos,5000);this.intentos=Math.min(this.intentos+1,4);
  this.timer=setTimeout(()=>{this.timer=null;this.conecta();},demora);
 }
 envia(pcm,rate){
  const secuencia=this.secuencia++,s=this.socket;
  if(!s||s.readyState!==1){this.descartados++;return;}
  if(s.bufferedAmount>65536){this.descartados++;if(this.bloqueadoDesde===null)this.bloqueadoDesde=Date.now();if(Date.now()-this.bloqueadoDesde>2000)this.falla(s);return;}
  this.bloqueadoDesde=null;
  try{s.send(paquete(pcm,rate,secuencia));this.enviados++;this.ultimoEnvio=Date.now();}catch(_){this.descartados++;this.falla(s);}
 }
 cierra(){
  this.cerrada=true;clearTimeout(this.timer);clearTimeout(this.plazo);this.timer=this.plazo=null;
  const s=this.socket;this.socket=null;if(s){s.onopen=s.onclose=s.onerror=null;try{s.close();}catch(_){}}this.estado='detenido';
 }
}
global.PLIPuente={paquete,Conexion,URL,ID_LEGADO};
})(globalThis);
