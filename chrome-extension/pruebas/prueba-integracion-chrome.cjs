// Chrome real, captura sintética y servidor de prueba local. No abre dispositivos físicos.
const {chromium}=require('playwright'),assert=require('node:assert/strict');
const {pathToFileURL}=require('node:url'),path=require('node:path');
let WebSocketServer;
try{WebSocketServer=require('ws').WebSocketServer;}catch(_){WebSocketServer=require(path.join(path.dirname(require.resolve('playwright-core')),'lib/utilsBundle.js')).wsServer;}
(async()=>{
 const wss=new WebSocketServer({host:'127.0.0.1',port:19771});let browser,paquetes=0,conexiones=0,senal=false,ultima=[];
 await new Promise((resolve,reject)=>{wss.once('listening',resolve);wss.once('error',reject);});
 wss.on('connection',s=>{conexiones++;s.on('message',b=>{
  assert.equal(b.subarray(0,4).toString(),'PLI1');assert.equal(b.readUInt32LE(4),48000);assert.equal(b.readUInt32LE(8),1024);
  paquetes++;let l=0,r=0,otros=0;
  for(let f=0;f<1024;f++){l+=Math.abs(b.readFloatLE(16+(f*8)*4));r+=Math.abs(b.readFloatLE(16+(f*8+1)*4));for(let c=2;c<8;c++)otros+=Math.abs(b.readFloatLE(16+(f*8+c)*4));}
  ultima=[l,r,otros];if(l>1&&r>1&&otros<.001)senal=true;
 });});
 try{
  browser=await chromium.launch({executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe',headless:true,args:['--allow-file-access-from-files','--autoplay-policy=no-user-gesture-required']});
  const p=await browser.newPage();await p.addInitScript(()=>{
   window.chrome={runtime:{id:'blhfjpfedgoeolfcpejpehpikainihpd',onMessage:{addListener:f=>window.receptor=f}}};
   window.mensaje=m=>new Promise(resolve=>receptor({...m,para:'offscreen'},null,resolve));
   navigator.mediaDevices.getUserMedia=async()=>{
    const c=window.fuenteTest=new AudioContext({sampleRate:48000,sinkId:{type:'none'}}),merge=c.createChannelMerger(2),dest=c.createMediaStreamDestination();
    [440,880].forEach((freq,i)=>{const o=c.createOscillator(),g=c.createGain();o.frequency.value=freq;g.gain.value=.05;o.connect(g);g.connect(merge,0,i);o.start();});
    merge.connect(dest);await c.resume();return dest.stream;
   };
  });
  await p.goto(pathToFileURL(path.join(__dirname,'../offscreen.html')).href);
  let r=await p.evaluate(()=>mensaje({tipo:'iniciar',streamId:'sintetico',tabId:1,ajustes:{destino:'app',modo:'frente',nivelDb:-6}}));assert.equal(r.ok,true,JSON.stringify(r));
  const esperaAudio=async()=>{const limite=Date.now()+15000;while(Date.now()<limite){const e=await p.evaluate(()=>mensaje({tipo:'estado'}));if(e.enviados>40)return;await new Promise(r=>setTimeout(r,100));}throw Error('No llegaron bloques suficientes');};
  await esperaAudio();
  assert.equal(senal,true,JSON.stringify(ultima));
  for(let i=0;i<3;i++){
   const antes=conexiones;for(const s of wss.clients)s.terminate();
   await new Promise(r=>setTimeout(r,1300));assert.ok(conexiones>antes,'Debe reconectar tras caída del servidor');
  }
  r=await p.evaluate(()=>mensaje({tipo:'reconectar'}));assert.equal(r.ok,true);
  await esperaAudio();
  const d=await p.evaluate(()=>mensaje({tipo:'diagnostico'}));assert.equal(d.destino,'app');assert.equal(d.contexto,'running');
  await p.evaluate(async()=>{await mensaje({tipo:'detener'});await fuenteTest.close();});
  console.log('PASA: Chrome real, ocho canales PLI1, señal estéreo al frente, tres reconexiones de red y reconstrucción de audio sin hardware. Paquetes='+paquetes);
 }finally{if(browser)await browser.close();for(const s of wss.clients)s.terminate();await new Promise(r=>wss.close(r));}
})().catch(e=>{console.error(e);process.exitCode=1;});
