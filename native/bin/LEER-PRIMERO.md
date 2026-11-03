Play Dolby 2026.11.3.2 — orientación de salida y Chrome reconstruido

Se reemplazaron Quieto, Órbita, Respira y La de 1:45 por 0°, +90° (derecha), −90° (izquierda) y 180°. Se gira la mezcla de consola/archivo y Chrome. El subwoofer conserva su canal. Entre las bocinas físicas se distribuye el sonido según el ángulo. El probador sigue identificando las bocinas físicas, sin giro.

Al activar un giro aparecen controles ámbar y GIRO ACTIVO con el ángulo. Al cambiar película pulsa 0° · Normal / Restablecer. Cada apertura empieza en 0°. El giro permanece al detener e iniciar el puente durante la misma sesión.

Extrae todo el ZIP. Cierra el EXE anterior y abre esta copia. En Chrome usa la extensión incluida, versión 2026.11.3.1, con ID ebpjbolnlhmbebbhiooonakajihpknco. Elige Con Qu-5 / Play Dolby y activa la captura. Selecciona consola ASIO o Chrome PLI · sin consola en el EXE, el receptor HDMI configurado en 7.1 e inicia el puente. El nivel Chrome tiene control independiente.

La conexión de Chrome se reconstruyó en native/src/browser_audio.hpp. El ejecutable se compila íntegramente desde las fuentes de GitHub. No requiere modificaciones binarias. La salida es 7.1 WASAPI compartida. Las pruebas verifican giro, energía por canal, subwoofer, WebSocket con origen autorizado, paquetes PCM y conversión 48/96 kHz. Se revisaron los botones y el aviso en la interfaz. Falta probar acústicamente con Qu-5, Integra y películas en sala.
