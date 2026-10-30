# Play Dolby + PLI Chrome — actualización local 2026-10-27

## Cambios instalados

- Perfil único 7.1, ocho salidas. Se retiraron estéreo y 5.1 de la selección. Cargar perfiles antiguos también conserva 7.1.
- Ganancia inicial 0 dB. Se conservan las otras ganancias para ajustar el nivel; una configuración cargada restaura la ganancia guardada.
- Nombre visible SUB en la cuarta salida, probador y paneles. Conserva su canal USB y posición en la mezcla.
- La extensión envía por defecto su audio ya procesado en ocho canales a Play Dolby. Chrome no abre el HDMI en este modo.
- Play Dolby mezcla esa señal con la consola o archivo en una cola independiente, sin volver a aplicar el paneo espacial. El desplegable Chrome de la parte superior permite 0, -6, -12 dB o Silencio.
- Se añadió la fuente Chrome PLI para usar solo la extensión sin consola. La salida sigue siendo 7.1.

## Aplicar y usar

Los archivos están actualizados. La aplicación que ya estaba en vivo sigue ejecutando la versión anterior; no se interrumpió su audio.

1. Cuando termines la reproducción actual, cierra y vuelve a abrir Play Dolby.
2. En Chrome, abre chrome://extensions y pulsa Recargar en PLI Espacial 7.1. Conserva la extensión cargada desde C:\Users\ben\OneDrive\Desktop\chrome-extension para mantener su identificador.
3. En la app selecciona Qu ASIO Driver como entrada y el receptor HDMI como salida. Perfil fijo 7.1, ganancia inicial 0 dB. Inicia el puente.
4. En la extensión deja Enviar a Play Dolby y activa la captura en la pestaña de música. Debe indicar Conectado a Play Dolby.
5. Chrome y consola se mezclan en la app; Chrome tiene su propio nivel. La opción Mandar graves a SUB de la extensión controla si genera señal para el subwoofer.
6. Si solo quieres Chrome, elige Chrome PLI como entrada en la app. No necesitas abrir la Qu.

No uses Directo al receptor a la vez que el puente exclusivo: ese modo se conserva para usar la extensión por separado. No hay cambio automático a HDMI si se desconecta la app. Tras cinco reintentos fallidos, detén y reactiva la captura.

## Verificación completada

- Compilación del ejecutable C++ sin errores.
- Motor existente: pruebas de ruteo, búfer, deriva y 65 452 comprobaciones de PLI Espacial aprobadas.
- Búfer Chrome: ocho canales, conversión nominal 48/96 kHz, deriva, límites, silencio y reconexión.
- Servidor: WebSocket real, Origin/ruta, mensajes malformados, ping, reconexión, puerto ocupado y parada acotada.
- Integración sin hardware: WebSocket -> PCM48k -> mezcla96k con señal de consola sintética. Orden de ocho canales, suma, desconexión y reconexión sin audio viejo aprobados.
- Extensión: protocolo, cola acotada y worklet; Chrome real con sink silencioso48k y ocho canales discretos aprobados.

## Pendiente de verificar en sala

No se ha probado captura real de una pestaña junto con Qu, HDMI y bocinas, ni sincronía con video. La cola Chrome objetivo es aproximadamente64ms, más los demás búferes; la conversión usa interpolación lineal y debe evaluarse auditivamente. La cola de consola se mantiene independiente.

El control de aplicaciones bloqueó abrir el ejecutable nuevo para inspección visual por falta de autorización para ese ejecutable. No se hizo esa revisión ni se inició reproducción nueva.

## Archivos y recuperación

Ejecutable instalado: PlayDolby.exe, versión local2026.10.27.1.
Copia previa a esta integración: PlayDolby.antes-puente-20261027.exe.bak.
Fuentes: C:\Users\ben\OneDrive\Desktop\play-dolby-main\native.
Extensión activa en disco: C:\Users\ben\OneDrive\Desktop\chrome-extension.

El receptor solo escucha127.0.0.1:19771/pli-audio y acepta el Origin chrome-extension://blhfjpfedgoeolfcpejpehpikainihpd. Cambiar la ruta/ID de la extensión exige actualizar el Origin permitido al recompilar. No se instala cable virtual ni se abre acceso en la red.

Referencias oficiales de diseño:
- WASAPI exclusivo: https://learn.microsoft.com/en-us/windows/win32/coreaudio/exclusive-mode-streams
- Salida silenciosa de Web Audio: https://developer.chrome.com/blog/audiocontext-setsinkid
