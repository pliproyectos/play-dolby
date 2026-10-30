'use strict';
let ctx=null,flujo=null,motor=null,reloj=null,tabId=null,puente=null,extractor=null;
let destino='app',ajustes={destino:'app',modo:'envolvente',rpm:3,nivelDb:-6,lfe:false};
let cola=Promise.resolve(),ocupado=0,ultimoError='',ultimoPCM=0,ultimaRevision=0,recuperaciones=[];
let recuperando=false,falloProcesador=false,prueba=null;
const sucesos=[];
function anota(mensaje){sucesos.push({hora:new Date().toISOString(),mensaje});if(sucesos.length>30)sucesos.shift();}
function encolar(fn){ocupado++;const p=cola.then(fn);cola=p.catch(()=>{});return p.finally(()=>{ocupado--;});}
function estado(){
 return {ok:true,activo:!!flujo,ocupado:ocupado>0,tabId,destino,ajustes:{...ajustes},
 canalesSalida:ctx?ctx.destination.channelCount:0,canalesMax:ctx?ctx.destination.maxChannelCount:0,
 modo:motor?motor.modo:ajustes.modo,frecuencia:ctx?ctx.sampleRate:0,contexto:ctx?ctx.state:null,
 conexion:puente?puente.estado:null,descartados:puente?puente.descartados:0,
 enviados:puente?puente.enviados:0,reconexiones:puente?puente.reconexiones:0,
 idExtension:chrome.runtime.id,idEsperado:PLIPuente.ID_LEGADO,ultimoError,segundosSinPCM:ultimoPCM?(Date.now()-ultimoPCM)/1000:null,
 espacial:motor?{...motor.analisis.datos,angulo:motor.angulo,girando:!!motor.giro,intensidad:motor.intensidad,automatico:motor.automatico}:null};
}
async function cerrarGrafo(){
 clearInterval(reloj);reloj=null;
 if(puente){puente.cierra();puente=null;}
 if(extractor){extractor.port.onmessage=null;extractor.onprocessorerror=null;extractor.disconnect();extractor=null;}
 const anterior=ctx;ctx=null;motor=null;ultimoPCM=0;falloProcesador=false;
 if(anterior){anterior.onstatechange=null;if(anterior.state!=='closed')await anterior.close();}
}
async function detener(){
 const anterior=flujo;flujo=null;tabId=null;
 if(anterior)for(const t of anterior.getTracks()){t.onended=null;t.stop();}
 await cerrarGrafo();ultimoError='';anota('Captura detenida.');
}
async function construir(){
 if(!flujo)throw Error('Activa primero la captura de la pestaña.');
 await cerrarGrafo();destino=ajustes.destino==='directo'?'directo':'app';
 try{
  if(destino==='app'&&typeof AudioContext.prototype.setSinkId!=='function')throw Error('Actualiza Chrome para mezclar con la Qu-5.');
  const c=ctx=new AudioContext(destino==='app'?{latencyHint:'playback',sampleRate:48000,sinkId:{type:'none'}}:{latencyHint:'playback'});
  let salida=c.destination;
  if(destino==='app'){
   if(!c.sinkId||c.sinkId.type!=='none')throw Error('Chrome no pudo crear la salida silenciosa para Play Dolby.');
   await c.audioWorklet.addModule('pcm-worklet.js');
   const nodo=extractor=new AudioWorkletNode(c,'pli-pcm',{numberOfInputs:1,numberOfOutputs:1,outputChannelCount:[1],channelCount:8,channelCountMode:'explicit',channelInterpretation:'discrete'});
   const conexion=puente=new PLIPuente.Conexion();
   nodo.port.onmessage=({data})=>{
    if(ctx!==c||extractor!==nodo)return;ultimoPCM=Date.now();
    try{conexion.envia(data.pcm,data.rate);}finally{nodo.port.postMessage('listo');}
   };
   nodo.onprocessorerror=()=>{if(extractor===nodo){falloProcesador=true;anota('El procesador de Chrome pidió recuperación.');}};
   nodo.connect(c.destination);salida=nodo;ultimoPCM=Date.now();
  }else{
   c.destination.channelCount=Math.min(8,c.destination.maxChannelCount);c.destination.channelCountMode='explicit';c.destination.channelInterpretation='discrete';
  }
  motor=new PLIMotor.Motor(c,c.createMediaStreamSource(flujo),salida);motor.configura(ajustes);
  await c.resume();let antes=performance.now();ultimaRevision=Date.now();
  reloj=setInterval(()=>{
   if(ctx!==c||!motor)return;
   const ahora=performance.now();motor.avanza(Math.min(.25,Math.max(0,(ahora-antes)/1000)));antes=ahora;
   if(Date.now()-ultimaRevision>=1000){ultimaRevision=Date.now();vigilar();}
  },40);
  ultimoError='';anota(destino==='app'?'Chrome envía a Play Dolby; HDMI reservado a la app.':'Chrome en salida independiente de Windows.');
 }catch(e){await cerrarGrafo();ultimoError=e.message;throw e;}
}
function vigilar(){
 if(ocupado||recuperando||!ctx||!flujo)return;
 if(ctx.state==='running'&&!falloProcesador&&!(destino==='app'&&Date.now()-ultimoPCM>4000))return;
 recuperaciones=recuperaciones.filter(t=>Date.now()-t<60000);
 if(recuperaciones.length>=3){ultimoError='Chrome no recupera el audio. Pulsa Reconectar Chrome y revisa el puente del EXE.';return;}
 recuperando=true;recuperaciones.push(Date.now());
 encolar(async()=>{
  if(!ctx||!flujo)return;anota('Recuperación del procesamiento de Chrome.');
  if(ctx.state==='suspended'&&!falloProcesador)await ctx.resume();else await construir();
 }).catch(e=>{ultimoError=e.message;}).finally(()=>{recuperando=false;});
}
async function capturar(streamId){
 let expiro=false,timer;
 const permiso=navigator.mediaDevices.getUserMedia({audio:{mandatory:{chromeMediaSource:'tab',chromeMediaSourceId:streamId}},video:false});
 permiso.then(s=>{if(expiro)s.getTracks().forEach(t=>t.stop());},()=>{});
 try{return await Promise.race([permiso,new Promise((_,reject)=>{timer=setTimeout(()=>{expiro=true;reject(Error('La captura no respondió. Vuelve a pulsar Activar en la pestaña de música.'));},10000);})]);}
 finally{clearTimeout(timer);}
}
async function iniciar(msg){
 if(prueba){await prueba.close();prueba=null;}
 await detener();ajustes={...ajustes,...msg.ajustes};recuperaciones=[];
 try{
  const s=flujo=await capturar(msg.streamId);tabId=msg.tabId;
  for(const t of s.getAudioTracks())t.onended=()=>{if(flujo===s)encolar(async()=>{if(flujo===s)await detener();}).catch(()=>{});};
  await construir();return estado();
 }catch(e){await detener();ultimoError=e.message;throw e;}
}
async function cambiar(o){
 if(o.destino&&o.destino!=='app'&&o.destino!=='directo')throw Error('Salida no válida.');
 const cambiaSalida=o.destino&&o.destino!==destino;
 if(cambiaSalida&&o.destino==='directo'&&puente?.estado==='conectado')throw Error('Con la Qu-5 conserva «Con Qu-5 / Play Dolby». Detén primero el puente del EXE para usar Chrome solo.');
 ajustes={...ajustes,...o};
 if(flujo){if(cambiaSalida||!ctx)await construir();else motor.configura(ajustes);}return estado();
}
async function probarCanales(msg){
 if((flujo?destino:msg.destino)!=='directo')throw Error('En modo conjunto usa el probador de Play Dolby.');
 if(flujo)throw Error('Detén la captura antes de probar canales.');
 if(prueba)throw Error('Ya hay una prueba de canales en curso.');
 const c=prueba=new AudioContext();
 try{
  const n=Math.min(8,c.destination.maxChannelCount);c.destination.channelCount=n;c.destination.channelCountMode='explicit';c.destination.channelInterpretation='discrete';
  const merge=c.createChannelMerger(n);merge.connect(c.destination);
  for(let k=0;k<n;k++){
   const o=c.createOscillator(),g=c.createGain(),t=c.currentTime+.2+k*1.1;
   o.frequency.value=k===3?60:440;g.gain.value=0;o.connect(g);g.connect(merge,0,k);
   g.gain.setValueAtTime(0,t);g.gain.linearRampToValueAtTime(.0316,t+.05);g.gain.setValueAtTime(.0316,t+.75);g.gain.linearRampToValueAtTime(0,t+.8);o.start(t);o.stop(t+.85);
  }
  await c.resume();setTimeout(()=>{if(prueba===c){prueba=null;c.close().catch(()=>{});}},(n*1.1+.4)*1000);return {ok:true,canales:n};
 }catch(e){prueba=null;await c.close();throw e;}
}
chrome.runtime.onMessage.addListener((msg,_sender,responder)=>{
 if(msg.para!=='offscreen')return false;
 if(msg.tipo==='estado'){responder(estado());return false;}
 if(msg.tipo==='diagnostico'){responder({...estado(),sucesos:sucesos.slice()});return false;}
 encolar(async()=>{
  if(msg.tipo==='iniciar')return iniciar(msg);
  if(msg.tipo==='detener'){await detener();return estado();}
  if(msg.tipo==='ajustes')return cambiar(msg.ajustes||{});
  if(msg.tipo==='salidaDirecta')return cambiar({destino:'directo'});
  if(msg.tipo==='reconectar'){recuperaciones=[];await construir();return estado();}
  if(msg.tipo==='giro'){if(motor)motor.iniciaGiro();return estado();}
  if(msg.tipo==='probar')return probarCanales(msg);
  return estado();
 }).then(responder,e=>{ultimoError=e.message||String(e);anota(ultimoError);responder({ok:false,error:ultimoError});});
 return true;
});
