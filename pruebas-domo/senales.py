"""Señales de prueba en Python puro (sin numpy): ruido rosa en banda, rampas, nivel RMS,
paneo circular y remuestreo x2 de las voces.

Ruido rosa: filtro "refinado" de Paul Kellet sobre ruido blanco uniforme. Sus polos están
fijos en frecuencia normalizada, así que a 96 kHz la pendiente de -3 dB/oct vale de unos
20 Hz hacia arriba (doctrina, no medido aquí). Las bandas se cortan con biquads
Butterworth (RBJ Audio EQ Cookbook), dos en cascada por borde (4.º orden).
"""
import math, random

# --- filtros -------------------------------------------------------------------------

def _biquad(x, b0, b1, b2, a1, a2):
    y, x1, x2, y1, y2 = [0.0] * len(x), 0.0, 0.0, 0.0, 0.0
    for i, xn in enumerate(x):
        yn = b0 * xn + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2
        x2, x1, y2, y1 = x1, xn, y1, yn
        y[i] = yn
    return y


def _coef(tipo, f, fs, q=1 / math.sqrt(2)):
    w = 2 * math.pi * f / fs
    c, al = math.cos(w), math.sin(w) / (2 * q)
    if tipo == "lp":
        b0, b1, b2 = (1 - c) / 2, 1 - c, (1 - c) / 2
    else:
        b0, b1, b2 = (1 + c) / 2, -(1 + c), (1 + c) / 2
    a0 = 1 + al
    return b0 / a0, b1 / a0, b2 / a0, (-2 * c) / a0, (1 - al) / a0


def pasa_altos(x, f, fs):
    k = _coef("hp", f, fs)
    return _biquad(_biquad(x, *k), *k)


def pasa_bajos(x, f, fs):
    k = _coef("lp", f, fs)
    return _biquad(_biquad(x, *k), *k)


# --- ruido ---------------------------------------------------------------------------

def ruido_rosa(n, semilla):
    r = random.Random(semilla)  # Mersenne Twister: misma semilla, mismo ruido
    b0 = b1 = b2 = b3 = b4 = b5 = b6 = 0.0
    out = [0.0] * n
    for i in range(n):
        w = 2.0 * r.random() - 1.0
        b0 = 0.99886 * b0 + w * 0.0555179
        b1 = 0.99332 * b1 + w * 0.0750759
        b2 = 0.96900 * b2 + w * 0.1538520
        b3 = 0.86650 * b3 + w * 0.3104856
        b4 = 0.55000 * b4 + w * 0.5329522
        b5 = -0.7616 * b5 - w * 0.0168980
        out[i] = b0 + b1 + b2 + b3 + b4 + b5 + b6 + w * 0.5362
        b6 = w * 0.115926
    return out


def ruido_banda(segundos, fs, f_baja, f_alta, rms_dbfs, semilla):
    """Ruido rosa limitado en banda, con el RMS ajustado exacto al objetivo.
    Se descarta un segundo de arranque para no llevarse el transitorio de los filtros."""
    pre = fs
    n = int(round(segundos * fs))
    x = ruido_rosa(n + pre, semilla)
    if f_baja:
        x = pasa_altos(x, f_baja, fs)
    if f_alta:
        x = pasa_bajos(x, f_alta, fs)
    x = x[pre:]
    return escalar_rms(x, rms_dbfs)


def rms(x):
    return math.sqrt(sum(v * v for v in x) / len(x)) if x else 0.0


def db(v):
    return 20 * math.log10(v) if v > 0 else float("-inf")


def escalar_rms(x, rms_dbfs):
    g = 10 ** (rms_dbfs / 20) / rms(x)
    return [v * g for v in x]


def rampas(x, fs, seg=0.05):
    """Entrada y salida en coseno elevado: sin clics."""
    n = min(int(seg * fs), len(x) // 2)
    y = list(x)
    for i in range(n):
        g = 0.5 - 0.5 * math.cos(math.pi * i / n)
        y[i] *= g
        y[-1 - i] *= g
    return y


def silencio(segundos, fs):
    return [0.0] * int(round(segundos * fs))


# --- paneo circular ------------------------------------------------------------------

def ganancias_anillo(azimuts, angulo):
    """Paneo de potencia constante entre las dos bocinas vecinas del anillo.
    azimuts: {canal: grados}, positivo hacia la derecha (horario visto desde arriba)."""
    orden = sorted(azimuts, key=lambda c: azimuts[c] % 360)
    a = angulo % 360
    for i, c in enumerate(orden):
        sig = orden[(i + 1) % len(orden)]
        a0, a1 = azimuts[c] % 360, azimuts[sig] % 360
        tramo = (a1 - a0) % 360 or 360
        d = (a - a0) % 360
        if d < tramo:
            t = d / tramo
            return {c: math.cos(t * math.pi / 2), sig: math.sin(t * math.pi / 2)}
    raise ValueError("anillo de bocinas incompleto")


# --- voces ---------------------------------------------------------------------------

def _medio_banda(k=16):
    h = []
    for j in range(k):
        t = j + 0.5
        ventana = 0.42 + 0.5 * math.cos(math.pi * t / (k + 0.5)) + 0.08 * math.cos(2 * math.pi * t / (k + 0.5))
        h.append(math.sin(math.pi * t) / (math.pi * t) * ventana)
    s = 2 * sum(h)
    return [v / s for v in h]


def duplicar_fs(x, k=16):
    """Remuestreo x2 (p. ej. 48 -> 96 kHz) con interpolador de media banda (sinc + Blackman)."""
    h = _medio_banda(k)
    n = len(x)
    xp = [0.0] * k + list(x) + [0.0] * (k + 1)
    out = [0.0] * (2 * n)
    for i in range(n):
        c = i + k
        out[2 * i] = x[i]
        acc = 0.0
        for j in range(k):
            acc += h[j] * (xp[c - j] + xp[c + 1 + j])
        out[2 * i + 1] = acc
    return out


def recortar_silencio(x, umbral_db=-50):
    u = 10 ** (umbral_db / 20)
    idx = [i for i, v in enumerate(x) if abs(v) > u]
    return x[idx[0]:idx[-1] + 1] if idx else []


def normalizar_pico(x, pico_dbfs):
    p = max(abs(v) for v in x)
    g = 10 ** (pico_dbfs / 20) / p
    return [v * g for v in x]
