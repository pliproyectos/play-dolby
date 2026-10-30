// Toma 8 canales tras PLIMotor; la salida de este nodo siempre es silencio.
class PLIPCM extends AudioWorkletProcessor {
  constructor() {
    super(); this.pcm = new Float32Array(1024 * 8); this.frames = 0; this.pendiente = false;
    this.port.onmessage = () => { this.pendiente = false; };
  }
  process(inputs, outputs) {
    for (const salida of outputs) for (const canal of salida) canal.fill(0);
    const entrada = inputs[0];
    if (!entrada || !entrada.length) return true;
    const n = entrada[0].length;
    for (let f = 0; f < n; f++) {
      for (let c = 0; c < 8; c++) this.pcm[this.frames * 8 + c] = entrada[c] ? entrada[c][f] : 0;
      if (++this.frames === 1024) {
        // Un único bloque pendiente: un hilo principal ocupado no acumula audio atrasado.
        if (!this.pendiente) {
          this.pendiente = true;
          this.port.postMessage({ pcm: this.pcm, rate: sampleRate }, [this.pcm.buffer]);
          this.pcm = new Float32Array(1024 * 8);
        }
        this.frames = 0;
      }
    }
    return true;
  }
}
registerProcessor('pli-pcm', PLIPCM);
