// PLI Espacial 7.1 — panel de la extensión.
const $ = (id) => document.getElementById(id);
const enviar = (msg) => chrome.runtime.sendMessage({ ...msg, para: 'fondo' });

function ajustes() {
  return { modo: $('modo').value, rpm: Number($('rpm').value), nivelDb: Number($('nivel').value), lfe: $('lfe').checked };
}
function guardar() { chrome.storage.local.set({ ajustes: ajustes() }); }

function pinta(e, tabActual) {
  const el = $('estado');
  el.className = 'estado';
  if (!e || !e.ok) { el.textContent = 'Error: ' + (e && e.error || 'sin respuesta'); el.classList.add('aviso'); return; }
  if (!e.activo) { el.textContent = 'Apagado. Abre la pestaña de la música y pulsa Activar.'; $('activar').textContent = '▶ Activar en esta pestaña'; return; }
  const aqui = e.tabId === tabActual;
  $('activar').textContent = aqui ? '■ Detener' : '▶ Pasar a esta pestaña';
  if (e.canalesMax < 8) {
    el.textContent = `Activo, pero la salida de Windows tiene ${e.canalesMax} canales: ponla en 7.1 (Panel de sonido › Configurar).`;
    el.classList.add('aviso');
  } else {
    el.textContent = `● En vivo · ${e.canalesSalida} canales · ${Math.round(e.frecuencia / 1000)} kHz${aqui ? '' : ' · en otra pestaña'}`;
    el.classList.add('vivo');
  }
}

(async () => {
  const [tab] = await chrome.tabs.query({ active: true, currentWindow: true });
  const { ajustes: a } = await chrome.storage.local.get('ajustes');
  if (a) { $('modo').value = a.modo; $('rpm').value = String(a.rpm); $('nivel').value = a.nivelDb; $('lfe').checked = !!a.lfe; }
  $('nivelTxt').textContent = `${$('nivel').value === '0' ? '0' : '−' + Math.abs($('nivel').value)} dB`;
  let e = await enviar({ tipo: 'estado' });
  pinta(e, tab.id);

  $('activar').onclick = async () => {
    if (e && e.activo && e.tabId === tab.id) { e = await enviar({ tipo: 'detener' }); pinta(e, tab.id); return; }
    $('estado').textContent = 'Activando…';
    try {
      const streamId = await chrome.tabCapture.getMediaStreamId({ targetTabId: tab.id });
      e = await enviar({ tipo: 'iniciar', streamId, ajustes: ajustes(), tabId: tab.id });
    } catch (err) { e = { ok: false, error: String(err && err.message || err) }; }
    pinta(e, tab.id);
  };
  const cambia = async () => { guardar(); $('nivelTxt').textContent = `${$('nivel').value === '0' ? '0' : '−' + Math.abs($('nivel').value)} dB`; if (e && e.activo) e = await enviar({ tipo: 'ajustes', ajustes: ajustes() }); };
  ['modo', 'rpm', 'lfe'].forEach((id) => $(id).onchange = cambia);
  $('nivel').oninput = cambia;
  $('probar').onclick = async () => {
    const r = await enviar({ tipo: 'probar' });
    $('estado').textContent = r && r.ok ? `Probando ${r.canales} canales: FL, FR, C, LFE, BL, BR, SL, SR (≈ 1 s cada uno)` : 'No se pudo probar: ' + (r && r.error);
  };
})();
