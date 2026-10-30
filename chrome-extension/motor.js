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
    envolvente: { centro: 0.22, ambiente: 0.5 },
    inteligente: { centro: 0.22, ambiente: 0.28 },
    orbita: { centro: 0, ambiente: 0.35 },
    orbitaEnvolvente: { centro: 0, ambiente: 0 },
    pasada: { centro: 0, ambiente: 0 },
  };

  // Base estéreo permanente + foco móvil. La suma absoluta por salida no supera 1:
  // incluso con L=R a escala completa la matriz no introduce saturación.
  function orbitaEnvolvente(grados, ancho, domo) {
    const izquierda = vbap(grados - ancho, domo);
    const derecha = vbap(grados + ancho, domo);
    const baseL = [0.72, 0, 0.22, 0, 0.42, 0, 0.48, 0];
    const baseR = [0, 0.72, 0.22, 0, 0, 0.42, 0, 0.48];
    const gl = [], gr = [];
    for (let c = 0; c < 8; c++) {
      const l = baseL[c] + 0.65 * izquierda[c];
      const r = baseR[c] + 0.65 * derecha[c];
      const escala = Math.max(1, l + r);
      gl[c] = l / escala; gr[c] = r / escala;
    }
    return [gl, gr];
  }

  // Historial circular de características: 12 s; no guarda ni retrasa la canción.
  class Ritmo {
    constructor() { this.frames = []; this.picos = []; this.ultimo = -1; this.media = 0; this.anterior = 0; this.datos = { bpm: 0, confianza: 0, buffer: 0, golpe: false, lateral: 0, bandas: [0,0,0] }; }
    agrega(t, bandas, lateral) {
      const energia = bandas[0] + bandas[1] * 0.4;
      const flujo = Math.max(0, energia - this.anterior); this.anterior = energia;
      this.media = this.media * 0.94 + flujo * 0.06;
      const golpe = flujo > Math.max(0.002, this.media * 2.2) && t - this.ultimo > 0.28;
      if (golpe) { this.picos.push(t); this.ultimo = t; }
      this.picos = this.picos.filter(x => t - x < 12);
      this.frames.push({ t, bandas: bandas.slice(), lateral });
      while (this.frames.length > 320 || (this.frames.length && t - this.frames[0].t > 12)) this.frames.shift();
      let bpm = 0, confianza = 0;
      if (this.picos.length >= 5) {
        const intervalos = this.picos.slice(1).map((v,i) => v-this.picos[i]).filter(v=>v>=0.28 && v<=1.3).sort((a,b)=>a-b);
        if (intervalos.length >= 4) {
          const med = intervalos[Math.floor(intervalos.length/2)];
          confianza = intervalos.filter(v=>Math.abs(v-med)<med*0.15).length/intervalos.length;
          if (confianza >= 0.6) bpm = Math.round(60/med);
        }
      }
      this.datos = { bpm, confianza, buffer: this.frames.length ? Math.min(12,t-this.frames[0].t) : 0, golpe, lateral, bandas };
      return this.datos;
    }
  }
  class Analisis extends Ritmo {
    constructor(ctx, medio, lado) {
      super(); this.ctx = ctx;
      this.m = ctx.createAnalyser(); this.s = ctx.createAnalyser();
      for (const n of [this.m,this.s]) { n.fftSize=2048; n.smoothingTimeConstant=0; }
      medio.connect(this.m); lado.connect(this.s);
      this.espectro = new Float32Array(this.m.frequencyBinCount);
      this.tm = new Float32Array(2048); this.ts = new Float32Array(2048);
    }
    actualiza() {
      this.m.getFloatFrequencyData(this.espectro);
      this.m.getFloatTimeDomainData(this.tm); this.s.getFloatTimeDomainData(this.ts);
      const rms = a => Math.sqrt(a.reduce((s,v)=>s+v*v,0)/a.length);
      const m=rms(this.tm), s=rms(this.ts);
      const bandas = [[40,220],[220,2200],[2200,12000]].map(([lo,hi])=>{
        let e=0;
        for(let i=Math.ceil(lo*2048/this.ctx.sampleRate);i<Math.min(this.espectro.length,hi*2048/this.ctx.sampleRate);i++) e+=Math.pow(10,this.espectro[i]/10);
        return Math.sqrt(e);
      });
      return this.agrega(this.ctx.currentTime,bandas,s/(m+s+1e-8));
    }
  }
  class Motor {
    // ctx: AudioContext u OfflineAudioContext · entrada: nodo estéreo · salida: nodo de 8 canales
    constructor(ctx, entrada, salida) {
      this.ctx = ctx;
      this.modo = 'envolvente'; this.rpm = 3; this.velocidad = 1; this.ancho = 30; this.lfe = false;
      this.angulo = 0; this.domo = DOMO; this.intensidad = 0.45; this.automatico = true; this.giro = null; this.ultimoGiro = -20;
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
      // Solo el componente lateral filtrado se mueve; la señal común permanece al frente.
      this.mov = CANALES.map((_, c) => {
        const n = g(0); lpA.connect(n); n.connect(this.merge, 0, c); return n;
      });
      this.analisis = new Analisis(ctx, medio, lado);
      this.merge.connect(this.maestro); this.maestro.connect(salida);
      this.aplica(true);
    }

    fija(param, v, inmediato) { if (inmediato) param.value = v; else param.setTargetAtTime(v, this.ctx.currentTime, 0.03); }

    // Recalcula ganancias según el modo (y el ángulo actual en órbita).
    aplica(inmediato) {
      const m = MODOS[this.modo] || MODOS.envolvente;
      let gl, gr;
      if (this.modo === 'pasada') {
        const fase = norm(this.angulo) / 360;
        const lado = fase < 0.5 ? 1 : -1;
        const recorrido = (fase % 0.5) * 2;
        const impulso = Math.pow(Math.sin(Math.PI * recorrido), 4);
        const direccion = lado * (155 - 130 * recorrido);
        const [movL, movR] = orbitaEnvolvente(direccion, 12, this.domo);
        const [baseL, baseR] = orbitaEnvolvente(0, 30, this.domo);
        gl = baseL.map((v, c) => v * (1 - impulso) + movL[c] * impulso);
        gr = baseR.map((v, c) => v * (1 - impulso) + movR[c] * impulso);
      }
      else if (this.modo === 'orbitaEnvolvente') { [gl, gr] = orbitaEnvolvente(this.angulo, this.ancho, this.domo); }
      else if (this.modo === 'orbita') { gl = vbap(this.angulo - this.ancho, this.domo); gr = vbap(this.angulo + this.ancho, this.domo); }
      else { gl = vbap(-30, this.domo); gr = vbap(30, this.domo); }
      for (let c = 0; c < 8; c++) { this.fija(this.gL[c].gain, gl[c], inmediato); this.fija(this.gR[c].gain, gr[c], inmediato); }
      const foco = vbap(this.angulo, this.domo);
      for (let c = 0; c < 8; c++) this.fija(this.mov[c].gain,
        this.modo === 'inteligente' && this.giro ? foco[c] * this.intensidad * 0.16 : 0, inmediato);
      // Reserva margen para el ambiente móvil, manteniendo la voz en ambos frontales.
      if (this.modo === 'inteligente') {
        this.fija(this.gL[0].gain, 0.82, inmediato); this.fija(this.gR[1].gain, 0.82, inmediato);
      }
      this.fija(this.gC.gain, m.centro, inmediato);
      this.fija(this.gAmb.gain, m.ambiente, inmediato);
      this.fija(this.gLFE.gain, this.lfe ? 0.5 : 0, inmediato);
    }

    // Avanza la órbita dt segundos (lo llama un temporizador).
    avanza(dt) {
      const datos = this.analisis.actualiza();
      if (this.modo === 'inteligente') {
        const t = this.ctx.currentTime;
        if (!this.giro && this.automatico && datos.golpe && datos.lateral > 0.2 && t - this.ultimoGiro > 12) this.iniciaGiro();
        if (this.giro) {
          const fase = (t - this.giro.inicio) / this.giro.duracion;
          this.angulo = norm(360 * Math.min(1, fase));
          if (fase >= 1) this.giro = null;
        }
        this.aplica(false); return;
      }
      if (this.modo !== 'orbita' && this.modo !== 'orbitaEnvolvente' && this.modo !== 'pasada') return;
      this.angulo = norm(this.angulo + this.rpm * 6 * dt * this.velocidad);
      this.aplica(false);
    }

    iniciaGiro() {
      if (this.modo !== 'inteligente') return;
      const bpm = this.analisis.datos.bpm || 100;
      this.giro = { inicio: this.ctx.currentTime, duracion: Math.max(3, Math.min(8, 480 / bpm)) };
      this.ultimoGiro = this.ctx.currentTime;
    }

    configura(o) {
      if (typeof o.intensidad === 'number' && Number.isFinite(o.intensidad)) this.intensidad = Math.max(0, Math.min(1, o.intensidad));
      if (typeof o.automatico === 'boolean') this.automatico = o.automatico;
      if (o.modo && o.modo !== this.modo) this.giro = null;
      if (o.modo && MODOS[o.modo]) this.modo = o.modo;
      if (typeof o.rpm === 'number') this.rpm = o.rpm;
      if (typeof o.velocidad === 'number') this.velocidad = o.velocidad;
      if (typeof o.lfe === 'boolean') this.lfe = o.lfe;
      if (typeof o.nivelDb === 'number') this.fija(this.maestro.gain, Math.pow(10, o.nivelDb / 20), false);
      this.aplica(false);
    }
  }

  global.PLIMotor = { Ritmo, CANALES, DOMO, vbap, orbitaEnvolvente, Motor, MODOS };
})(typeof window !== 'undefined' ? window : globalThis);
