# PARCHE DOLBY — contexto para Claude

Planetario con domo 7.1. Cadena: consola **Allen & Heath Qu-5** → USB/ASIO → PC Windows con
**Play Dolby** (C++, PortAudio, `native/`) → HDMI de GPU AMD (WASAPI) → receptor **Integra DRX-8.4**
→ amplificadores **NEXO NXAMP4x1 Mk2** → bocinas NEXO ePS10/ePS8-EN54 y subwoofer eLS18.
Especificación y etapas: `SPEC.md`. Ficha de equipos y preguntas abiertas de sala:
`EQUIPOS-SALA.md`. Kit de señales de prueba: `pruebas-domo/` (tiene su propio `CLAUDE.md`).

La memoria de Claude **no viaja** entre PCs: este archivo es la memoria del proyecto. Estado, no
bitácora: actualízalo en sitio cuando algo cambie de «no verificado» a «verificado».

## Verificado en el domo — 2026-10-19 (PC de Luis, sesión con el sistema conectado)

- **El Integra anuncia LPCM de 8 canales** por EDID (hasta 192 kHz), además de AC-3, E-AC-3, DTS-HD
  y TrueHD. El hardware sí soporta 7.1.
- **Causa de «solo estéreo» en Windows:** la salida HDMI «AV Receiver» estaba configurada en
  2 canales, y **FxSound** (dispositivo virtual de 2 canales) se interponía. Arreglo: cerrar
  FxSound, `mmsys.cpl` → la salida HDMI → Configurar → **7.1 envolvente**. Quedó en 8 canales con
  máscara 0x63F (FL FR FC LFE BL BR SL SR). Es configuración **por PC**: en otra máquina se repite.
- **La Qu-5 entrega USB a 96 kHz** (ASIO rechaza 48 kHz). Esa era la causa de los
  `Invalid sample rate` de `bridge.log`. Play Dolby ahora toma la frecuencia de la consola.
- **HDMI en WASAPI exclusivo:** acepta 8 canales a 44,1/48/96 kHz; en compartido, 96 kHz se
  rechaza. El periodo mínimo del HDMI de AMD es de **10 ms (960 muestras)**; no baja más.
- **Latencia del puente:** de ~43 a **~20 ms**, sumando la latencia declarada por los drivers y la
  cola medida. La cola es salida + entrada + margen de 192; con margen 32 hubo 50 faltas en 30 s.
  ASIO se fija en 64 muestras. Prueba física de 2 min en silencio: 0 faltas, 0 desbordes,
  0 avisos del driver.
- Se escucharon en sala, por VLC, `prueba-7.1.wav` y las versiones 7.1 de `musica-7.1/`. A Luis
  le gustó la órbita y pidió mover instrumentos: de ahí salió `capas71`.

## No verificado

- Latencia **acústica** real, que incluye el procesamiento del Integra.
- La interfaz gráfica de Play Dolby con los cambios de latencia: se compiló, no se operó.
- Una función completa de 60 min. Sincronía con el video de Nightshade.
- El control del Integra por red (eISCP, puerto 60128): no respondió desde la Wi-Fi «IPN».
  Necesita estar en la misma red local que la PC.
- El modo de escucha del Integra (Multichannel/Direct) se pone a mano con el control.
- Todo lo que hay después del Integra (NXAMP, presets, cruce de graves, retardos): se sabe por los
  manuales, no por inspección en sitio. Ver `EQUIPOS-SALA.md`.

## 2026-10-22 — ventana, guía y preparación para GitHub

- **Ventana de Play Dolby:**
  - normal de Windows (minimizar, maximizar, bordes; tamaño mínimo del 70 %);
  - se quitó «Pantalla completa · F11»;
  - el interruptor de Dynamic Space pasó del encabezado a la columna de escenas.
- **Guía de usuario:** `docs/GUIA-DE-USUARIO.md`, con capturas en `docs/img/`. También va en el
  instalador.
- **Repositorio Git** iniciado en la raíz, con rama `main`. **Sin commits ni remoto todavía:** Luis
  pasará la cuenta.
  - `.gitignore` deja fuera Zig (`zig.exe` pesa 164 MB, más que el límite de GitHub; se baja con
    `native/tools/obtener-zig.ps1`, que verifica SHA-256), `ear-venv`, `native/bin`, las cachés y
    las salidas generadas. Se suben 612 archivos (146 MB).
  - `portaudio.dll` vive en `native/vendor/`; los `build-*.ps1` la copian a `bin`.
  - libiamf ya no trae `.git` (procedencia en `native/vendor/iamf/PROCEDENCIA.md`).
  - Verificado: un clon simulado compila los 3 builds con las pruebas en verde.

## Cierre temporal de sesión — 2026-10-21 (el proyecto se comparte por Drive)

**Hecho entre el 2026-10-20 y el 2026-10-21, en la laptop de Luis y sin la Qu-5.** Todo con pruebas
automáticas: 65 452 comprobaciones, 0 fallas. Detalle por fase en `PLAN-MOTOR-ESPACIAL-PLI.md`.
- **PLI Espacial**, el motor espacial propio en lugar de pagar Dolby Atmos:
  - fase 1: VBAP 2D desde `domo.json`;
  - fase 2: escenas JSON y `escena71`;
  - fase 3: objetos en vivo dentro de Play Dolby;
  - fase 3b: Dynamic Space en la ventana principal;
  - fase 4: ADM (`adm71`, validado con el renderizador EBU EAR) e IAMF solo PCM (`iamf71`, bit por
    bit igual a la referencia de AOMedia).
- **Fuente archivo:** Play Dolby puede tomar un WAV o una carpeta de capas como si fuera la consola.
  Se agregó porque, sin la Qu-5, el puente se negaba a arrancar y no sonaba nada. También hay
  objetos estéreo.
- **Escuchado por Luis (VLC → receptor por HDMI en 7.1):**
  - `musica-7.1/pli-espacial*/`: le gustó;
  - tramo 1:45–2:00 del remix: le gustó;
  - «Cosmic» original: notó «distorsión en algunas partes». El archivo no recorta; falta saber el
    minuto. Se hizo una variante «estable», sin el acelerón de 2:00–2:30.
- **Portabilidad:**
  - las escenas guardan la ficha con ruta relativa (`../../pruebas-domo/domo.json`);
  - `native/tools/ear-venv` no es portable: se recrea con `native/tools/crear-ear-venv.ps1`;
  - `README.md` dice qué se puede omitir al subir.

**Siguiente sesión, en este orden:**
1. Luis prueba en sala Play Dolby con «▶ Carpeta de capas…» (`musica-7.1/pli-espacial-cosmic/capas`).
   Nunca se ha oído el puente arrancado desde un archivo.
2. Si Luis dice en qué minuto distorsiona Cosmic, revisar ese tramo (fuente al tope, acelerón o
   separación).
3. Cuando la Qu-5 esté habilitada:
   - diagnóstico y `Prueba Hardware.bat` en la PC de la sala;
   - objetos en vivo por el puente durante 30 min, sin faltas;
   - medir los ángulos reales y pasarlos a `domo.json` y a `native/espacial/ear-domo.yaml`.
4. Opcional, con permiso de Luis para descargar: códecs Opus/AAC/FLAC para IAMF.

## Estado al 2026-10-20 y siguiente paso

- **Migración de PC:** la PC con la que se trabajó en el domo (GPU AMD) deja de usarse. El paquete
  completo está en la USB `D:\PARCHE DOLBY`, sincronizado y verificado con SHA-256.
- **PC nueva:** el proyecto se copió completo a la laptop de Luis, en `Desktop\AUDIO DIGITAL`
  (17 104 archivos, mismo tamaño que la USB). Desde el 2026-10-21 también se comparte por Drive: la
  ruta puede ser otra. Aquí todavía no se corre el diagnóstico ni la prueba
  de hardware.
- **Nueva ruta de audio (Luis, 2026-10-20):**
  - Una tarjeta de audio externa con 8 salidas de plug lleva el audio a la Qu-5 por 4 cables
    estéreo a XLR, con la entrada de la consola configurada como estéreo.
  - **Objetivo inmediato:** que Windows saque 7.1 por esa tarjeta. La parte de la Qu-5 va después.
  - Diagnóstico de esta PC (laptop, 2026-10-20): Realtek estéreo, un monitor C24F390 estéreo por
    HDMI y GPU NVIDIA. No hay tarjeta externa, Qu ni Integra conectados. **Nahimic está abierto** y
    **FxSound está instalado**: los dos estorban al 7.1.
- **Respuestas de sala (Luis, 2026-10-20)**, detalle en `EQUIPOS-SALA.md`:
  - Las ePS10 están en FL, C y FR, acostadas y con el motor girado.
  - Las entradas son XLR.
  - **El cruce de graves lo hace el NXAMP.**
  - **Los retardos los pone el NXAMP.**
  - Dirac y modo de escucha: no se sabe; se revisa con la lista paso a paso de `EQUIPOS-SALA.md`.
  - **No hay liga con la alarma de incendio** (nota para la institución:
    `NOTA-INSTITUCION-EN54.md`).
- **Consecuencia:** el Integra debe quedar con las bocinas en Large, las distancias iguales y en
  modo Direct. Al eLS18 solo llega el LFE. Si el contenido es estéreo convertido, el sub no suena a
  menos que se genere LFE. Generarlo ya no está vetado, pero es una decisión de Luis y hay que
  probarlo en sala. **No tocar presets del NXAMP.**
- **Falta:**
  - Las posiciones de las ePS8.
  - Cuántos eLS18 y NXAMP hay, y el preset de cada canal.
  - Cómo salen por XLR los envolventes (el Integra solo trae XLR en FL/C/FR/SW).
  - El estado de Dirac.
  - Las distancias y ángulos medidos para `pruebas-domo/domo.json`.

## Herramientas (todas en `native/bin/`; se compilan con `native/build-cinematic.ps1`, `build-espacial.ps1` y `build-iamf.ps1`, que usan el Zig portátil de `native/tools/`)

| Qué | Cómo |
|---|---|
| Diagnóstico de audio de la PC (solo lectura) | `pwsh -File native\scripts\diagnostico-audio.ps1` |
| Prueba física Qu → HDMI, 2 min en silencio | `Prueba Hardware.bat` (cerrar Play Dolby antes) |
| Tonos por bocina | `prueba-7.1.wav` (lo genera `native/scripts/generar-prueba-71.py`) |
| Estéreo → 7.1 conservador + órbita | `native\bin\upmix71.exe entrada.wav carpeta` |
| Estéreo → 6 capas que se mueven por separado | `native\bin\capas71.exe entrada.wav carpeta` |
| Lo mismo con **PLI Espacial** (VBAP 2D con los ángulos de la ficha) | `native\bin\capas71.exe entrada.wav carpeta --domo pruebas-domo\domo.json` |
| Escena editable → 7.1 (PLI Espacial fase 2; `capas71` escribe `escena.json`) | `native\bin\escena71.exe escena.json` · formato en `native/espacial/ESCENAS.md` |
| **PLI Espacial en vivo** (fase 3): objetos desde canales USB, arrastrables | Play Dolby → botón «✦ PLI Espacial». Motor `native/espacial/vivo.hpp`, panel `native/src/espacial_panel.hpp`. Sin probar con la Qu-5 |
| **Dynamic Space** (fase 3b) en la ventana principal | Insignia, domo en vivo, barras por salida, escenas rápidas y velocidad maestra. Vista previa sin consola |
| **ADM** (fase 4): escena ↔ archivo ADM BW64 (ITU-R BS.2076) | `native\bin\adm71.exe exporta escena.json salida.wav` · `adm71.exe importa entrada.wav carpeta --domo pruebas-domo\domo.json` |
| **IAMF** → 7.1 de Windows (solo PCM) | `native\bin\iamf71.exe entrada.iamf salida.wav` (compilar con `native\build-iamf.ps1`). Reordena ITU → Windows |
| Validar un ADM con el renderizador de referencia EBU | `$env:PYTHONUTF8=1; native\tools\ear-venv\Scripts\ear-render.exe -s 0+7+0 -l native\espacial\ear-domo.yaml entrada.wav salida.wav` |
| **Fuente archivo** (sin consola) | En «01 Conecta tu consola»: «▶ Archivo WAV…» (sus canales son USB 1…N) o «▶ Carpeta de capas…» (configura 6 objetos estéreo de PLI Espacial y pone las rutas directas en Silencio). Copia exacta sin servo; se repite |
| **Instalador / USB portable** (2026-10-22) | `pwsh -File native\empaquetar-usb.ps1 -Destino "D:\PLAY DOLBY" -Reemplazar` (`-Reemplazar` solo borra un paquete anterior de Play Dolby). Compilar antes con los tres `build-*.ps1`. Fuentes en `instalador\`. `INSTALAR.cmd` instala sin administrador en `%LOCALAPPDATA%\Programs\Play Dolby` (accesos y «Aplicaciones instaladas»; conserva `domo.json`); `ABRIR SIN INSTALAR.cmd` corre desde la USB. Play Dolby busca `domo.json` junto al exe primero |
| **Actualizaciones con aviso** (2026-10-22) | Terminales: `actualizador.ps1` (en `instalador/`; va junto al exe) revisa los Releases de `pliproyectos/play-dolby` al prender la PC (acceso en Inicio de Windows) y cada 12 h con Play Dolby abierto (`native/src/actualizacion_ui.hpp`, botón «⟳ Actualización»). Verifica SHA-256, nunca instala en vivo, conserva `domo.json` y se deshace sola si la versión nueva no arranca. Publicar: `pwsh -File native\publicar-version.ps1 -Version AAAA.MM.DD` y crear el Release `vAAAA.MM.DD` con los 2 archivos. Probado de punta a punta con Releases locales (11/11) |
| Pruebas + compilación de PLI Espacial | `pwsh -File native\build-espacial.ps1` (motor en `native/espacial/pli_espacial.hpp`) |
| Leer manuales en PDF sin poppler | `python -I native\scripts\pdf-texto.py manual.pdf` |

La canción de ejemplo está en `musica-7.1/origen/`. Las versiones 7.1 se regeneran desde ahí en
menos de 30 s. Para reproducir en VLC directo al receptor:
`vlc --mmdevice-audio-device=<id de la salida HDMI> archivo.wav`. El id lo da el diagnóstico.

## Al llegar a una PC nueva

1. Corre el diagnóstico. ¿El receptor anuncia 8 canales LPCM? ¿Windows lo tiene en 7.1? ¿Hay
   FxSound?
2. Si no hay `native/bin/*.exe` o fallan, recompila con `native/build-cinematic.ps1`: corre las
   pruebas unitarias antes de compilar.
3. Con la Qu-5 conectada: `Prueba Hardware.bat`. Debe decir PASA.
4. Si el GPU no es AMD, el periodo HDMI y la latencia cambian: vuelve a medir con
   `hw_bridge_test --run 30 --exclusive --margin N`.

## Reglas de Luis que aplican

Español técnico. No afirmar «funciona» sin correrlo. Las decisiones de sala y de ingeniería son
suyas (niveles, qué va a cada bocina, dónde queda el frente). No instalar nada sin preguntar.
Salidas en la carpeta del proyecto. Volumen del Integra bajo al probar.
