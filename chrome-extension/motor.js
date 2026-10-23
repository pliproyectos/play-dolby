// PLI Espacial para Chrome — motor de audio (Web Audio). Convierte el estéreo de una pestaña en 7.1.
// Salida en 8 canales DISCRETOS en el orden de Windows (máscara 0x63F): FL FR FC LFE BL BR SL SR.
// Paneo VBAP 2D, el mismo de PLI Espacial (native/espacial/pli_espacial.hpp), con los ángulos nominales
// de pruebas-domo/domo.json (0° = frente, + = derecha).
// Script normal (no módulo): lo usan offscreen.html y la página de pruebas. Expone window.PLIMotor.
(function (global) {
  'use strict';
  const CANALES = ['FL', 'FR', 'FC', 'LFE', 'BL', 'BR', 'SL', 'SR'];
  const DOMO = [ // canal, azimut
    [2, 0], [1, 30], [7, 100], [5, 145], [4, -145], [6, -100], [0, -30],
  ];

  const norm = (g) => ((g % 360) + 360) % 360;

  // Ganancias VBAP 2D para un ángulo: el sonido se reparte entre las dos bocinas vecinas,
  // con potencia constante. Índice = canal de salida (orden CANALES). El LFE siempre queda en 0.
  function vbap(grados, domo) {
    const b = (domo || DOMO).map(([c, a]) => [c, norm(a)]).sort((x, y) => x[1] - y[1]);
    const th = norm(grados);
    let k = b.length - 1;
    for (let i = 0; i + 1 < b.length; i++) if (th >= b[i][1] && th < b[i + 1][1]) { k = i; break; }
    const A = b[k], C = b[(k + 1) % b.length];
    const v = (d) => [Math.sin(d * Math.PI / 180), Math.cos(d * Math.PI / 180)];
    const la = v(A[1]), lc = v(C[1]), p = v(th);
    const det = la[0] * lc[1] - la[1] * lc[0];
    let ga = Math.max(0, (p[0] * lc[1] - p[1] * lc[0]) / det), gc = Math.max(0, (la[0] * p[1] - la[1] * p[0]) / det);
    const n = Math.hypot(ga, gc) || 1;
    const g = new Array(8).fill(0);
    g[A[0]] += ga / n; g[C[0]] += gc / n;
    return g;
  }

  // Ajustes por modo.
  //  frente     : L → FL, R → FR. Nada más (el estéreo tal cual).
  //  envolvente : L/R al frente, centro suave, ambiente (L−R decorrelado) a laterales y traseras.
  //  orbita     : la mezcla (L y R abiertos ±ancho) gira por el domo; ambiente más bajo.
  const MODOS = {
    frente: { centro: 0, ambiente: 0 },
    envolvente: { centro: 0.35, ambiente: 0.7 },
    orbita: { centro: 0, ambiente: 0.35 },
  };

  class Motor {
    // ctx: AudioContext u OfflineAudioContext · entrada: nodo estéreo · salida: nodo de 8 canales
    constructor(ctx, entrada, salida) {
      this.ctx = ctx;
      this.modo = 'envolvente'; this.rpm = 3; this.velocidad = 1; this.ancho = 30; this.lfe = false;
      this.angulo = 0; this.domo = DOMO;
      const g = (v) => { const n = ctx.createGain(); n.gain.value = v; return n; };
      this.split = ctx.createChannelSplitter(2);
      this.merge = ctx.createChannelMerger(8);
      this.maestro = g(1);
      entrada.connect(this.split);
      // Matriz directa: L y R a cada una de las 8 salidas.
      this.gL = []; this.gR = [];
      for (let c = 0; c < 8; c++) {
        const a = g(0), b = g(0);
        this.split.connect(a, 0); this.split.connect(b, 1);
        a.connect(this.merge, 0, c); b.connect(this.merge, 0, c);
        this.gL.push(a); this.gR.push(b);
      }
      // Medio (L+R)/2 → centro y LFE.
      const mL = g(0.5), mR = g(0.5), medio = g(1);
      this.split.connect(mL, 0); this.split.connect(mR, 1); mL.connect(medio); mR.connect(medio);
      this.gC = g(0); medio.connect(this.gC); this.gC.connect(this.merge, 0, 2);
      const lp1 = ctx.createBiquadFilter(), lp2 = ctx.createBiquadFilter();
      lp1.type = lp2.type = 'lowpass'; lp1.frequency.value = lp2.frequency.value = 80;
      this.gLFE = g(0); medio.connect(lp1); lp1.connect(lp2); lp2.connect(this.gLFE); this.gLFE.connect(this.merge, 0, 3);
      // Ambiente: lado (L−R)/2, sin graves ni brillo, con retardos distintos por bocina (decorrelación).
      const sL = g(0.5), sR = g(-0.5), lado = g(1);
      this.split.connect(sL, 0); this.split.connect(sR, 1); sL.connect(lado); sR.connect(lado);
      const hp = ctx.createBiquadFilter(); hp.type = 'highpass'; hp.frequency.value = 120;
      const lpA = ctx.createBiquadFilter(); lpA.type = 'lowpass'; lpA.frequency.value = 7000;
      lado.connect(hp); hp.connect(lpA);
      this.gAmb = g(0);
      lpA.connect(this.gAmb);
      // canal, retardo (s), signo
      [[6, 0.012, 1], [7, 0.015, -1], [4, 0.021, 1], [5, 0.025, -1]].forEach(([c, d, s]) => {
        const r = ctx.createDelay(0.05); r.delayTime.value = d;
        const k = g(s * 0.7071);
        this.gAmb.connect(r); r.connect(k); k.connect(this.merge, 0, c);
      });
      this.merge.connect(this.maestro); this.maestro.connect(salida);
      this.aplica(true);
    }

    fija(param, v, inmediato) { if (inmediato) param.value = v; else param.setTargetAtTime(v, this.ctx.currentTime, 0.03); }

    // Recalcula ganancias según el modo (y el ángulo actual en órbita).
    aplica(inmediato) {
      const m = MODOS[this.modo] || MODOS.envolvente;
      let gl, gr;
      if (this.modo === 'orbita') { gl = vbap(this.angulo - this.ancho, this.domo); gr = vbap(this.angulo + this.ancho, this.domo); }
      else { gl = vbap(-30, this.domo); gr = vbap(30, this.domo); }
      for (let c = 0; c < 8; c++) { this.fija(this.gL[c].gain, gl[c], inmediato); this.fija(this.gR[c].gain, gr[c], inmediato); }
      this.fija(this.gC.gain, m.centro, inmediato);
      this.fija(this.gAmb.gain, m.ambiente, inmediato);
      this.fija(this.gLFE.gain, this.lfe ? 0.5 : 0, inmediato);
    }

    // Avanza la órbita dt segundos (lo llama un temporizador).
    avanza(dt) {
      if (this.modo !== 'orbita') return;
      this.angulo = norm(this.angulo + this.rpm * 6 * dt * this.velocidad);
      this.aplica(false);
    }

    configura(o) {
      if (o.modo && MODOS[o.modo]) this.modo = o.modo;
      if (typeof o.rpm === 'number') this.rpm = o.rpm;
      if (typeof o.velocidad === 'number') this.velocidad = o.velocidad;
      if (typeof o.lfe === 'boolean') this.lfe = o.lfe;
      if (typeof o.nivelDb === 'number') this.fija(this.maestro.gain, Math.pow(10, o.nivelDb / 20), false);
      this.aplica(false);
    }
  }

  global.PLIMotor = { CANALES, DOMO, vbap, Motor, MODOS };
})(typeof window !== 'undefined' ? window : globalThis);
