# Kit de pruebas del domo — contexto para Claude

Lo lees porque Luis abrió una sesión en esta carpeta, probablemente desde la USB (`D:\PARCHE DOLBY\pruebas-domo\`) y en otra PC con Windows 11. La memoria de la sesión anterior **no viaja**: lo que necesitas está aquí. Para el detalle humano, lee `LEEME.md`; para el proyecto completo, `..\SPEC.md`.

## Qué es

Instrumento de prueba para la cadena del planetario: consola Qu-5 → USB/ASIO → PC con **Play Dolby** (C++, PortAudio) → HDMI (GPU AMD, WASAPI) → **Integra DRX-8.4** → domo 7.1. Los WAV van en el orden 7.1 de Windows, máscara **0x63F**: FL FR FC LFE BL BR SL SR, PCM de 24 bit, a 48 y 96 kHz. La cabecera se escribe a mano en `wav71.py`; libsndfile escribe 0xFF por omisión y por eso no se usó.

## Estado al 2026-10-20 (v1.0.0, hecho en casa, sin conexión al domo)

- **VERIFICADO en escritorio:** 20 WAV contra sus manifiestos (`salida/VERIFICACION.md`). El `wave` de CPython los lee con los mismos bytes. El verificador detectó los cinco daños que se le metieron a propósito. La copia en la USB se verificó con sus huellas SHA-256.
- **NO VERIFICADO:** nada de la cadena física. Nunca se ha escuchado en el domo.
- **`domo.json` es PROVISIONAL:** ángulos nominales. Cambian el orden de la vuelta y de los pares de polaridad.

## Qué hacer al llegar

1. Pregunta a Luis dónde está: ¿en el domo, conectado? ¿En qué PC?
2. **Si está en el domo**, guíalo por P1 con `hoja-de-campo.html`:
   - cerrar Play Dolby, porque ocupa WASAPI exclusivo;
   - Windows en 7.1;
   - el Integra a volumen bajo, en modo Direct o Multichannel y sin AccuEQ;
   - reproducir 01 → 02 → 03 → 06 → 04;
   - anotar lo que muestra la pantalla de información del Integra.
3. Después siguen P2 (tonos internos de Play Dolby), P3 (consola) y P4 (Nightshade). La falla queda entre el último punto que pasa y el primero que falla.
4. Si trae la ficha medida, actualiza `domo.json` (pon `estado` como medido) y corre `generar-todo.bat`. Si cambian los nombres de las voces, corre primero `voces.ps1`.
5. Si esta PC tiene Python 3.12 o más nuevo, `verificar.bat` confirma que los archivos llegaron intactos.

## Hoja de ruta (plan aprobado el 2026-10-20)

| Fase | Qué | Estado |
|---|---|---|
| 0 | Ficha del domo medida | pendiente, en sitio |
| 1 | Generador y programas 01–06 | **hecho** |
| 2 | Recorrido P1→P4 con acta (etapa B de la SPEC) | pendiente, en sitio |
| 3 | Claqueta en domemaster y sincronía audio-video ≤40 ms (etapa E) | necesita ffmpeg y los formatos de Nightshade |
| 4 | Medición con micrófono: un solo archivo con barridos y clics de referencia para la deriva | necesita micrófono y numpy |
| 5 | Que Play Dolby use la ficha y las secuencias | corresponde al código de `native/` |

## Reglas de Luis que aplican aquí

- Español técnico.
- Separar lo verificado de lo no verificado; no digas «funciona» sin haberlo corrido.
- Las decisiones de ingeniería y de sala son suyas: nivel de calibración, qué va a BL/BR con contenido 5.1, dónde queda el frente.
- No instalar nada sin preguntar (ffmpeg, numpy).
- Las salidas van a la carpeta del proyecto.
- **Protección de bocinas:** el techo está en `domo.json`; nunca subir el volumen del Integra de golpe.
