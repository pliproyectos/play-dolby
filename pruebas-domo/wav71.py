"""Escritor y lector WAV multicanal con cabecera propia (solo biblioteca estándar).

Formato: WAVE_FORMAT_EXTENSIBLE (0xFFFE), PCM entero de 24 bit, máscara de canales
explícita. Para 7.1 la máscara es 0x63F y el orden intercalado es
FL FR FC LFE BL BR SL SR (bits en orden ascendente, Microsoft Learn,
"WAVEFORMATEXTENSIBLE" y "Channel mask").

Por qué no una librería: libsndfile (python-soundfile) escribe 0xFF en 8 canales por
omisión, que es el 7.1 "wide" (FLC/FRC en lugar de SL/SR). La cabecera son 68 bytes
definidos por Microsoft; aquí se escriben a mano y se verifican al releer.

Límite: RIFF admite 4 GB por archivo (a 96 kHz/24 bit/8 canales, unos 31 min) y FAT32
también corta en 4 GB. El escritor se niega a pasar de ahí en lugar de corromper.
"""
import struct, sys, wave

assert sys.byteorder == "little", "el empaquetado de 24 bit asume little-endian"

GUID_PCM = bytes.fromhex("0100000000001000800000aa00389b71")  # KSDATAFORMAT_SUBTYPE_PCM

BITS_ALTAVOZ = {  # Microsoft Learn, dwChannelMask
    "FL": 0x1, "FR": 0x2, "FC": 0x4, "LFE": 0x8, "BL": 0x10, "BR": 0x20,
    "FLC": 0x40, "FRC": 0x80, "BC": 0x100, "SL": 0x200, "SR": 0x400,
}

LAYOUTS = {  # nombre -> canales en orden de archivo (orden de bit ascendente)
    "mono": ["FC"],
    "estereo": ["FL", "FR"],
    "5.1": ["FL", "FR", "FC", "LFE", "BL", "BR"],             # 0x03F
    "5.1-laterales": ["FL", "FR", "FC", "LFE", "SL", "SR"],   # 0x60F
    "7.1": ["FL", "FR", "FC", "LFE", "BL", "BR", "SL", "SR"], # 0x63F
}

LIMITE_RIFF = 0xFFFFFFFF
_SIGNO = bytes(0xFF if b & 0x80 else 0 for b in range(256))  # extensión de signo 24→32
ESCALA_24 = 8388607.0


def mascara(canales):
    m, previo = 0, 0
    for c in canales:
        bit = BITS_ALTAVOZ[c]
        if bit <= previo:
            raise ValueError(f"canales fuera del orden de bit: {canales}")
        m |= bit
        previo = bit
    return m


def a_24bit(muestras):
    """Lista de floats en [-1, 1) -> bytes PCM 24 bit little-endian."""
    q = [int(round(x * ESCALA_24)) for x in muestras]
    for v in (min(q, default=0), max(q, default=0)):
        if v > 8388607 or v < -8388608:
            raise ValueError("muestra fuera de escala: el generador debió frenarla antes")
    import array
    b32 = array.array("i", q).tobytes()
    out = bytearray(len(q) * 3)
    out[0::3], out[1::3], out[2::3] = b32[0::4], b32[1::4], b32[2::4]
    return bytes(out)


def de_24bit(datos):
    """bytes PCM 24 bit -> lista de enteros con signo."""
    import array
    n = len(datos) // 3
    b32 = bytearray(n * 4)
    b32[0::4], b32[1::4], b32[2::4] = datos[0::3], datos[1::3], datos[2::3]
    b32[3::4] = datos[2::3].translate(_SIGNO)
    return array.array("i", bytes(b32)).tolist()


class Lienzo:
    """Buffer intercalado en silencio digital; se pintan canales mono en tramos."""

    def __init__(self, fs, canales, n_frames):
        self.fs, self.canales, self.n = fs, list(canales), n_frames
        self.nch = len(self.canales)
        self.frame = self.nch * 3
        total = n_frames * self.frame
        if total + 68 > LIMITE_RIFF:
            raise ValueError("el archivo pasaría de 4 GB: acortar el programa")
        self.datos = bytearray(total)

    def pintar(self, canal, inicio_frame, muestras):
        if not muestras:
            return
        k = self.canales.index(canal)
        fin = inicio_frame + len(muestras)
        if inicio_frame < 0 or fin > self.n:
            raise ValueError(f"tramo fuera del lienzo en {canal}")
        b = a_24bit(muestras)
        base = inicio_frame * self.frame + k * 3
        tope = fin * self.frame
        for j in range(3):
            self.datos[base + j:tope:self.frame] = b[j::3]


def cabecera(fs, canales, n_bytes_datos):
    nch, bits = len(canales), 24
    bloque = nch * bits // 8
    fmt = struct.pack("<HHIIHHHHI16s", 0xFFFE, nch, fs, fs * bloque, bloque, bits,
                      22, bits, mascara(canales), GUID_PCM)
    relleno = n_bytes_datos % 2
    tam_riff = 4 + (8 + len(fmt)) + (8 + n_bytes_datos + relleno)
    return (b"RIFF" + struct.pack("<I", tam_riff) + b"WAVE"
            + b"fmt " + struct.pack("<I", len(fmt)) + fmt
            + b"data" + struct.pack("<I", n_bytes_datos)), relleno


def escribir(ruta, lienzo):
    cab, relleno = cabecera(lienzo.fs, lienzo.canales, len(lienzo.datos))
    with open(ruta, "wb") as f:
        f.write(cab)
        f.write(lienzo.datos)
        if relleno:
            f.write(b"\x00")


def leer_cabecera(ruta):
    """Lector propio: devuelve los campos de fmt y la posición de data."""
    with open(ruta, "rb") as f:
        riff, tam, wave_id = struct.unpack("<4sI4s", f.read(12))
        if riff != b"RIFF" or wave_id != b"WAVE":
            raise ValueError("no es RIFF/WAVE")
        info = {"tam_riff": tam}
        while True:
            h = f.read(8)
            if len(h) < 8:
                break
            cid, clen = struct.unpack("<4sI", h)
            if cid == b"fmt ":
                c = f.read(clen)
                tag, nch, fs, bps, bloque, bits = struct.unpack_from("<HHIIHH", c, 0)
                info.update(tag=tag, canales=nch, fs=fs, bytes_s=bps, bloque=bloque, bits=bits)
                if tag == 0xFFFE:
                    cb, validos, m = struct.unpack_from("<HHI", c, 16)
                    info.update(cb_size=cb, bits_validos=validos, mascara=m,
                                subformato_pcm=(c[24:40] == GUID_PCM))
            elif cid == b"data":
                info.update(data_ofs=f.tell(), data_len=clen)
                f.seek(clen + (clen % 2), 1)
            else:
                f.seek(clen + (clen % 2), 1)
    return info


def leer_datos(ruta, info):
    with open(ruta, "rb") as f:
        f.seek(info["data_ofs"])
        return f.read(info["data_len"])


def leer_con_wave_stdlib(ruta):
    """Lector independiente (CPython wave, admite EXTENSIBLE desde 3.12)."""
    with wave.open(str(ruta), "rb") as w:
        return {"canales": w.getnchannels(), "fs": w.getframerate(),
                "bytes_muestra": w.getsampwidth(), "frames": w.getnframes(),
                "datos": w.readframes(w.getnframes())}


def canal(datos, nch, k):
    """Extrae el canal k (enteros) de datos intercalados de 24 bit."""
    frame = nch * 3
    sub = bytearray(len(datos) // nch)
    for j in range(3):
        sub[j::3] = datos[k * 3 + j::frame]
    return de_24bit(bytes(sub))
