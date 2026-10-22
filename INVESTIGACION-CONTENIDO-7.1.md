# Investigación: contenido de prueba 7.1 para el domo

**Fecha:** 2026-10-19.
**Cadena:** Qu-5 → Play Dolby (PortAudio, 8 canales) → HDMI LPCM → Integra DRX-8.4 → domo 7.1.
**Máscara Windows:** 0x63F, es decir FL FR FC LFE BL BR SL SR.

**Cómo se hizo.** Investigación web con dos subagentes en paralelo:
- Contenido existente.
- Fabricación.

**Lo que no se hizo.** No se descargó, instaló ni reprodujo nada. El subagente 1 solo pidió cabeceras HTTP (`curl -I`) para ver si los enlaces estaban vivos.

**Ningún comando ni fragmento de código de este documento se ha ejecutado**, salvo la lectura de cabecera del §0.

## Marcas que se usan

| Marca | Significado |
|---|---|
| **[V]** | Verificado en una fuente que sí se abrió (página, doc primaria o código fuente) |
| **[V-búsq]** | Visto solo en un resumen de buscador o en un foro |
| **[NV]** | De memoria o de terceros, sin verificar |
| **[NV-ej]** | El comando o el código está escrito pero no se corrió |

---

## 0. Lo verificado en esta máquina

Se leyó la cabecera de `prueba-7.1.wav`, que genera `native/scripts/generar-prueba-71.py`. Fue solo lectura: `WAVE_FORMAT_EXTENSIBLE (0xFFFE), 8 canales, 48000 Hz, 16 bit, máscara 0x63F` **[V]**.

Lo que sale de esa lectura:
- **La máscara ya es correcta.** No hay que corregir el generador.
- **El LFE ya usa 50 Hz, no 440 Hz** (línea 19 del script) **[V]**. El subagente 2 había advertido de un posible problema con 440 Hz en el LFE, y no aplica.
- **El archivo es de 48 kHz y la app trabaja a 96 kHz.** No verifiqué qué hace Play Dolby con un archivo de otra frecuencia. El material nuevo conviene hacerlo **a 96 kHz y 24 bit** para no depender de un remuestreo.

## Resumen operativo

1. **Para identificar canales con voz:** usar **SpeakerEject 7.1 voice** (A1). Licencia CC BY 4.0, máscara 0x63F verificada, WAV directo.
2. **Para calibrar y probar polaridad:** no existe un paquete 7.1 LPCM libre y verificado. **Hay que fabricarlo.** La parte B trae las recetas.
3. **Los demos Dolby y DTS** (TrueHD, DTS-HD MA) son útiles para escuchar en conjunto, pero tienen dos limitaciones:
   - Requieren que un reproductor (LAV o ffmpeg) los decodifique a PCM; Play Dolby no hace *bitstream*.
   - Su licencia es dudosa: valen para prueba interna, no para función pública.
4. **El contenido de domo libre es 5.1, no 7.1.** Al pasarlo a 7.1, Ls y Rs deben ir a **SL/SR**. Qué hacer con BL/BR es criterio de diseño sonoro y lo decide Luis.

---

# PARTE A — Contenido existente

## A.1 Identificación de canales

| # | Recurso | Formato | Canales y orden | Licencia | ¿Sirve como LPCM 7.1? | Estado |
|---|---|---|---|---|---|---|
| A1 | **SpeakerEject 7.1 con voz.** https://speakereject.com/audio/surround-test-7.1-voice.wav (también existen `.flac` y `.mp4` AAC). Página: https://speakereject.com/surround-sound-test/ | WAV EXTENSIBLE PCM, 16 bit, 48 kHz, 24 s (tramos de 3 s) | 8 canales, **0x63F**. En el tramo LFE la voz sale por el centro y el LFE lleva 80 Hz | CC BY 4.0 | **Sí, directo** | [V] máscara, orden, licencia y enlace vivo. La voz es TTS (Piper); el idioma es probablemente inglés [NV] |
| A2 | **SpeakerEject 7.1, solo tonos.** https://speakereject.com/audio/surround-test-7.1.wav | WAV PCM, 16 bit, 48 kHz, 16 s, con SHA-256 publicado | 0x63F. Tono de 440 Hz y 80 Hz en el LFE | CC BY 4.0 | Sí. Equivale a nuestro `prueba-7.1.wav` | [V] |
| A3 | **Fraunhofer IIS 7.1 Channel Identification.** https://www2.iis.fraunhofer.de/AAC/7.1auditionOutLeader%20v2.wav. Página: https://www2.iis.fraunhofer.de/AAC/multichannel.html | WAV PCM; también existe versión HE-AAC MP4 | «BluRay 7.1», con voz. La página no da el orden por canal | © Fraunhofer, sin licencia explícita | Sí, si la máscara es 0x63F | [V] página y enlaces. [NV] máscara: revisar con MediaInfo |
| A4 | **Microsoft `8_Channel_ID.wav`.** https://mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/Samples/Microsoft/8_Channel_ID.wav | WAV EXTENSIBLE, 24 bit, 48 kHz, 8 s | FL FR C LFE BL BR y **«Auxiliary L/R» sin bit en la máscara** | Microsoft | **Con cuidado.** Cada reproductor enruta los canales 7 y 8 a su manera | [V] descripción en McGill |
| A5 | Otra copia de A4. https://samples.ffmpeg.org/A-codecs/wavpcm/8_Channel_ID.wav | WAV de otro tamaño | Sin documentar | — | Igual que A4 | [V] que existe. No es el mismo archivo |
| A6 | **sfiera/flac-test-files `surround71.flac`.** https://github.com/sfiera/flac-test-files | FLAC | Orden FLAC 8 canales = 0x63F. Voces de `alsa-utils` y ruido en el LFE | GPLv2 | Sí | [V] README y enlace. [NV] frecuencia de muestreo |
| A7 | **BigSoundBank «Test 7.1 (Surround)».** https://bigsoundbank.com/test-7-1-surround-s3123.html | WAV/BWF, 24 bit, 48 kHz, 35 s | Orden no indicado. **Voz en francés** | **CC0** | Probable, pero hay que revisar la máscara | [V] metadatos y licencia. [NV] orden |
| A8 | **Dolby TrueHD 7.1 Channel Check.** https://www.demolandia.net/cinema/dolby-demo-trailers-hd/page-8.html | m2ts, TrueHD 7.1, 94 s | Voz de Dolby | Redistribución de terceros | Sí, decodificado con LAV, MPC o VLC | [V] ficha. La copia en demo-world.eu da HTTP 500 |
| A9 | **Dolby Digital Plus 7.1 Channel Check** (misma página que A8) | m2ts, E-AC-3 7.1 | 7.1 | Igual que A8 | Probable | [V] ficha. [NV] que el decodificador entregue los 8 canales |
| A10 | **DTS-HD MA Sound Check 7.1.** https://www.demolandia.net/cinema/dts-demo-trailers-hd.html | m2ts, DTS-HD MA 7.1 | 7.1 | Tercero | Sí; ffmpeg decodifica DTS-HD MA | [V] ficha |
| A11 | **MPEG-H Channel ID 7.1+4** (Fraunhofer). https://mpegh.com/?p=1416 | MP4 MPEG-H | 7.1+4 | Solo uso del usuario final | **No.** VLC, MPC y LAV no decodifican MPEG-H [NV] | [V] página y licencia |
| A12 | 2L Immersive Channel ID 7.1.4 (NativeDSD, de pago) | Varios formatos | — | Comercial | Si trae WAV, sí | [NV]: la página de producto da 404 |

## A.2 Calibración (ruido rosa y tonos)

| # | Recurso | Detalle | Estado |
|---|---|---|---|
| B1 | **MPEG-H Level Alignment y LFE Level Alignment.** https://mpegh.com/?p=1416 | Principales: ruido rosa de **500–2000 Hz a −24 LKFS**. LFE: ruido de **35,5–70,8 Hz**. No se puede reproducir (MPEG-H), pero **sirve como referencia de parámetros** | [V] parámetros |
| B2 | **Dolby Atmos Test Tones**, `download.dolby.com/us/en/test-tones/dolby-test-tones_7_1_4.mp4`, citados en https://hometheater.substack.com/p/atmos-audio-test-files | E-AC-3 JOC según terceros. Un PC sin Atmos suele obtener solo el núcleo 5.1 | **[V] HTTP 404** a una petición HEAD; no se probó GET |
| B3 | **Atmos Test Tones 7.1.2 / 7.1.4** en Demolandia. https://www.demolandia.net/cinema/dolby-demo-trailers-hd.html | MKV de 741 y 893 MB | [V] ficha. [NV] códec |
| B4 | Spears & Munsil HD Benchmark (disco comercial) | Pruebas DTS-HD MA y TrueHD. **AVS HD 709 no trae audio** | [V-búsq] |

## A.3 Fase, polaridad y LFE

| # | Recurso | Detalle | Estado |
|---|---|---|---|
| C1 | **SpeakerEject, prueba de fase.** https://speakereject.com/speaker-phase-test/ | WAV **estéreo**: seno de 200 Hz en fase y en contrafase, a −26 dBFS pico. No hay versión de 8 canales; para 7.1 hay que fabricarla (ver §B.3) | [V] |
| C2 | SpeakerEject LFE, `/audio/surround-test-5.1-lfe.wav` | Seno de 50 Hz en 5.1 | [V] mencionado |
| C3 | audiocheck.net (polaridad y graves) | Solo estéreo; el WAV es solo para mecenas | [V]. No sirve para 7.1 |
| C4 | Barridos de Thothsong en AVS Forum | Uno por altavoz, 9.x.6 | [NV] |

## A.4 Demos 7.1 y domo

| # | Recurso | Detalle | Estado |
|---|---|---|---|
| D1 | **Lista del foro de Radxa.** https://forum.radxa.com/t/hd-audio-passthrough-testing/1864 | DD+ 7.1, TrueHD 7.1, DTS-HD MA 7.1, **LPCM 7.1** y **FLAC 7.1** en Google Drive. Es la misma lista que la wiki de Kodi (https://kodi.wiki/view/Samples, que dio 403) | [V] la lista. **No se comprobó cada enlace**; los de demo-world dan 500 |
| D2 | **Demolandia DTS.** https://www.demolandia.net/cinema/dts-demo-trailers-hd.html | Orchestra, Listen, DTS:X 7.1.4 con núcleo MA 7.1 | [V] fichas. [NV] que ffmpeg extraiga el núcleo 7.1 |
| D3 | **thedigitaltheater.com** | THX «Sphere» 7.1, Dolby «Element» 7.1.2, THX Deep Note DTS-HD MA 7.1 (**upmix**) | [V] sitio vivo |
| D4 | **ESO «Audio Universe: Tour of the Solar System».** https://eso.org/public/videos/au-totss-fulldome | Fulldome 4K, **5.1, no 7.1**, con pista **en español** (https://www.eso.org/public/archives/videos/broadcastaudio/au-totss-fulldome_surround_es.zip, 1,5 GB). Licencia CC BY-ND 4.0; para funciones hay que llenar un formulario | [V] |
| D5 | Ritter Planetarium, «Shining Light on the Stars». https://www.utoledo.edu/nsm/rpbo/fulldomestudio/fdfa-hr.html | Fulldome gratis con audio WAV. Canales y licencia no indicados | [V] página |
| D6 | «The Scientific Alphabet» (Altair). https://fddb.org/shows/the-scientific-alphabet | Estéreo, 5.1 y 7.1; de pago | [V-búsq] |

## A.5 Pistas multicanal libres

| # | Recurso | Detalle | Estado |
|---|---|---|---|
| E1 | **Películas abiertas de Blender en Xiph.** https://media.xiph.org/ | Tears of Steel, Sintel, Big Buck Bunny y Elephants Dream en FLAC. **Todas son 5.1.** Licencia CC BY | [V] |
| E2 | **Freesound, filtro de 8 canales.** https://freesound.org/search/?q=surround&f=channels%3A8 | Solo 6 resultados. Por ejemplo «Beach (7.1 Surround)», FLAC 24/48, CC BY-NC, orden no indicado | [V] |

**Hallazgos negativos [V]:**
- El kit OTT de Dolby solo trae Channel ID en 5.1.
- El banco de pruebas libre de 2L ya no está disponible.
- No se halló música 7.1 en Creative Commons que valga la pena.

## A.6 Advertencias sobre contenido ajeno

- **0x63F frente a 0xFF.** 0xFF es el «7.1 wide» (FLC/FRC en lugar de SL/SR) [V, Microsoft Learn]. Antes de usar un WAV ajeno, revisar su máscara con MediaInfo o ffprobe, o con el verificador del §B.1.
- **Atmos en TrueHD.** La base 7.1 es un subflujo TrueHD que LAV y ffmpeg decodifican a LPCM 7.1 [NV].
- **DD+ Atmos (JOC).** El núcleo decodificable suele ser 5.1 [NV].
- **DTS:X.** ffmpeg da el núcleo MA, a menudo 7.1 [NV].
- **Muchos «7.1» son en realidad 5.1 o upmix.** Hay que contar los canales reales.
- **TrueHD tuvo un error histórico de traseros y laterales invertidos** en ffmpeg (2010) [V]. Hoy se da por corregido [NV]. La prueba definitiva es la voz por canal.

---

# PARTE B — Fabricación

## B.1 WAV 0x63F desde Python

- **Máscara [V].** Bits: FL 0x1, FR 0x2, FC 0x4, LFE 0x8, BL 0x10, BR 0x20, FLC 0x40, FRC 0x80, SL 0x200, SR 0x400. Los canales van intercalados en orden de bit ascendente. Fuentes: https://learn.microsoft.com/en-us/windows/win32/api/mmreg/ns-mmreg-waveformatextensible y https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/channel-mask
- **`soundfile` (libsndfile) NO escribe 0x63F por omisión [V, código `src/wav.c`].**
  - Con `format='WAVEX'` y 8 canales escribe **0xFF** (7.1 wide).
  - Con `'WAV'` no escribe EXTENSIBLE.
  - Se puede forzar con `SFC_SET_CHANNEL_MAP_INFO`, pero python-soundfile no lo expone. Haría falta su API privada, y es frágil.
- **Recomendación:** cabecera a mano con numpy y struct, como ya hace `generar-prueba-71.py`. A continuación, la versión a **24 bit** **[NV-ej]**:

```python
import numpy as np, struct
GUID_PCM = bytes.fromhex("0100000000001000800000aa00389b71")
def write_wav_71(path, x, fs=96000, mask=0x63F):
    """x: float (n, 8) en [-1, 1), orden FL FR FC LFE BL BR SL SR. PCM 24 bit EXTENSIBLE."""
    nch, bits = 8, 24
    q = np.clip(np.round(x * 8388607.0), -8388608, 8388607).astype('<i4')
    data = q.view(np.uint8).reshape(-1, 4)[:, :3].tobytes()
    block = nch * bits // 8
    fmt = struct.pack('<HHIIHHHHI16s', 0xFFFE, nch, fs, fs*block, block, bits, 22, bits, mask, GUID_PCM)
    pad = b'\x00' if len(data) % 2 else b''
    with open(path, 'wb') as f:
        f.write(b'RIFF' + struct.pack('<I', 4 + 8 + len(fmt) + 8 + len(data) + len(pad)) + b'WAVE')
        f.write(b'fmt ' + struct.pack('<I', len(fmt)) + fmt)
        f.write(b'data' + struct.pack('<I', len(data)) + data + pad)
```

**Límite:** un RIFF admite 4 GB. A 96 kHz, 24 bit y 8 canales eso son **unos 31 minutos**. Para más, usar RF64 (`ffmpeg -rf64 auto`).

**Verificador de máscara.** Es el mismo que se usó en el §0 y funciona [V]:

```python
import struct
b = open('archivo.wav','rb').read(200); i = b.find(b'fmt ') + 8
tag, ch = struct.unpack_from('<HH', b, i); print(hex(tag), ch, hex(struct.unpack_from('<I', b, i+20)[0]))
# esperado: 0xfffe 8 0x63f
```

## B.2 SoX y FFmpeg

**SoX [V, código `src/wav.c` de la rama master].**
- Con 8 canales escribe **0x63F**.
- Con 6 canales escribe siempre 0x3F (5.1 *back*), sin opción para cambiarlo.
- Que el binario 14.4.2 de Windows haga lo mismo: [NV].

**FFmpeg [V, ffmpeg-utils y `riffenc.c`].** Escribe la máscara del layout que tenga el flujo:

| Layout | Canales | Máscara |
|---|---|---|
| `7.1` | FL FR FC LFE BL BR SL SR | 0x63F |
| `7.1(wide)` | FL FR FC LFE BL BR FLC FRC | 0xFF |
| `5.1` | FL FR FC LFE BL BR | 0x3F |
| `5.1(side)` | FL FR FC LFE SL SR | 0x60F |

En la documentación actual, `-ch_layout` es alias de `-channel_layout`.

**Ejemplos [NV-ej].** Son sintaxis de bash; en PowerShell hay que cuidar las comillas.

```bash
# Ruido rosa 500 Hz–2 kHz en FC, con rampas de 50 ms
ffmpeg -f lavfi -i "anoisesrc=c=pink:r=96000:a=0.5:d=3:seed=3" \
  -af "highpass=f=500:p=2,highpass=f=500:p=2,lowpass=f=2000:p=2,lowpass=f=2000:p=2,afade=t=in:d=0.05,afade=t=out:st=2.95:d=0.05,pan=7.1|FC=c0" \
  -c:a pcm_s24le c3_FC.wav

# LFE: ruido rosa 20–120 Hz
ffmpeg -f lavfi -i "anoisesrc=c=pink:r=96000:a=0.5:d=3:seed=4" \
  -af "highpass=f=20:p=2,lowpass=f=120:p=2,lowpass=f=120:p=2,afade=t=in:d=0.1,afade=t=out:st=2.9:d=0.1,pan=7.1|LFE=c0" \
  -c:a pcm_s24le c4_LFE.wav

# Unir 8 monos en un 7.1 con mapa explícito (preferible a amerge, que puede reordenar)
ffmpeg -i FL.wav -i FR.wav -i FC.wav -i LFE.wav -i BL.wav -i BR.wav -i SL.wav -i SR.wav \
  -filter_complex "join=inputs=8:channel_layout=7.1:map=0.0-FL|1.0-FR|2.0-FC|3.0-LFE|4.0-BL|5.0-BR|6.0-SL|7.0-SR" \
  -c:a pcm_s24le -ar 96000 todo_71.wav

# Concatenar tramos de 8 canales (lista.txt con líneas:  file 'c1_FL.wav')
ffmpeg -f concat -safe 0 -i lista.txt -c copy secuencia_71.wav

# Medir el RMS por canal: la amplitud "a" de anoisesrc no es RMS
ffmpeg -i secuencia_71.wav -af astats=measure_perchannel=RMS_level+Peak_level:measure_overall=none -f null -
```

**SoX equivalente [NV-ej]:**

```bash
sox -n -r 96000 -b 24 -c 1 rosa_bp.wav synth 3 pinknoise sinc 500-2k fade q 0.05 3 0.05 gain -n -20
sox rosa_bp.wav -c 8 c3_FC.wav remix 0 0 1 0 0 0 0 0
```

Ojo: `gain -n -20` normaliza el **pico**, no el RMS. El RMS queda entre 10 y 12 dB más abajo.

## B.3 Pruebas que conviene fabricar (no existen libres en 7.1)

Este es el criterio de los subagentes; decide Luis.

1. **Identificación con voz en español.** Por canal: voz, 0,5 s de silencio, 3 s de ruido rosa y 1 s de silencio. En el LFE, ruido de 20–120 Hz en lugar de voz, porque la voz no pasa el filtro del LFE.
2. **Calibración.** Ruido rosa de 500–2000 Hz a −20 dBFS RMS en cada canal principal, y ruido de 20–120 Hz en el LFE. Los parámetros del MPEG-H (B1) sirven de referencia.
3. **Polaridad.** Por cada par adyacente (FL-FR, SL-SR, BL-BR, FL-SL…), el mismo seno de 200 Hz o ruido en banda, primero en fase y luego con un canal invertido. Si está bien cableado, la versión en fase suena más llena y centrada.
4. **Barrido de LFE** de 20 a 200 Hz, lento y a nivel bajo.

## B.4 Voces TTS en español

- **Windows SAPI.** `System.Speech` y `SetOutputToWaveFile` existen [V]. Hace falta una voz en español instalada en Windows [NV-ej]:

  ```powershell
  Add-Type -AssemblyName System.Speech
  $s = New-Object System.Speech.Synthesis.SpeechSynthesizer
  $s.GetInstalledVoices() | % { $_.VoiceInfo.Name + ' ' + $_.VoiceInfo.Culture }
  $s.SetOutputToWaveFile("$PWD\voz_FL.wav"); $s.Speak('Frontal izquierdo'); $s.SetOutputToNull()
  ```

- **Otras opciones:**
  - edge-tts requiere internet [V-búsq].
  - Piper y espeak-ng funcionan sin conexión [NV].
- **Textos:** FL «frontal izquierdo», FR «frontal derecho», FC «central», LFE «subgrave», BL «trasero izquierdo», BR «trasero derecho», SL «lateral izquierdo», SR «lateral derecho».
- **Colocar una voz en su canal [NV-ej]:**

  ```bash
  ffmpeg -i voz_SL.wav -af "aresample=96000,pan=7.1|SL=c0" -c:a pcm_s24le v7_SL.wav
  ```

  En Python: remuestrear la voz y escribirla en la columna *k* de la matriz (n, 8).

## B.5 Audacity y Reaper

**Audacity [V, manual].**
- En Export → Configure → «Advanced Mixing Options» se asignan pistas a canales.
- Admite hasta 32 canales en WAV y 8 en FLAC.
- Qué máscara escribe: **[NV]**. Por usar libsndfile, puede salir 0xFF. Hay que verificarla y, si hace falta, re-etiquetar:

  ```bash
  ffmpeg -i audacity8.wav -af "channelmap=map=0-FL|1-FR|2-FC|3-LFE|4-BL|5-BR|6-SL|7-SR:channel_layout=7.1" -c:a pcm_s24le ok_71.wav
  ```

**Reaper [NV / V-búsq]; el manual dio 404.**
- Pasos: pista y master con 8 canales, ReaSurroundPan o envíos directos, y render del master a 8 canales en WAV de 24 bit y 96 kHz.
- **Riesgo:** el orden SMPTE de algunos DAW (L R C LFE **Ls Rs** Lrs Rrs) pone los laterales en 5/6. En 0x63F los **traseros** van en 5/6. Si se etiqueta tal cual, **se cruzan laterales y traseros**.
  - Corrección: `channelmap=map=0-FL|1-FR|2-FC|3-LFE|6-BL|7-BR|4-SL|5-SR`.
  - Referencia: https://forum.juce.com/t/vst3-side-and-and-rear-channels-flipped/61858

## B.6 Niveles

- **Dolby [V]:** https://professional.dolby.com/en-in/gaming/tutorial/atmos-design-studio/
  - Ruido rosa a 79 dB SPL, ponderación C, respuesta lenta, por canal principal. Se puede calibrar más bajo.
  - LFE: +10 dB en banda (20–80 Hz) medido con RTA, o +4 a +6 dB con sonómetro C.
- **−20 dBFS RMS = 85 dBC en cine**, y 79–82 dBC en salas chicas [V-búsq; SMPTE RP 200 citado en foros].
- **Banda de 500 Hz a 2 kHz:** práctica común [NV]. El MPEG-H la usa [V].
- **Propuesta (criterio, no norma):**
  - Primeras pruebas a **−30 dBFS RMS** con el receptor bajo.
  - Calibración a −20 dBFS RMS.
  - LFE al mismo nivel digital; el receptor le suma +10 dB.
  - Normalizar por **RMS**. El ruido rosa tiene un factor de cresta de 10–12 dB, así que a −20 RMS los picos quedan cerca de −8 dBFS [NV].

## B.7 Empaquetado con video y domo

- **MKV [V]** (https://www.matroska.org/technical/codec_specs.html):
  - Admite `A_PCM/INT/LIT` y `A_FLAC`.
  - La especificación **no fija el orden** de los canales PCM multicanal; depende del reproductor.
- **FLAC de 8 canales [V, RFC 9639]:** el orden fijo coincide con 0x63F. Por eso **FLAC 7.1 dentro de MKV es la opción más limpia** con video.
- **Evitar:**
  - MP4 con PCM: hay errores de 24 bit en `ipcm` [V-búsq, ticket 10359 de ffmpeg].
  - AAC 7.1: la configuración 7 es ambigua [V-búsq, tickets 2153 y 8656 de ffmpeg].
- **Pizarra de video sincronizada** con el nombre del canal [NV-ej]. Un `drawtext` por tramo con `enable='between(t,0,4)'`:

  ```bash
  ffmpeg -f lavfi -i "color=c=black:s=4096x4096:r=30:d=32" -i secuencia_71.wav -filter_complex \
    "[0:v]drawtext=fontfile='C\:/Windows/Fonts/arial.ttf':fontsize=220:fontcolor=white:x=(w-tw)/2:y=h*0.75:text='FRONTAL IZQ (FL)':enable='between(t,0,4)'[v]" \
    -map "[v]" -map 1:a -c:v libx264 -pix_fmt yuv420p -c:a flac prueba_71_domo.mkv
  ```

- **Domo, según IMERSA [V]** (https://imersa.org/guidelines):
  - *Dome master*: fotograma cuadrado con fisheye equidistante, hasta 4096² o más, a **30 fps**.
  - Audio en estéreo, 5.1 o 7.1, entregado como WAV o AIFF mono discretos.
  - Sincronía: el primer segundo de audio coincide con el primer cuadro.
  - Sufijos de archivo `_L _R _C _LFE _Ls _Rs`; no hay convención publicada para 7.1.
  - Convenciones de Digistar y Sky-Skan: no se encontraron [NV].
- **Reproductores:**
  - MPC-BE o MPC-HC con LAV y MPC Audio Renderer en WASAPI exclusivo, con «bit-exact» activado [V-búsq].
  - VLC ha tenido errores de orden en 7.1 [V-búsq]: **no usarlo como referencia de mapeo**.
  - La referencia es **Play Dolby**, que pasa 8 canales sin interpretarlos.

## B.8 Advertencias

**1. Orden de canales por formato:**

| Formato | Orden | Estado |
|---|---|---|
| WAV 0x63F / FLAC de 8 canales | FL FR FC LFE BL BR SL SR | [V] |
| Vorbis / Opus familia 1 | FL **FC** FR **SL SR** BL BR **LFE** | [V] Vorbis spec, RFC 7845 |
| AC-3 / E-AC-3 interno | L C R Ls Rs LFE (ffmpeg reordena al decodificar) | [NV] |
| Pro Tools «Film» | L C R Ls Rs LFE | [NV] |
| SMPTE (DAW) | L R C LFE Ls Rs Lrs Rrs: los laterales van en 5/6 | [NV] |

**2. Traseros frente a laterales.**
- Windows: `5.1` = BL/BR (0x3F) y `5.1(side)` = SL/SR (0x60F) [V].
- Al llevar 5.1 a un 7.1 de cine, **Ls/Rs → SL/SR** [NV-ej]:

  ```bash
  ffmpeg -i in51.wav -af "pan=7.1|FL=FL|FR=FR|FC=FC|LFE=LFE|SL=BL|SR=BR" -c:a pcm_s24le out71.wav
  ```

  Si el origen es `5.1(side)`, usar `SL=SL|SR=SR`. Qué hacer con BL/BR (silencio o algún reparto) **lo decide Luis**.
- En el receptor, SL/SR suele llamarse «Surround» y BL/BR «Surround Back» [NV]. Confirmarlo con la prueba de voz.

**3. Receptor durante las pruebas [NV].**
- Modo *Direct* o *Multichannel*, sin *upmix* (Dolby Surround, Neural:X).
- Corrección de sala (AccuEQ/Dirac) apagada al identificar canales.
- Revisar la gestión de graves (Small/Large, cruce, filtro del LFE).

**4. Protección de bocinas** (buena práctica general [NV]):
- Nunca seno a fondo de escala; arrancar a −30 dBFS.
- Sin tonos agudos (>4–5 kHz) sostenidos a nivel alto.
- Nada subsónico: pasa-altos de 20 Hz en el LFE.
- Rampas de 20–100 ms al entrar y salir, para evitar clics.
- Sin DC.
- Ojo con la excursión del sub: el receptor suma +10 dB al LFE.
- Preferir ruido en banda a seno puro en pruebas largas.

---

## Pendientes, sin verificar en sitio

1. Qué hace Play Dolby con un WAV de 48 kHz cuando el flujo va a 96 kHz.
2. Máscaras reales de los archivos A3 y A7 (Fraunhofer y BigSoundBank), revisadas con MediaInfo.
3. Si los enlaces de Google Drive de la lista de Radxa (D1) siguen vivos.
4. Correspondencia en el DRX-8.4 entre SL/SR → «Surround» y BL/BR → «Surround Back»: confirmarla con A1 o con nuestra secuencia de voz.
5. Filtro y cruce del LFE en el DRX-8.4.

## Fuentes abiertas por los subagentes

**Microsoft Learn:**
- WAVEFORMATEXTENSIBLE: https://learn.microsoft.com/en-us/windows/win32/api/mmreg/ns-mmreg-waveformatextensible
- Máscaras de canales: https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/channel-mask
- SAPI: https://learn.microsoft.com/en-us/dotnet/api/system.speech.synthesis.speechsynthesizer.setoutputtowavefile

**libsndfile:**
- Documentación: https://libsndfile.github.io/libsndfile/command.html
- Código fuente: https://raw.githubusercontent.com/libsndfile/libsndfile/master/src/wav.c (además de `wavlike.c`, `sndfile.c` y `include/sndfile.h`)
- python-soundfile: https://raw.githubusercontent.com/bastibe/python-soundfile/master/soundfile.py

**SoX:**
- Código: https://raw.githubusercontent.com/chirlu/sox/master/src/wav.c
- Manual: https://manpages.debian.org/bookworm/sox/sox.1.en.html

**FFmpeg:**
- https://ffmpeg.org/ffmpeg-utils.html
- https://ffmpeg.org/ffmpeg-filters.html
- https://ffmpeg.org/ffmpeg.html
- https://raw.githubusercontent.com/FFmpeg/FFmpeg/master/libavformat/riffenc.c
- Error de 2010 (TrueHD): https://ffmpeg.org/pipermail/ffmpeg-devel/2010-June/091602.html

**Formatos y códecs:**
- Matroska: https://www.matroska.org/technical/codec_specs.html
- FLAC: https://www.rfc-editor.org/rfc/rfc9639.html
- Opus: https://www.rfc-editor.org/rfc/rfc7845.html
- Vorbis: https://xiph.org/vorbis/doc/Vorbis_I_spec.html

**Herramientas y normas:**
- Audacity: https://manual.audacityteam.org/man/advanced_mixing_options.html
- Dolby: https://professional.dolby.com/en-in/gaming/tutorial/atmos-design-studio/
- IMERSA: https://imersa.org/guidelines
- MPEG-H: https://mpegh.com/?p=1416

**Contenido:**
- https://speakereject.com/surround-sound-test/
- https://speakereject.com/speaker-phase-test/
- https://www2.iis.fraunhofer.de/AAC/multichannel.html
- https://mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/Samples.html
- https://github.com/sfiera/flac-test-files
- https://bigsoundbank.com/test-7-1-surround-s3123.html
- https://www.demolandia.net/cinema/dolby-demo-trailers-hd.html
- https://www.demolandia.net/cinema/dts-demo-trailers-hd.html
- https://forum.radxa.com/t/hd-audio-passthrough-testing/1864
- https://thedigitaltheater.com/
- https://eso.org/public/videos/au-totss-fulldome
- https://www.utoledo.edu/nsm/rpbo/fulldomestudio/fdfa-hr.html
- https://media.xiph.org/
- https://freesound.org/search/?q=surround&f=channels%3A8
- https://hometheater.substack.com/p/atmos-audio-test-files

**No se pudieron abrir:**
- Manual de Reaper (404)
- Merge request de VLC (bloqueo antibots)
- Melbourne Planetarium (403)
- PDF de Auro-3D
- Wiki de Kodi (403)
- Copias de demo-world.eu (HTTP 500)
