const {chromium}=require('playwright');
const {pathToFileURL}=require('node:url');
const path=require('node:path');
(async()=>{
 const browser=await chromium.launch({executablePath:'C:/Program Files/Google/Chrome/Application/chrome.exe',headless:true,args:['--allow-file-access-from-files']});
 try{
 const page=await browser.newPage();
 await page.goto(pathToFileURL(path.join(__dirname,'prueba-motor.html')).href);
 await page.waitForFunction(()=>['PASA','FALLA','ERROR'].includes(document.title));
 const result=await page.locator('#salida').innerText();console.log(result);if(await page.title()!=='PASA')throw Error('Fallo DSP');
 await page.addInitScript(()=>{
   window.chrome={storage:{local:{get:async()=>({ajustes:{destino:'app',modo:'envolvente'}}),set:async x=>{window.guardado=x;}}},runtime:{sendMessage:async msg=>({ok:true,activo:false})}};
 });
 await page.goto(pathToFileURL(path.join(__dirname,'../espacial.html')).href);
 await page.waitForFunction(()=>document.querySelector('#estado').textContent.includes('YouTube'));
 await page.locator('#activar').click();
 await page.waitForFunction(()=>window.guardado?.ajustes.modo==='inteligente');
 if(!await page.locator('#giro').isDisabled())throw Error('Giro habilitado sin captura');
 await page.screenshot({path:path.join(__dirname,'espacial-verificado.png'),fullPage:true});
 console.log('PASA: panel sin captura, preferencias y botón de giro protegido.');
 await page.evaluate(()=>{chrome.runtime.sendMessage=async msg=>{window.ultimoMensaje=msg;return {ok:true,activo:true,destino:'app',conexion:'conectado',modo:'inteligente',espacial:{bpm:120,confianza:.9,buffer:12,lateral:.4,bandas:[.1,.2,.05],girando:true,angulo:90}};};});
 await page.waitForFunction(()=>document.querySelector('#bpm').textContent==='120');
 await page.locator('#giro').click();
 if(await page.locator('#giro').isDisabled())throw Error('Giro no habilitado con captura');
 await page.locator('#frente').click();
 await page.waitForFunction(()=>window.guardado?.ajustes.modo==='envolvente');
 console.log('PASA: telemetría activa y cambio a Envolvente.');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
