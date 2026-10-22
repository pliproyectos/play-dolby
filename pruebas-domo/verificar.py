"""Verifica el núcleo y cada archivo generado contra su manifiesto.

Uso: python verificar.py      -> escribe salida/VERIFICACION.md; código de salida 1 si algo falla.

Qué se comprueba:
  0. Núcleo: bytes de cabecera contra un literal escrito a mano desde la especificación de
     Microsoft (no desde el código), ida y vuelta de 24 bit en los extremos, relleno impar.
  1. Cabecera de cada WAV con el lector propio (formato, máscara, tamaños).
  2. Lector independiente: el módulo wave de CPython abre el archivo y entrega los mismos bytes.
  3. Huella SHA-256 igual a la del manifiesto.
  4. Aislamiento: fuera de sus tramos, cada canal es silencio digital exacto (cero).
  5. Niveles: ruido al RMS objetivo (±0,25 dB), vuelta con potencia total constante (±0,3 dB),
     polaridad exacta (copia o inversión muestra a muestra), continua < 2 % del RMS.
  6. Protección: ningún pico por encima del techo de la ficha.
"""
import hashlib, json, math, pathlib, platform, sys, datetime

import wav71

AQUI = pathlib.Path(__file__).resolve().parent
FS24 = 8388607.0


def db(v):
    return 20 * math.log10(v) if v > 0 else float("-inf")


def prueba_nucleo(tmp):
    fallas = []
    # Literal escrito desde la especificación: 48 kHz, 8 canales, 24 bit, 1 frame, máscara 0x63F
    esperado = bytes.fromhex(
        "52494646" "54000000" "57415645"            # RIFF, 84, WAVE
        "666d7420" "28000000"                        # fmt , 40
        "feff" "0800" "80bb0000" "00941100"          # EXTENSIBLE, 8 ch, 48000, 1152000 B/s
        "1800" "1800" "1600" "1800" "3f060000"       # bloque 24, 24 bit, cbSize 22, 24 válidos, 0x63F
        "0100000000001000800000aa00389b71"           # KSDATAFORMAT_SUBTYPE_PCM
        "64617461" "18000000")                       # data, 24
    cab, rell = wav71.cabecera(48000, wav71.LAYOUTS["7.1"], 24)
    if cab != esperado or rell:
        fallas.append("cabecera 7.1 distinta del literal de la especificación")
    extremos = [-8388608, -1, 0, 1, 8388607, -4194304, 4194303]
    vuelta = wav71.de_24bit(wav71.a_24bit([v / FS24 if v > -8388608 else -1.0000001192 for v in extremos]))
    if vuelta != extremos:
        fallas.append(f"ida y vuelta 24 bit: {vuelta} != {extremos}")
    # Relleno de byte impar: mono, 1 frame = 3 bytes de datos
    lz = wav71.Lienzo(48000, ["FC"], 1)
    lz.pintar("FC", 0, [0.5])
    ruta = tmp / "_nucleo_impar.wav"
    wav71.escribir(ruta, lz)
    info = wav71.leer_cabecera(ruta)
    tam = ruta.stat().st_size
    if tam != 8 + info["tam_riff"] or tam % 2:
        fallas.append("relleno impar o tamaño RIFF incorrecto")
    w = wav71.leer_con_wave_stdlib(ruta)
    if w["datos"] != wav71.leer_datos(ruta, info):
        fallas.append("wave stdlib no coincide en archivo impar")
    ruta.unlink()
    for mal in (["FR", "FL"], ["SL", "BL"]):
        try:
            wav71.mascara(mal)
            fallas.append(f"aceptó orden inválido {mal}")
        except ValueError:
            pass
    return fallas


def verificar_archivo(carpeta, man):
    ruta = carpeta / man["archivo"]
    f = []
    info = wav71.leer_cabecera(ruta)
    nch, fs = len(man["canales"]), man["fs"]
    esperado = {"tag": 0xFFFE, "canales": nch, "fs": fs, "bits": 24, "bloque": nch * 3,
                "bytes_s": fs * nch * 3, "cb_size": 22, "bits_validos": 24,
                "mascara": int(man["mascara"], 16), "subformato_pcm": True,
                "data_len": man["frames"] * nch * 3}
    for k, v in esperado.items():
        if info.get(k) != v:
            f.append(f"cabecera {k} = {info.get(k)!r}, esperado {v!r}")
    if ruta.stat().st_size != 8 + info["tam_riff"]:
        f.append("tamaño RIFF no cuadra con el archivo")
    datos = wav71.leer_datos(ruta, info)
    w = wav71.leer_con_wave_stdlib(ruta)
    if (w["canales"], w["fs"], w["bytes_muestra"], w["frames"]) != (nch, fs, 3, man["frames"]):
        f.append("wave stdlib lee otros parámetros")
    if w["datos"] != datos:
        f.append("wave stdlib entrega bytes distintos")
    h = hashlib.sha256(ruta.read_bytes()).hexdigest()
    if h != man["sha256"]:
        f.append("SHA-256 distinto del manifiesto")

    techo = 10 ** (man["proteccion"]["pico_max_dbfs"] / 20) * FS24
    canal = {c: wav71.canal(datos, nch, k) for k, c in enumerate(man["canales"])}
    medidas = []
    for c, x in canal.items():
        pico = max(abs(v) for v in x)
        if pico > techo:
            f.append(f"{c}: pico {db(pico / FS24):.2f} dBFS sobre el techo")
        ocupado = sorted((t["inicio_frame"], t["fin_frame"]) for t in man["tramos"] if c in t["canales"])
        pos = 0
        for a, b in ocupado + [(len(x), len(x))]:
            if any(x[pos:a]):
                f.append(f"{c}: hay señal fuera de sus tramos entre {pos / fs:.2f} y {a / fs:.2f} s")
                break
            pos = max(pos, b)

    for t in man["tramos"]:
        a, b, obj = t["inicio_frame"], t["fin_frame"], t["objetivo_rms_dbfs"]
        if t["tipo"] == "ruido":
            c = t["canales"][0]
            m = int(0.06 * fs)
            seg = canal[c][a + m:b - m]
            if not any(seg):
                f.append(f"«{t['etiqueta']}»: {c} en silencio donde debía sonar")
                continue
            r = db(math.sqrt(sum(v * v for v in seg) / len(seg)) / FS24)
            # Continua relativa al RMS: un tramo corto de 20 Hz no promedia a cero exacto
            # (umbral absoluto 1e-4 fallaba en 1,5 s de LFE sin haber DC: 2026-10-20).
            dc_rel = abs(sum(seg) / len(seg)) / (10 ** (r / 20) * FS24)
            medidas.append((t["etiqueta"], r, obj))
            if abs(r - obj) > 0.25:
                f.append(f"«{t['etiqueta']}»: RMS {r:.2f} dBFS, objetivo {obj}")
            if dc_rel > 0.02:
                f.append(f"«{t['etiqueta']}»: componente continua {100 * dc_rel:.1f} % del RMS")
        elif t["tipo"] == "vuelta":
            m = int(0.6 * fs)
            pot = 0.0
            for c in t["canales"]:
                seg = canal[c][a + m:b - m]
                pot += sum(v * v for v in seg)
            r = db(math.sqrt(pot / (b - a - 2 * m)) / FS24)
            medidas.append((t["etiqueta"], r, obj))
            if abs(r - obj) > 0.3:
                f.append(f"«{t['etiqueta']}»: potencia total {r:.2f} dBFS, objetivo {obj}")
        elif t["tipo"] == "polaridad":
            c1, c2 = t["canales"]
            x1, x2 = canal[c1][a:b], canal[c2][a:b]
            ok = x2 == x1 if t["relacion"] == "igual" else x2 == [-v for v in x1]
            if not ok:
                f.append(f"«{t['etiqueta']}»: la relación {t['relacion']} no es exacta")
        elif t["tipo"] == "voz":
            for c in t["canales"]:
                if not any(canal[c][a:b]):
                    f.append(f"«{t['etiqueta']}»: voz vacía en {c}")
    return f, medidas


def main():
    salida = AQUI / "salida"
    lineas = ["# Verificación del kit de pruebas del domo", "",
              f"Fecha: {datetime.datetime.now().isoformat(timespec='seconds')} · "
              f"Python {platform.python_version()} ({platform.system()} {platform.release()})", ""]
    total = 0
    fn = prueba_nucleo(salida)
    total += len(fn)
    lineas += ["## 0. Núcleo", "", "**PASA**" if not fn else "**FALLA**", ""] + [f"- {x}" for x in fn] + [""]
    for carpeta in sorted(p for p in salida.iterdir() if p.is_dir()):
        lineas += [f"## {carpeta.name}", "", "| Archivo | Duración | Máscara | Resultado |", "|---|---|---|---|"]
        detalles = []
        for mj in sorted(carpeta.glob("*.json")):
            man = json.loads(mj.read_text(encoding="utf-8"))
            fallas, medidas = verificar_archivo(carpeta, man)
            total += len(fallas)
            lineas.append(f"| {man['archivo']} | {man['duracion_s']:.1f} s | {man['mascara']} | "
                          f"{'PASA' if not fallas else 'FALLA (' + str(len(fallas)) + ')'} |")
            detalles += [f"- {man['archivo']}: {x}" for x in fallas]
            print(f"{carpeta.name:6s} {man['archivo']:28s} {'PASA' if not fallas else 'FALLA'}")
        lineas += [""] + detalles + ([""] if detalles else [])
    lineas += ["## Veredicto", "", "**TODO PASA**" if total == 0 else f"**{total} FALLAS**", "",
               "Alcance: esto verifica los archivos. No prueba la cadena (Windows, HDMI, Integra, bocinas):",
               "eso se comprueba en el domo con la hoja de campo."]
    (salida / "VERIFICACION.md").write_text("\n".join(lineas), encoding="utf-8")
    print("TODO PASA" if total == 0 else f"{total} FALLAS")
    return 0 if total == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
