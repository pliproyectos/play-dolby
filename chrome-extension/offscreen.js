// PLI Espacial 7.1 — aquí corre el audio: captura la pestaña, la pasa por el motor y la saca en 8 canales.
let ctx = null, flujo = null, motor = null, reloj = null, tabId = null, probando = false;

function estado() {
  return {
    ok: true, activo: !!ctx, tabId,
    canalesSalida: ctx ? ctx.destination.channelCount : 0,
    canalesMax: ctx ? ctx.destination.maxChannelCount : 0,
    modo: motor ? motor.modo : null, frecuencia: ctx ? ctx.sampleRate : 0,
  };
}

async function detener() {
  clearInterval(reloj); reloj = null;
  if (flujo) flujo.getTracks().forEach((t) => t.stop());
  if (ctx) await ctx.close();
  ctx = flujo = motor = null; tabId = null;
}

async function iniciar(streamId, ajustes, tab) {
  await detener();
  flujo = await navigator.mediaDevices.getUserMedia({
    audio: { mandatory: { chromeMediaSource: 'tab', chromeMediaSourceId: streamId } },
    video: false,
  });
  ctx = new AudioContext({ latencyHint: 'playback' });
  const max = ctx.destination.maxChannelCount;
  // 8 canales DISCRETOS en el orden de Windows si la salida predeterminada es 7.1.
  ctx.destination.channelCount = Math.min(8, max);
  ctx.destination.channelCountMode = 'explicit';
  ctx.destination.channelInterpretation = 'discrete';
  const fuente = ctx.createMediaStreamSource(flujo);
  motor = new PLIMotor.Motor(ctx, fuente, ctx.destination);
  motor.configura(ajustes || {});
  let antes = performance.now();
  reloj = setInterval(() => { const ahora = performance.now(); motor.avanza((ahora - antes) / 1000); antes = ahora; }, 40);
  tabId = tab;
  return estado();
}

// Un tono suave (−30 dBFS, 0,8 s) por bocina, en orden FL FR FC LFE BL BR SL SR.
async function probarCanales() {
  if (probando) return { ok: true };
  probando = true;
  const c = new AudioContext();
  const n = Math.min(8, c.destination.maxChannelCount);
  c.destination.channelCount = n; c.destination.channelCountMode = 'explicit'; c.destination.channelInterpretation = 'discrete';
  const merge = c.createChannelMerger(n); merge.connect(c.destination);
  for (let k = 0; k < n; k++) {
    const o = c.createOscillator(), g = c.createGain();
    o.frequency.value = k === 3 ? 60 : 440; g.gain.value = 0;
    o.connect(g); g.connect(merge, 0, k);
    const t = c.currentTime + 0.2 + k * 1.1;
    g.gain.setValueAtTime(0, t); g.gain.linearRampToValueAtTime(0.0316, t + 0.05);
    g.gain.setValueAtTime(0.0316, t + 0.75); g.gain.linearRampToValueAtTime(0, t + 0.8);
    o.start(t); o.stop(t + 0.85);
  }
  setTimeout(() => { c.close(); probando = false; }, (0.4 + n * 1.1) * 1000);
  return { ok: true, canales: n };
}

chrome.runtime.onMessage.addListener((msg, _sender, responder) => {
  if (msg.para !== 'offscreen') return false;
  (async () => {
    try {
      if (msg.tipo === 'iniciar') responder(await iniciar(msg.streamId, msg.ajustes, msg.tabId));
      else if (msg.tipo === 'detener') { await detener(); responder(estado()); }
      else if (msg.tipo === 'ajustes') { if (motor) motor.configura(msg.ajustes); responder(estado()); }
      else if (msg.tipo === 'probar') responder(await probarCanales());
      else responder(estado());
    } catch (e) { responder({ ok: false, error: String(e && e.message || e) }); }
  })();
  return true;
});
