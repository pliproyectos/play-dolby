# Play Dolby · PLI Ingeniería Aplicada

Versión del paquete: **2026.10.29.3**. Base actual: carpeta entregada por Luis desde la PC del domo, con el EXE del 28 de octubre y la extensión actualizada.

## Empezar

- **[Play Dolby.bat](Play%20Dolby.bat)** abre el programa actual, [native/bin/PlayDolby.exe](native/bin/PlayDolby.exe).
- **[Actualizar extension Chrome.bat](Actualizar%20extension%20Chrome.bat)** actualiza la extensión en su carpeta instalada, conserva su identificador y crea un respaldo.
- **[Descargar para USB](https://github.com/pliproyectos/play-dolby/releases/tag/v2026.10.29.3)**: paquete completo con los WAV de prueba y paquete pequeño de actualización.

Para Chrome y Qu-5 simultáneamente, el EXE mezcla ambas fuentes y controla el HDMI. En la extensión elige **Con Qu-5 / Play Dolby**. Para Chrome independiente, detén el puente del EXE y elige **Solo Chrome**.

Después de copiar la extensión, pulsa **Recargar** en chrome://extensions y vuelve a activar la captura en la pestaña de música. Conserva la carpeta instalada: el EXE actual acepta un identificador específico. El botón **Ver diagnóstico de conexión** permite comprobarlo.

## Qué contiene el repositorio

- `native/bin/`: EXE actual recibido, DLL, herramientas, configuración y guía operativa.
- `chrome-extension/`: extensión actual, logo PLI, motor 7.1 y pruebas de conexión.
- `docs/ESTADO-ACTUAL-2026-10-29.md`: uso, comprobaciones y pendientes de sala.
- `docs/PAQUETE-ACTUAL-2026-10-29.json`: inventario SHA-256 del paquete completo, incluidos los audios que se distribuyen por Release.
- `native/src/`, `native/espacial/`, `native/tests/`: código disponible y pruebas del proyecto.

**El código fuente del EXE del 28 de octubre permanece en la otra PC.** El código `native/src` disponible es anterior a la integración con Chrome y no reproduce ese ejecutable. Se conserva el binario recibido sin recompilar. El build de desarrollo usa otro nombre para no sustituirlo por accidente.

Las propuestas antiguas, duplicados de ejecutables y ZIP de respaldo se retiraron del árbol actual; siguen recuperables en el historial de Git. Los WAV grandes están en el paquete completo del Release.

## Validación

La extensión pasó 37 pruebas del motor, pruebas de transporte, concurrencia y recuperación, además de una prueba con Chrome real, ocho canales y tres desconexiones de red. Esa prueba usa audio sintético y salida silenciosa; falta verificar en sala la mezcla prolongada de Qu-5, HDMI y Chrome.

Consulta [las instrucciones de esta versión](docs/ESTADO-ACTUAL-2026-10-29.md). La otra PC puede actualizarse desde USB sin internet.
