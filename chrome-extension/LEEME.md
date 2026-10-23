# PLI Espacial 7.1 · extensión para Chrome

Convierte el audio **estéreo** de una pestaña (YouTube Music, Spotify web, YouTube…) en **7.1** y lo
saca por los 8 canales de Windows hacia el receptor. El Integra recibe PCM multicanal, no estéreo.

## Instalar (una vez)

**Lo fácil:** doble clic en **`INSTALAR-EXTENSION.bat`**.
- Copia la extensión a `%LOCALAPPDATA%\PLI\PLI-Espacial-7.1`, para que no dependa de la USB.
- Copia esa ruta al portapapeles y abre Chrome en `chrome://extensions`.
- Ahí solo quedan los clics que Chrome exige por seguridad: **Modo de desarrollador** → **Cargar
  descomprimida** → **Ctrl+V** (pegar la ruta) → **Seleccionar carpeta**.

Para actualizar, vuelve a correr el `.bat` y pulsa «Recargar» en la extensión.

**A mano**, si prefieres:

1. Abre Chrome y escribe en la barra: `chrome://extensions`
2. Arriba a la derecha, activa **Modo de desarrollador**.
3. **Cargar descomprimida** → elige esta carpeta (`chrome-extension`, la que tiene `manifest.json`).
4. Fija el ícono de PLI en la barra (ícono de pieza de rompecabezas → 📌).

## Antes de usarla

- Windows: la salida HDMI del receptor en **7.1 Surround** y como **predeterminada**
  (Panel de sonido `mmsys.cpl`). Chrome saca el audio por la salida predeterminada.
- **Cierra Play Dolby**: usa esa salida en exclusiva y Chrome no podría sonar.
- **Volumen del receptor bajo** al probar.

## Usar

1. Pon la música en una pestaña.
2. Clic en el ícono de PLI → **▶ Activar en esta pestaña**.
3. Elige el modo:
   - **Envolvente**: el estéreo al frente; el ambiente (lo que es distinto entre L y R) a laterales
     y traseras. La voz se queda al frente.
   - **Órbita**: toda la mezcla gira por el domo; el giro tiene tres velocidades o va al revés.
   - **Frente**: el estéreo tal cual, solo FL/FR.
4. **Nivel** y **Mandar graves al subwoofer (LFE)**. El LFE viene apagado: es una decisión de sala.
5. **◎ Probar canales**: un tono suave (−30 dBFS) por bocina, en orden FL, FR, C, LFE, BL, BR, SL, SR.
   Sirve para comprobar que cada bocina está donde debe.

El panel dice «● En vivo · 8 canales». Si dice que la salida tiene 2 canales, Windows no está en 7.1.
Para apagar: el mismo botón (**■ Detener**) o cierra la pestaña.

## Cómo funciona

- `tabCapture` toma el audio de la pestaña. Chrome la silencia mientras se procesa, para que no suene
  doble.
- `motor.js` reparte el sonido con el mismo **paneo VBAP de PLI Espacial**. Usa los ángulos de
  `pruebas-domo/domo.json`: FL −30°, C 0°, FR 30°, SL −100°, SR 100°, BL −145° y BR 145°.
- La salida va en 8 canales discretos, en el orden de Windows 7.1 (FL FR FC LFE BL BR SL SR).
- Pruebas del motor (render fuera de línea, sin bocinas): `pruebas/prueba-motor.html` servida por
  HTTP. Por ejemplo, `python -m http.server 8765 --directory chrome-extension` y abrir
  `http://localhost:8765/pruebas/prueba-motor.html`. Resultado del 2026-10-22: **PASA 16/16**.

## No verificado todavía

- La extensión cargada en Chrome con audio real hacia el receptor. Esa primera prueba es en sala, con
  «Probar canales».
- Si los ángulos del domo cambian al medirlos, hay que actualizar `DOMO` en `motor.js`.
