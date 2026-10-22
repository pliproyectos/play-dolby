"""Genera prueba-7.1.wav: un tono por bocina, en secuencia, PCM 8 canales 48 kHz.

Orden de canales del archivo = máscara Windows 7.1 (0x63F):
FL FR C LFE BL BR SL SR. Nivel conservador (-30 dBFS), rampas de 50 ms.
Cada canal suena 2 s y luego 1 s de silencio. LFE usa 50 Hz; el resto 440 Hz.
"""
import math, struct, sys, pathlib

SR, NCH = 48000, 8
NOMBRES = ["FL", "FR", "C", "LFE", "BL", "BR", "SL", "SR"]
AMP = 10 ** (-30 / 20)
TONO, PAUSA, RAMPA = 2.0, 1.0, 0.05

def main(destino):
    n_tono, n_pausa, n_rampa = int(TONO * SR), int(PAUSA * SR), int(RAMPA * SR)
    silencio = b"\x00\x00" * NCH
    datos = bytearray(silencio * SR)  # 1 s inicial de silencio
    for ch in range(NCH):
        f = 50.0 if NOMBRES[ch] == "LFE" else 440.0
        for i in range(n_tono):
            env = min(1.0, i / n_rampa, (n_tono - i) / n_rampa)
            v = int(32767 * AMP * env * math.sin(2 * math.pi * f * i / SR))
            marco = [0] * NCH
            marco[ch] = v
            datos += struct.pack("<8h", *marco)
        datos += silencio * n_pausa
    # WAVE_FORMAT_EXTENSIBLE, subformato PCM, máscara 0x63F
    guid_pcm = bytes.fromhex("0100000000001000800000aa00389b71")
    fmt = struct.pack("<HHIIHHHHI", 0xFFFE, NCH, SR, SR * NCH * 2, NCH * 2, 16, 22, 16, 0x63F) + guid_pcm
    riff = b"WAVE" + b"fmt " + struct.pack("<I", len(fmt)) + fmt + b"data" + struct.pack("<I", len(datos)) + datos
    pathlib.Path(destino).write_bytes(b"RIFF" + struct.pack("<I", len(riff)) + riff)
    for ch, nom in enumerate(NOMBRES):
        print(f"{1 + ch * 3:>3} s  {nom}")

if __name__ == "__main__":
    main(sys.argv[1])
