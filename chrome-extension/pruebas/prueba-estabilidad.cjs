const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict'),path=require('node:path');
const root=path.join(__dirname,'..');
function transporte(){
 let now=0,id=0;const timers=new Map(),sockets=[];
 class Socket{constructor(){this.readyState=0;this.bufferedAmount=0;this.sent=[];sockets.push(this);}send(x){if(this.fail)throw Error('Socket cerrado');this.sent.push(x);}close(){this.readyState=3;this.onclose?.();}}
 const box={Float32Array,ArrayBuffer,DataView,WebSocket:Socket,Date:{now:()=>now},setTimeout:(f,ms)=>{timers.set(++id,{f,at:now+ms});return id;},clearTimeout:i=>timers.delete(i)};
 vm.createContext(box);vm.runInContext(fs.readFileSync(path.join(root,'puente.js'),'utf8'),box);
 const tick=ms=>{now+=ms;for(const [i,t] of [...timers])if(t.at<=now){timers.delete(i);t.f();}};
 const c=new box.PLIPuente.Conexion(),pcm=new Float32Array(8192);const first=sockets.at(-1);
 tick(4001);assert.equal(first.readyState,3);assert.equal(c.estado,'reconectando');tick(500);
 const s=sockets.at(-1);s.readyState=1;s.onopen();c.envia(pcm,48000);assert.equal(s.sent.length,1);
 const v=new DataView(s.sent[0]);assert.equal(v.getUint32(4,true),48000);assert.equal(v.getUint32(8,true),1024);
 s.bufferedAmount=65537;c.envia(pcm,48000);tick(2001);c.envia(pcm,48000);assert.equal(s.readyState,3);assert.equal(c.estado,'reconectando');
 tick(500);const t=sockets.at(-1);t.readyState=1;t.onopen();t.fail=true;assert.doesNotThrow(()=>c.envia(pcm,48000));assert.equal(c.estado,'reconectando');
 for(let i=0;i<30;i++){tick(5000);sockets.at(-1).onerror?.();}
 const count=sockets.length;c.cierra();tick(10000);assert.equal(sockets.length,count);assert.equal(timers.size,0);
 console.log('PASA: timeout de conexión, reconexión prolongada, cola atascada, excepción de envío y cancelación.');
}
async function sesiones(){
 let listener,created=0;const streams=[],contexts=[];let timerId=0;
 class Context{
  constructor(options){this.options=options;this.sinkId=options.sinkId;this.destination={maxChannelCount:8,channelCount:2};this.state='suspended';this.sampleRate=48000;this.currentTime=0;this.audioWorklet={addModule:async()=>{}};contexts.push(this);}
  async resume(){this.state='running';}async close(){this.state='closed';}setSinkId(){}createMediaStreamSource(){return {};}
 }
 class Motor{constructor(){this.analisis={datos:{}};this.modo='envolvente';this.maestro={disconnect(){},connect(){}};}configura(a){Object.assign(this,a);}avanza(){}iniciaGiro(){}}
 class Conexion{constructor(){this.estado='conectado';this.enviados=0;}cierra(){this.estado='detenido';}envia(){this.enviados++;}}
 class Worklet{constructor(){this.port={postMessage(){}};}connect(){}disconnect(){}}
 const box={console,Date,Promise,performance:{now:()=>0},setTimeout,clearTimeout,setInterval:()=>++timerId,clearInterval(){},AudioContext:Context,AudioWorkletNode:Worklet,PLIMotor:{Motor},PLIPuente:{Conexion,ID_LEGADO:'original'},chrome:{runtime:{id:'original',onMessage:{addListener:f=>listener=f}}},navigator:{mediaDevices:{getUserMedia:async()=>{
  created++;await new Promise(r=>setTimeout(r,5));const track={stopped:false,onended:null,stop(){this.stopped=true;}};const s={getTracks:()=>[track],getAudioTracks:()=>[track]};streams.push(s);return s;
 }}}};
 vm.createContext(box);vm.runInContext(fs.readFileSync(path.join(root,'offscreen.js'),'utf8'),box);
 const send=m=>new Promise(resolve=>listener({...m,para:'offscreen'},null,resolve));
 const first=await send({tipo:'iniciar',streamId:'1',tabId:1,ajustes:{destino:'app'}});assert.equal(first.contexto,'running');assert.equal(contexts[0].options.sinkId.type,'none');
 const oldEnded=streams[0].getTracks()[0].onended;
 await Promise.all([send({tipo:'reconectar'}),send({tipo:'ajustes',ajustes:{modo:'inteligente'}})]);
 assert.equal(created,1);assert.equal((await send({tipo:'estado'})).modo,'inteligente');assert.equal(contexts.filter(c=>c.state!=='closed').length,1);
 const denied=await send({tipo:'ajustes',ajustes:{destino:'directo'}});assert.equal(denied.ok,false);assert.equal((await send({tipo:'estado'})).destino,'app');
 await send({tipo:'iniciar',streamId:'2',tabId:2,ajustes:{destino:'app'}});oldEnded();await new Promise(r=>setTimeout(r,10));assert.equal((await send({tipo:'estado'})).tabId,2);
 assert.equal(contexts.filter(c=>c.state!=='closed').length,1);assert.ok(contexts.every(c=>c.options.sinkId?.type==='none'));
 await Promise.all([send({tipo:'detener'}),send({tipo:'detener'})]);assert.ok(streams.every(s=>s.getTracks()[0].stopped));assert.ok(contexts.every(c=>c.state==='closed'));
 console.log('PASA: un contexto por captura, reconectar sin recapturar, comandos simultáneos, evento antiguo y protección del modo conjunto.');
 let n=0,doc=false;const sw={chrome:{offscreen:{hasDocument:async()=>doc,createDocument:async()=>{n++;await new Promise(r=>setTimeout(r,5));doc=true;}},runtime:{onMessage:{addListener(){}}}}};vm.createContext(sw);vm.runInContext(fs.readFileSync(path.join(root,'service-worker.js'),'utf8'),sw);
 await vm.runInContext('Promise.all([asegurarOffscreen(),asegurarOffscreen(),asegurarOffscreen()])',sw);assert.equal(n,1);
 console.log('PASA: creación concurrente del documento offscreen sin duplicados.');
}
transporte();sesiones().catch(e=>{console.error(e);process.exitCode=1;});
