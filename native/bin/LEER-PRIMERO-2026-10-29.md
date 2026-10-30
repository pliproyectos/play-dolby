# Paquete actual de PLI — 2026.10.29.3

Este paquete parte de la carpeta actual entregada por Luis en USB. PlayDolby.exe conserva exactamente el binario recibido del 28 de octubre (SHA-256 1A589715DE11677395B7FF929B38603349A73D452470A837957121ACD8027218). La extensión se actualizó a 2026.10.29.3.

## Chrome y Qu-5 simultáneamente

1. Abre el EXE recibido, selecciona Qu ASIO Driver y el receptor HDMI e inicia el puente. La app debe ser la única que abre el HDMI.
2. Actualiza la extensión en SU MISMA CARPETA instalada. Usa ACTUALIZAR-EXTENSION.bat y selecciona esa carpeta, o copia allí el contenido de chrome-extension. No la elimines de Chrome ni vuelvas a instalarla en otra ruta.
3. En chrome://extensions pulsa Recargar. En la pestaña de música, pulsa la extensión y elige «Con Qu-5 / Play Dolby · ambas fuentes juntas». Activa la captura.
4. El estado «Enlace con Play Dolby activo» confirma la conexión, no la salida física. Comprueba que el puente del EXE está iniciado y su entrada Chrome no está en Silencio.
5. Si se interrumpe Chrome, pulsa «Reconectar Chrome». Reconstruye únicamente su procesador y transporte; no reinicia el EXE ni cambia la consola.

Para usar Chrome SIN el EXE: detén el puente del EXE y elige «Solo Chrome». Configura Windows con el receptor como salida predeterminada en 7.1. No uses esta salida directa para mezclar con la Qu-5 en un EXE que mantiene HDMI en exclusivo.

## Identificador y límites conocidos

El EXE actual acepta solo la extensión chrome-extension://blhfjpfedgoeolfcpejpehpikainihpd. Copiar o reinstalar una extensión sin clave a otra ruta puede cambiar su identificador. El botón de diagnóstico muestra el identificador actual y el esperado. Un fallo de enlace puede ser por la app cerrada, puente detenido, puerto ocupado o identificador rechazado; no se interpreta automáticamente como «app cerrada».

La corrección anterior forzaba la salida independiente; ese ajuste puede competir con el EXE. Esta versión propone modo conjunto una vez tras actualizar y permite elegir explícitamente Solo Chrome. Cambiar al modo conjunto libera primero el contexto de salida directa.

Esta actualización añade: logo PLI, operaciones de captura serializadas, protección frente a eventos de sesiones antiguas, timeout de captura, timeout de conexión, reconexión por atasco de envío, recuperación limitada del procesador, diagnóstico y reconexión manual sin volver a capturar la pestaña. El transporte conserva PLI1, 48 kHz y ocho canales discretos.

## Validación y trabajo pendiente

Pasaron 37 pruebas del motor, pruebas de protocolo y AudioWorklet en Chrome real con salida silenciosa, y pruebas simuladas de fallos de conexión, cola atascada, operaciones concurrentes y recuperación. La sala con Qu-5 e Integra no está conectada a esta computadora; no se ha confirmado aquí la estabilidad acústica ni la mezcla física prolongada.

El código fuente correspondiente al EXE del 28 de octubre permanece en la otra PC. El código native del repositorio es anterior a esa integración y NO reproduce este binario. No reemplazar este EXE por una compilación del código antiguo. Para corregir bloqueos internos del EXE y eliminar de forma segura la dependencia del ID fijo, traer la carpeta native actualizada de la otra PC.

## Uso sin internet y GitHub

La otra computadora puede recibir todo por USB. El paquete completo incluye demos y herramientas; el paquete pequeño de actualización incluye el programa y la extensión sin demos.

El actualizador consulta Releases de GitHub, requiere ZIP y SHA-256 y pide confirmación para instalar. Chrome necesita Recargar y, si usa una carpeta externa al programa, copiar antes los archivos con ACTUALIZAR-EXTENSION.bat. La versión del paquete 2026.10.29.3 no significa que se haya recompilado PlayDolby.exe.
