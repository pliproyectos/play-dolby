"""Genera los programas de prueba del domo a partir de domo.json y de las voces.

Uso:  python generar.py            (48 y 96 kHz)
      python generar.py 96000      (solo una frecuencia)

Salida: salida/<fs>/<programa>.wav + <programa>.json (manifiesto con receta, tramos,
niveles objetivo y huella SHA-256), y salida/GUION.md con la línea de tiempo.
Solo biblioteca estándar de Python 3.12 o más nuevo.
"""
import datetime, hashlib, json, math, pathlib, sys, time, wave

import senales as S
import wav71

VERSION = "1.0.0"
AQUI = pathlib.Path(__file__).resolve().parent
NIVEL_PRUEBA = -30.0   # dBFS RMS, pruebas de escucha
NIVEL_CAL = -20.0      # dBFS RMS, calibración con sonómetro
VOZ_PICO = -14.0       # dBFS pico de las voces
SEMILLA = 7100
BANDAS = {             # Hz
    "media": (500, 2000),      # identificación y calibración (estilo Dolby/MPEG-H)
    "lfe": (20, 120),          # subgrave
    "vuelta": (200, 4000),     # paneo circular: banda ancha para localizar
    "polaridad": (100, 1000),  # donde se oye la cancelación entre dos bocinas
}


class Programa:
    def __init__(self, clave, titulo, layout, fs, kit):
        self.clave, self.titulo, self.layout, self.fs, self.kit = clave, titulo, layout, fs, kit
        self.canales = wav71.LAYOUTS[layout]
        self.tramos, self.cursor = [], 0

    def pausa(self, seg):
        self.cursor += int(round(seg * self.fs))

    def bloque(self, etiqueta, tipo, senales, objetivo=None, **extra):
        n = max(len(v) for v in senales.values())
        meta = {"etiqueta": etiqueta, "tipo": tipo, "canales": list(senales),
                "inicio_frame": self.cursor, "fin_frame": self.cursor + n,
                "objetivo_rms_dbfs": objetivo, **extra}
        self.tramos.append((self.cursor, senales, meta))
        self.cursor += n

    def voz(self, clave, canales, etiqueta=None):
        v = self.kit.voz(clave, self.fs)
        self.bloque(etiqueta or f"voz «{clave}»", "voz", {c: v for c in canales})

    def ruido(self, banda, seg, canal, nivel, etiqueta):
        x = self.kit.ruido(banda, seg, nivel, self.fs)
        self.bloque(etiqueta, "ruido", {canal: x}, nivel, banda_hz=BANDAS[banda])


class Kit:
    def __init__(self):
        self.ficha = json.loads((AQUI / "domo.json").read_text(encoding="utf-8"))
        self.prot = self.ficha["proteccion"]
        self.anillo = {c["id"]: c["azimut"] for c in self.ficha["canales"] if c["azimut"] is not None}
        self._voces, self._ruidos = {}, {}

    def voz(self, clave, fs):
        if (clave, fs) not in self._voces:
            with wave.open(str(AQUI / "voces" / f"{clave}.wav"), "rb") as w:
                assert w.getsampwidth() == 2 and w.getnchannels() == 1 and w.getframerate() == 48000
                import array
                a = array.array("h", w.readframes(w.getnframes()))
            x = S.recortar_silencio([v / 32768.0 for v in a])
            x = S.normalizar_pico(x, VOZ_PICO)
            if fs == 96000:
                x = S.duplicar_fs(x)
            elif fs != 48000:
                raise ValueError("las voces solo se preparan a 48 o 96 kHz")
            self._voces[(clave, fs)] = S.rampas(x, fs, 0.01)
        return self._voces[(clave, fs)]

    def _base(self, banda, fs, seg):
        k = (banda, fs)
        if k not in self._ruidos or len(self._ruidos[k]) < seg * fs:
            fb, fa = BANDAS[banda]
            self._ruidos[k] = S.ruido_banda(max(seg, 24.0), fs, fb, fa, -20.0, SEMILLA)
        return self._ruidos[k]

    def ruido(self, banda, seg, nivel, fs):
        n = int(round(seg * fs))
        x = S.escalar_rms(self._base(banda, fs, seg)[:n], nivel)
        return S.rampas(x, fs)

    def pares(self):
        orden = sorted(self.anillo, key=lambda c: self.anillo[c] % 360)
        return [(orden[i], orden[(i + 1) % len(orden)]) for i in range(len(orden))]


# --- programas -----------------------------------------------------------------------

def p_identificacion(kit, fs):
    p = Programa("01-identificacion", "Identificación de canales 7.1", "7.1", fs, kit)
    p.pausa(1.0)
    for c in p.canales:
        if c == "LFE":
            p.voz("LFE", ["FC"], "voz «Subgrave» por el central")
            p.pausa(0.3)
            p.ruido("lfe", 2.5, "LFE", NIVEL_PRUEBA, "ruido 20–120 Hz en LFE")
        else:
            p.voz(c, [c])
            p.pausa(0.3)
            p.ruido("media", 2.5, c, NIVEL_PRUEBA, f"ruido 500 Hz–2 kHz en {c}")
        p.pausa(1.0)
    return p


def _vuelta(p, kit, seg_vuelta, vueltas, sentido, nivel):
    fs = p.fs
    n = int(round(seg_vuelta * vueltas * fs))
    x = kit.ruido("vuelta", seg_vuelta * vueltas, nivel, fs)
    env = S.rampas([1.0] * n, fs, 0.5)
    orden = sorted(kit.anillo, key=lambda c: kit.anillo[c] % 360)
    az = [kit.anillo[c] % 360 for c in orden]
    salida = {c: [0.0] * n for c in orden}
    m = len(orden)
    for i in range(n):
        a = (sentido * 360.0 * i / (seg_vuelta * fs)) % 360   # arranca en el central (0°)
        for k in range(m):
            tramo = (az[(k + 1) % m] - az[k]) % 360 or 360
            d = (a - az[k]) % 360
            if d < tramo:
                t = d / tramo * math.pi / 2
                v = x[i] * env[i]
                salida[orden[k]][i] = v * math.cos(t)
                salida[orden[(k + 1) % m]][i] = v * math.sin(t)
                break
    nombre = "horario" if sentido > 0 else "antihorario"
    p.bloque(f"vuelta {nombre}: {vueltas} × {seg_vuelta:g} s, ruido 200 Hz–4 kHz", "vuelta",
             salida, nivel, sentido=nombre, segundos_por_vuelta=seg_vuelta, banda_hz=BANDAS["vuelta"])


def p_vuelta(kit, fs):
    p = Programa("02-vuelta-al-domo", "Vuelta al domo (paneo circular)", "7.1", fs, kit)
    p.pausa(1.0)
    p.voz("vuelta", ["FC"])
    p.pausa(0.5)
    _vuelta(p, kit, 12.0, 2, +1, NIVEL_PRUEBA)
    p.pausa(1.0)
    p.voz("antihorario", ["FC"])
    p.pausa(0.5)
    _vuelta(p, kit, 12.0, 2, -1, NIVEL_PRUEBA)
    p.pausa(1.0)
    return p


def p_polaridad(kit, fs):
    p = Programa("03-polaridad", "Polaridad por pares vecinos", "7.1", fs, kit)
    p.pausa(1.0)
    for a, b in kit.pares():
        clave = next((k for k in (f"par-{a}-{b}", f"par-{b}-{a}") if (AQUI / "voces" / f"{k}.wav").exists()), None)
        if clave:
            p.voz(clave, [a, b])
        else:
            p.voz(a, [a, b]); p.pausa(0.15); p.voz(b, [a, b])
        p.pausa(0.4)
        x = kit.ruido("polaridad", 3.0, NIVEL_PRUEBA, fs)
        p.voz("uno", [a, b]); p.pausa(0.2)
        p.bloque(f"{a}+{b} en fase", "polaridad", {a: x, b: x}, NIVEL_PRUEBA,
                 relacion="igual", banda_hz=BANDAS["polaridad"])
        p.pausa(0.6)
        p.voz("dos", [a, b]); p.pausa(0.2)
        p.bloque(f"{a}+{b} con {b} invertido", "polaridad", {a: x, b: [-v for v in x]}, NIVEL_PRUEBA,
                 relacion="invertida", banda_hz=BANDAS["polaridad"])
        p.pausa(1.2)
    return p


def p_calibracion(kit, fs):
    p = Programa("04-calibracion", "Calibración: ruido rosa en banda a −20 dBFS RMS", "7.1", fs, kit)
    p.pausa(1.0)
    for c in p.canales:
        if c == "LFE":
            p.voz("LFE", ["FC"], "voz «Subgrave» por el central")
            p.pausa(0.5)
            p.ruido("lfe", 20.0, "LFE", NIVEL_CAL, "calibración LFE 20–120 Hz")
        else:
            p.voz(c, [c])
            p.pausa(0.5)
            p.ruido("media", 20.0, c, NIVEL_CAL, f"calibración {c} 500 Hz–2 kHz")
        p.pausa(1.5)
    return p


def p_apertura(kit, fs):
    p = Programa("05-rutina-apertura", "Rutina de apertura (antes de cada función)", "7.1", fs, kit)
    p.pausa(0.5)
    p.voz("apertura", ["FC"])
    p.pausa(0.5)
    for c in p.canales:
        if c == "LFE":
            p.voz("LFE", ["FC"], "voz «Subgrave» por el central"); p.pausa(0.15)
            p.ruido("lfe", 1.2, "LFE", NIVEL_PRUEBA, "ruido 20–120 Hz en LFE")
        else:
            p.voz(c, [c]); p.pausa(0.15)
            p.ruido("media", 0.8, c, NIVEL_PRUEBA, f"ruido en {c}")
        p.pausa(0.3)
    p.pausa(0.5)
    _vuelta(p, kit, 10.0, 1, +1, NIVEL_PRUEBA)
    p.pausa(0.5)
    return p


def p_referencia(kit, fs, layout, titulo):
    p = Programa(f"06-ref-{layout}", titulo, layout, fs, kit)
    p.pausa(0.5)
    p.voz(f"ref-{layout}", [c for c in p.canales if c != "LFE"])
    p.pausa(0.6)
    for c in p.canales:
        if c == "LFE":
            p.voz("LFE", ["FC"], "voz «Subgrave» por el central"); p.pausa(0.2)
            p.ruido("lfe", 1.5, "LFE", NIVEL_PRUEBA, "ruido 20–120 Hz en LFE")
        else:
            p.voz("MONO" if layout == "mono" else c, [c]); p.pausa(0.2)
            p.ruido("media", 1.5, c, NIVEL_PRUEBA, f"ruido en {c}")
        p.pausa(0.6)
    return p


PROGRAMAS = [
    p_identificacion, p_vuelta, p_polaridad, p_calibracion, p_apertura,
    lambda k, fs: p_referencia(k, fs, "mono", "Referencia mono (máscara FC)"),
    lambda k, fs: p_referencia(k, fs, "estereo", "Referencia estéreo"),
    lambda k, fs: p_referencia(k, fs, "5.1", "Referencia 5.1 con traseros (0x3F)"),
    lambda k, fs: p_referencia(k, fs, "5.1-laterales", "Referencia 5.1 con laterales (0x60F)"),
    lambda k, fs: p_referencia(k, fs, "7.1", "Referencia 7.1 (0x63F)"),
]


# --- render --------------------------------------------------------------------------

def proteger(p, kit):
    techo = 10 ** (kit.prot["pico_max_dbfs"] / 20)
    for _, sen, meta in p.tramos:
        if meta["objetivo_rms_dbfs"] is not None and meta["objetivo_rms_dbfs"] > kit.prot["rms_max_dbfs"]:
            raise ValueError(f"{p.clave}: «{meta['etiqueta']}» pide más RMS que el límite de la ficha")
        for c, x in sen.items():
            pico = max(abs(v) for v in x)
            if pico > techo:
                raise ValueError(f"{p.clave}: «{meta['etiqueta']}» en {c} llega a {S.db(pico):.1f} dBFS")
            meta.setdefault("pico_dbfs", {})[c] = round(S.db(pico), 2)


def sha256(ruta):
    h = hashlib.sha256()
    with open(ruta, "rb") as f:
        for bloque in iter(lambda: f.read(1 << 20), b""):
            h.update(bloque)
    return h.hexdigest()


def render(p, kit, carpeta):
    proteger(p, kit)
    p.pausa(0.5)
    lienzo = wav71.Lienzo(p.fs, p.canales, p.cursor)
    for inicio, sen, _ in p.tramos:
        for c, x in sen.items():
            lienzo.pintar(c, inicio, x)
    ruta = carpeta / f"{p.clave}.wav"
    wav71.escribir(ruta, lienzo)
    manifiesto = {
        "programa": p.clave, "titulo": p.titulo, "archivo": ruta.name,
        "fs": p.fs, "bits": 24, "layout": p.layout, "canales": p.canales,
        "mascara": hex(wav71.mascara(p.canales)), "frames": p.cursor,
        "duracion_s": round(p.cursor / p.fs, 3), "sha256": sha256(ruta),
        "generador": f"pruebas-domo/generar.py v{VERSION}",
        "fecha": datetime.datetime.now().isoformat(timespec="seconds"),
        "ficha": {"estado": kit.ficha["estado"], "fecha": kit.ficha["fecha"]},
        "semilla": SEMILLA, "proteccion": kit.prot,
        "tramos": [m for _, _, m in p.tramos],
    }
    (carpeta / f"{p.clave}.json").write_text(json.dumps(manifiesto, ensure_ascii=False, indent=1), encoding="utf-8")
    return manifiesto


def guion(manifiestos):
    lin = ["# Guion de los programas de prueba", "",
           "Tiempos iguales a 48 y 96 kHz. «Dónde» es el canal del archivo; la bocina física sale de la ficha del domo.", ""]
    for m in manifiestos:
        lin += [f"## {m['programa']} — {m['titulo']}", "",
                f"Duración {m['duracion_s']:.1f} s · layout {m['layout']} · máscara {m['mascara']}", "",
                "| Inicio | Fin | Qué suena | Dónde |", "|---|---|---|---|"]
        for t in m["tramos"]:
            a, b = t["inicio_frame"] / m["fs"], t["fin_frame"] / m["fs"]
            lin.append(f"| {int(a // 60)}:{a % 60:04.1f} | {int(b // 60)}:{b % 60:04.1f} | {t['etiqueta']} | {' + '.join(t['canales'])} |")
        lin.append("")
    return "\n".join(lin)


def main(frecuencias):
    kit = Kit()
    hechos = {}
    for fs in frecuencias:
        carpeta = AQUI / "salida" / f"{fs // 1000}kHz"
        carpeta.mkdir(parents=True, exist_ok=True)
        hechos[fs] = []
        for fabrica in PROGRAMAS:
            t0 = time.time()
            m = render(fabrica(kit, fs), kit, carpeta)
            hechos[fs].append(m)
            print(f"{fs:>6} Hz  {m['archivo']:28s} {m['duracion_s']:7.1f} s  {time.time() - t0:5.1f} s de calculo")
    primera = hechos[frecuencias[0]]
    (AQUI / "salida" / "GUION.md").write_text(guion(primera), encoding="utf-8")


if __name__ == "__main__":
    main([int(a) for a in sys.argv[1:]] or [48000, 96000])
