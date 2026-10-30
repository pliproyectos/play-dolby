# Estado del paquete — 2026.10.30.1

La carpeta contiene el EXE PlayDolby.exe recibido de Luis, compilación local del 28 de octubre, sin recompilar. La extensión es PLI Espacial 2026.10.29.3.

## Probar el 7.1 sin internet

Ejecuta `GENERAR-PRUEBAS-7.1.bat`; elige 48 kHz o 96 kHz y la carpeta donde guardar. Windows crea cuatro WAV PCM de 24 bits, ocho canales discretos y nivel máximo −30 dBFS: un tono por bocina, una vuelta por los siete canales principales, estéreo separado por FL/FR y tono solo por FC. Son tonos técnicos, no música. El generador usa PowerShell y .NET que ya incluye Windows; no instala ni descarga nada. Cada sesión crea una carpeta nueva. Mantén el Integra a volumen bajo.

## Chrome junto con la Qu-5

1. Abre Play Dolby, elige Qu ASIO Driver y el receptor HDMI e inicia el puente.
2. Ejecuta `ACTUALIZAR-EXTENSION.bat` y selecciona la carpeta de PLI Espacial que ya está instalada en Chrome.
3. Pulsa Recargar en `chrome://extensions`; activa la captura en la pestaña musical y elige **Con Qu-5 / Play Dolby**. El EXE mezcla ambas fuentes y controla HDMI.
4. Si se interrumpe la conexión de Chrome, usa **Reconectar Chrome**. El diagnóstico muestra el ID permitido por el EXE.
5. Para Chrome solo, detén el puente del EXE y selecciona **Solo Chrome**.

GitHub contiene el ZIP actualizador del EXE y la extensión, sin WAV de demos ni música. Los audios locales continúan en esta USB. El código fuente del EXE del 28 de octubre está en la otra computadora; el código native de este repositorio es anterior. Falta confirmar la reproducción prolongada con Qu-5 e Integra en sala.
