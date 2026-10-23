// PLI Espacial 7.1 — intermediario entre el panel (popup) y el documento oculto donde corre el audio.
// Chrome solo permite procesar audio de pestañas en un documento «offscreen».
async function asegurarOffscreen() {
  if (await chrome.offscreen.hasDocument()) return;
  await chrome.offscreen.createDocument({
    url: 'offscreen.html',
    reasons: ['USER_MEDIA'],
    justification: 'Procesar el audio de la pestaña y sacarlo en 7.1',
  });
}

chrome.runtime.onMessage.addListener((msg, _sender, responder) => {
  if (msg.para !== 'fondo') return false;
  (async () => {
    try {
      if (msg.tipo === 'iniciar' || msg.tipo === 'probar') {
        await asegurarOffscreen();
        responder(await chrome.runtime.sendMessage({ ...msg, para: 'offscreen' }));
      } else if (await chrome.offscreen.hasDocument()) {
        responder(await chrome.runtime.sendMessage({ ...msg, para: 'offscreen' }));
      } else {
        responder({ ok: true, activo: false });
      }
    } catch (e) {
      responder({ ok: false, error: String(e && e.message || e) });
    }
  })();
  return true;  // respuesta asíncrona
});
