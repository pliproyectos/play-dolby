// Ejecuta solamente el grafo silencioso en un Chrome headless de perfil temporal.
const fs = require('node:fs'), os = require('node:os'), path = require('node:path');
const { spawn } = require('node:child_process');
const chrome = process.env.CHROME_PATH || 'C:/Program Files/Google/Chrome/Application/chrome.exe';
const profile = fs.mkdtempSync(path.join(os.tmpdir(), 'pli-puente-test-'));
const sleep = ms => new Promise(r => setTimeout(r, ms));
const child = spawn(chrome, ['--headless', '--disable-gpu', '--no-first-run', '--no-default-browser-check',
  '--autoplay-policy=no-user-gesture-required', '--allow-file-access-from-files', '--remote-debugging-port=0', '--user-data-dir=' + profile],
  { windowsHide: true, stdio: 'ignore' });
let ws;
(async () => {
  const deadline = Date.now() + 10000;
  while (!fs.existsSync(path.join(profile, 'DevToolsActivePort'))) { if (Date.now() > deadline) throw Error('Chrome no inició'); await sleep(50); }
  const port = fs.readFileSync(path.join(profile, 'DevToolsActivePort'), 'utf8').split('\n')[0];
  const url = require('node:url').pathToFileURL(path.join(__dirname, 'prueba-puente-browser.html')).href;
  const target = await (await fetch(`http://127.0.0.1:${port}/json/new?${encodeURIComponent(url)}`, { method: 'PUT' })).json();
  ws = new WebSocket(target.webSocketDebuggerUrl);
  await new Promise((resolve, reject) => { ws.onopen = resolve; ws.onerror = reject; });
  let id = 0; const pending = new Map();
  ws.onmessage = event => { const msg = JSON.parse(event.data); const p = pending.get(msg.id); if (p) { pending.delete(msg.id); msg.error ? p.reject(Error(msg.error.message)) : p.resolve(msg.result); } };
  const call = (method, params) => new Promise((resolve, reject) => { pending.set(++id, { resolve, reject }); ws.send(JSON.stringify({ id, method, params })); });
  await sleep(150);
  const result = await call('Runtime.evaluate', { expression: `new Promise((resolve) => { const poll = () => { const text = document.querySelector('#resultado')?.textContent; if (text && text !== 'ESPERANDO') resolve(text); else setTimeout(poll, 50); }; poll(); })`, awaitPromise: true, returnByValue: true, timeout: 7000 });
  const text = result.result.value; console.log(text); if (!text || !text.startsWith('PASA:')) process.exitCode = 1;
  const browser = await (await fetch(`http://127.0.0.1:${port}/json/version`)).json();
  console.log('Navegador: ' + browser.Browser);
})().catch(e => { console.error(e); process.exitCode = 1; }).finally(() => { if (ws) ws.close(); child.kill(); });
