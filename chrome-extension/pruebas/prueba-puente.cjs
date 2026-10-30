// node pruebas/prueba-puente.cjs — sin Chrome, dispositivos ni servidor.
const fs = require('node:fs'), vm = require('node:vm'), assert = require('node:assert/strict');
const path = require('node:path'), root = path.join(__dirname, '..');
let temporizadores = [], sockets = [];
class Socket {
  constructor(url) { this.url = url; this.readyState = 0; this.bufferedAmount = 0; this.enviados = []; sockets.push(this); }
  send(b) { this.enviados.push(b); }
  close() { this.readyState = 3; if (this.onclose) this.onclose(); }
}
const sandbox = { Float32Array, ArrayBuffer, DataView, WebSocket: Socket,
  setTimeout: (fn, demora) => { const timer = { fn, demora, cancelado: false }; temporizadores.push(timer); return timer; },
  clearTimeout: timer => { if (timer) { timer.cancelado = true; const i = temporizadores.indexOf(timer); if (i >= 0) temporizadores.splice(i, 1); } } };
vm.createContext(sandbox); vm.runInContext(fs.readFileSync(path.join(root, 'puente.js'), 'utf8'), sandbox);
const pcm = new Float32Array(1024 * 8);
for (let f = 0; f < 1024; f++) for (let c = 0; c < 8; c++) pcm[f * 8 + c] = c / 8;
const packet = sandbox.PLIPuente.paquete(pcm, 48000, 42), view = new DataView(packet);
assert.equal(Buffer.from(packet).subarray(0, 4).toString(), 'PLI1');
assert.equal(view.getUint32(4, true), 48000); assert.equal(view.getUint32(8, true), 1024); assert.equal(view.getUint32(12, true), 42);
assert.equal(packet.byteLength, 16 + 1024 * 8 * 4);
for (let c = 0; c < 8; c++) assert.equal(view.getFloat32(16 + c * 4, true), c / 8);
pcm[0] = NaN; assert.equal(new DataView(sandbox.PLIPuente.paquete(pcm, 48000, 0)).getFloat32(16, true), 0);
assert.throws(() => sandbox.PLIPuente.paquete(new Float32Array(8), 48000, 0));
assert.throws(() => sandbox.PLIPuente.paquete(pcm, 200000, 0));
const conexion = new sandbox.PLIPuente.Conexion(), first = sockets[0];
conexion.envia(pcm, 48000); assert.equal(conexion.descartados, 1);
first.readyState = 1; first.onopen(); conexion.envia(pcm, 48000); assert.equal(first.enviados.length, 1);
first.bufferedAmount = 65537; conexion.envia(pcm, 48000); assert.equal(first.enviados.length, 1);
first.close();
// Un arranque largo de la app no agota los reintentos ni acumula audio.
for (let i = 0; i < 30; i++) {
  assert.equal(temporizadores.length, 1);
  const timer = temporizadores.shift();
  assert.equal(timer.demora, Math.min(500 * 2 ** Math.min(i, 4), 5000));
  timer.fn(); conexion.envia(pcm, 48000); sockets.at(-1).close();
}
assert.equal(sockets.length, 31); assert.equal(conexion.estado, 'reconectando');
const conectar = temporizadores.shift(); conectar.fn();
const recuperado = sockets.at(-1); recuperado.readyState = 1; recuperado.onopen();
assert.equal(conexion.estado, 'conectado'); assert.equal(conexion.intentos, 0);
conexion.envia(pcm, 48000); assert.equal(recuperado.enviados.length, 1);
recuperado.close(); assert.equal(temporizadores[0].demora, 500); // Reset tras éxito.
const pendiente = temporizadores[0], totalSockets = sockets.length;
conexion.cierra(); assert.equal(conexion.estado, 'detenido');
assert.equal(pendiente.cancelado, true); pendiente.fn();
assert.equal(sockets.length, totalSockets); // Ni un temporizador ya encolado revive la captura.
let Processor;
const worklet = { Float32Array, sampleRate: 48000, AudioWorkletProcessor: class { constructor() { this.port = { bloques: [], postMessage: (b) => this.port.bloques.push(b) }; } },
  registerProcessor: (_name, ctor) => { Processor = ctor; } };
vm.createContext(worklet); vm.runInContext(fs.readFileSync(path.join(root, 'pcm-worklet.js'), 'utf8'), worklet);
const proc = new Processor(), input = Array.from({ length: 8 }, (_, c) => new Float32Array(128).fill(c / 8));
for (let i = 0; i < 8; i++) { const output = new Float32Array(128).fill(1); assert.equal(proc.process([input], [[output]]), true); assert.ok(output.every(x => x === 0)); }
assert.equal(proc.port.bloques.length, 1); assert.equal(proc.port.bloques[0].rate, 48000);
for (let f = 0; f < 1024; f++) for (let c = 0; c < 8; c++) assert.equal(proc.port.bloques[0].pcm[f * 8 + c], c / 8);
// Un canal ausente produce ceros, sin duplicar el canal frontal ni hacer downmix.
for (let i = 0; i < 8; i++) proc.process([[input[0]]], [[new Float32Array(128)]]);
assert.equal(proc.port.bloques.length, 1); // Sin ACK, no acumula mensajes al hilo principal.
proc.port.onmessage({ data: 'listo' });
for (let i = 0; i < 8; i++) proc.process([[input[0]]], [[new Float32Array(128)]]);
assert.ok(proc.port.bloques[1].pcm.every(x => x === 0));
console.log('PASA: protocolo 8ch, header, saneamiento, cola limitada, reconexión tras arranque lento/reset/cancelación, worklet 8ch y salida silenciosa.');
