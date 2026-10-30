# Estado del paquete PLI — 2026.10.30.2

El ejecutable PlayDolby.exe de la carpeta se conserva como fue recibido de Luis (versión local 2026.10.28.1); no se recompiló. La extensión PLI Espacial es 2026.10.29.3.

## Pruebas 7.1 sin internet

En la carpeta `herramientas`, ejecuta `GENERAR-PRUEBAS-7.1.bat`. Elige 48 kHz o 96 kHz y un lugar para guardar los resultados. El BAT utiliza PowerShell/.NET incluidos en Windows y crea cuatro WAV PCM de 24 bit, ocho canales discretos, a −30 dBFS máximo:

1. Tonos por bocina en orden FL, FR, FC, LFE, BL, BR, SL, SR.
2. Un tono que recorre los siete canales principales; LFE queda vacío.
3. Señal estéreo: L por FL y R por FR; FC y ambiente quedan vacíos.
4. Tono solo por FC.

Son tonos técnicos, no música. Los archivos se escriben en una carpeta nueva y no reemplazan pruebas anteriores. Mantén el Integra a volumen bajo.

## Chrome junto con Qu-5

1. Abre Play Dolby, elige Qu ASIO Driver y el receptor HDMI y pulsa Iniciar puente.
2. Ejecuta `ACTUALIZAR-EXTENSION.bat` para detectar automáticamente la carpeta `chrome-extension` que está junto al BAT. Después pulsa Recargar en `chrome://extensions`.
3. Sobre la pestaña musical activa la captura y elige **Con Qu-5 / Play Dolby**. La app mezcla ambas fuentes y controla el HDMI.
4. Para Chrome solo, detén primero el puente del EXE y elige **Solo Chrome**. No mantengas simultáneamente la salida directa de la extensión y la salida exclusiva de Play Dolby.
5. Si se interrumpe el flujo de Chrome, pulsa **Reconectar Chrome**; el diagnóstico muestra el ID que el EXE permite.

## GitHub y pendientes

El repositorio recibe el EXE que dejó Luis, la extensión, el generador y las herramientas. El ZIP de actualización de GitHub no incluye los WAV locales de demos ni de música. Para la otra PC, puedes copiar la carpeta completa desde la USB sin conexión a internet.

La fuente del EXE recibido permanece en la otra computadora. `native/src` aquí es de una versión anterior y no debe reemplazar el binario. La mezcla física prolongada con la Qu-5, el Integra y el domo sigue pendiente de confirmar en sala.
