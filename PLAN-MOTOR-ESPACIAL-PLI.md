# Plan: motor espacial propio de PLI

Fecha: 2026-10-20 · Proyecto PARCHE DOLBY · Estado: **fase 1 implementada y probada en escritorio,
sin escucharse en sala.**

**Avance de la fase 1 (2026-10-20):**
- Motor: `native/espacial/pli_espacial.hpp`, con VBAP 2D, lectura de `domo.json` y rechazo de
  fichas inválidas (huecos de 180° o más, bocinas repetidas).
- Pruebas: `native/tests/espacial_tests.cpp`, 65 386 comprobaciones y 0 fallas. Cubren la bocina
  exacta, la potencia constante, la continuidad, que el LFE quede siempre en cero y la simetría
  izquierda/derecha.
- `capas71 ... --domo pruebas-domo\domo.json` usa el motor. Sin `--domo`, el resultado es idéntico
  bit por bit al ejecutable anterior (SHA-256 comparado).
- Render de prueba: `musica-7.1/pli-espacial/`.
- Falta: escucharlo en el domo y poner en la ficha los ángulos **medidos** (hoy son nominales).

**Avance de la fase 2 (2026-10-20):**
- Escenas JSON (`native/espacial/escena.hpp`, formato en `native/espacial/ESCENAS.md`):
  - objetos mono o estéreo, con nivel, ancho y momento de entrada;
  - movimientos por puntos, giro y vaivén, con una curva de velocidad;
  - el LFE solo se genera si la escena lo pide.
- Programa `escena71.exe`: renderiza 175 s de audio en unos 6 s.
- `capas71` escribe `escena.json` con su coreografía.
- Aceptación: renderizar esa escena reproduce la salida de `capas71 --domo` con una diferencia
  máxima de **−84,5 dBFS** (el LFE, idéntico: 1 LSB). `capas71` sin ficha sigue idéntico bit por bit
  al ejecutable anterior.
- Pruebas: 65 408 comprobaciones, 0 fallas.
- 2026-10-20: se escuchó la versión PLI Espacial por VLC en esta laptop, conectada a un receptor
  por HDMI en 7.1.

**Avance de la fase 3 (2026-10-20): implementada; probada en escritorio sin audio real.**
- Motor en vivo: `native/espacial/vivo.hpp`.
  - Hasta 8 objetos. Cada uno toma un canal USB y puede estar fijo, girando o meciéndose, con su nivel.
  - Corre dentro del callback de captura: no reserva memoria, no usa candados y no agrega latencia.
  - Ganancias en rampa por bloque. Al cambiar de entrada, la anterior baja a 0 antes de que entre la nueva.
  - Se suma a las rutas directas.
- Panel `native/src/espacial_panel.hpp`, que se abre con el botón «✦ PLI Espacial» de Play Dolby.
  - Domo visto desde arriba con las bocinas de `domo.json` y objetos que se arrastran con el ratón.
  - Por objeto: entrada, movimiento y nivel.
  - Botones: encender/apagar (también con el puente en vivo), recargar la ficha (solo con el puente
    detenido) y guardar/cargar en `%LOCALAPPDATA%\PlanetarioBridge\espacial.txt`.
- Pruebas:
  - 65 425 comprobaciones, 0 fallas: rampa, cambio de entrada, giro, vaivén, LFE en cero, entradas
    inexistentes, NaN y la suma con el núcleo del puente.
  - `core_tests` sigue en PASA: con «Espacial» apagado, el puente es el mismo de antes.
- Interfaz operada por mensajes de Windows y verificada con capturas:
  - el panel se dibuja;
  - los selectores aplican;
  - el arrastre mueve el objeto;
  - el botón enciende.
- Respaldo del ejecutable anterior: `native/bin/PlayDolby-anterior.exe`.
- **No verificado:** audio real con la Qu-5 por el puente y la prueba de 30 min sin faltas.

- Interfaz: se decidió integrar todo en **Play Dolby**. La fase 3 agrega un panel «Espacial».
  No se hará un programa aparte: WASAPI exclusivo permite un solo programa en la salida HDMI.

## La pregunta

¿Podemos tener algo como «Dolby Atmos» propio, con nuestros controladores, en vez de pagar la app
Dolby Access?

## Respuesta corta

1. **Para sacar 7.1 no hace falta Atmos.** En Windows se elige «7.1 Surround» y se manda PCM de 8
   canales al Integra. Es gratis y ya se verificó en el domo el 2026-10-19. La opción «Dolby Atmos
   para el centro de entretenimiento» solo sirve para mandar el formato codificado de Dolby al
   receptor. Con 7.1 sin bocinas de altura no aporta nada que no hagamos ya.
2. **No se puede hacer un «Atmos» propio con ese nombre ni con su formato.** Atmos es una marca y un
   códec de Dolby, y el Integra solo lo decodifica si viene de un codificador licenciado. Lo que sí
   es libre es **la idea**: audio por objetos (cada sonido con su posición) que se convierte a las
   bocinas reales. Para eso hay estándares abiertos y librerías libres.
3. **No hace falta un controlador (driver) de Windows.** Un driver de kernel necesita firma de
   Microsoft y, si falla, tumba la PC. Los «Spatial sound» de la lista (Dolby, DTS) son socios de
   Microsoft. Play Dolby ya es nuestro motor en modo usuario: el motor espacial va **dentro de Play
   Dolby**.

**Nombre:** no usar «Atmos» ni «Dolby» en un producto propio, porque son marcas registradas. Por
ejemplo: **«PLI Espacial»** o «PLI Domo 3D». Si Play Dolby sale del uso interno, también conviene
renombrarlo.

## Lo que ya existe y es libre

| Opción | Qué es | Licencia | Para nosotros |
|---|---|---|---|
| **libear** (BBC + IRT) | Renderer de objetos según **ITU-R BS.2127** (el estándar abierto de audio por objetos, formato ADM) | **Apache 2.0**: se puede usar y cerrar sin problema | Núcleo de ganancias por objeto. C++14, igual que Play Dolby |
| **libspatialaudio** (Videolabs/VLC) | Ambisonics hasta 3.er orden, render ADM (BS.2127) e **IAMF** | LGPL 2.1+: se puede usar ligándola como DLL | Si llega contenido de domo en Ambisonics o IAMF |
| **IAMF / Eclipsa Audio** (Alliance for Open Media, Google, Samsung) | Formato inmersivo **libre de regalías**, alternativa abierta a Atmos | Libre de regalías; referencia `libiamf` | Formato de intercambio a futuro, no de reproducción en sala |
| Nuestro `panRing` (`native/upmix/audio71.hpp`) | Paneo de potencia constante entre bocinas vecinas (VBAP 2D) | Propio | **Ya funciona** en `capas71`. Se escucharon sus resultados en el domo |

Conclusión: **no hay que inventar la librería desde cero**. El plan es crecer lo que ya tenemos y
apoyarnos en libear cuando haga falta el estándar.

## Arquitectura propuesta

```
Fuentes (estéreo, capas, voz del narrador, efectos)
   └─► Escena: objetos con posición en el tiempo (archivo JSON de coreografía)
          └─► Renderer PLI: ganancias por bocina según domo.json (ángulos y distancias medidas)
                 └─► 8 canales (o más, si se agregan bocinas) ─► WASAPI exclusivo / ASIO
                        ├─► HDMI → Integra (ruta actual)
                        └─► tarjeta externa de 8 salidas → Qu-5 (ruta nueva)
```

Regla central: **la escena no sabe cuántas bocinas hay**. Las posiciones se dan en grados, y
`domo.json` dice dónde está cada bocina. Si mañana se agregan bocinas, solo cambia la ficha.

## Fases

| Fase | Qué se hace | Criterio de aceptación |
|---|---|---|
| **0. Hoy** | Windows en 7.1 PCM. Sin Atmos y sin pagar | Diagnóstico: 8 canales, máscara 0x63F. `prueba-7.1.wav` suena en cada bocina |
| **1. Paneo desde la ficha** | `panRing` deja de usar ángulos fijos y lee `domo.json` (VBAP 2D con los ángulos reales). Pruebas unitarias | Un objeto en el ángulo de una bocina sale solo por esa bocina. La potencia total es constante al girar |
| **2. Formato de escena** | JSON de objetos: archivo de audio, trayectoria (puntos de tiempo y ángulo), ancho y nivel. `capas71` pasa a ser «escena + renderer» | Regenerar las versiones de `musica-7.1/` desde escenas y escucharlas igual o mejor en sala |
| **3. Tiempo real** | El renderer corre dentro de Play Dolby: objetos movidos en vivo desde la interfaz (después, por MIDI u OSC desde la Qu-5 o Nightshade) | Sin faltas en 30 min. La latencia añadida se mide y se reporta |
| **3b. Dynamic Space en la ventana principal** | Llevar lo espacial a la pantalla principal de Play Dolby, no solo al panel. Ver detalle abajo | Se opera una función completa desde la ventana principal sin abrir el panel. A 30 cuadros por segundo no sube la carga ni hay faltas en el puente |
| **4. Estándar abierto** | Importar y exportar **ADM/BW64** con libear (BS.2127) e IAMF con libspatialaudio | Un archivo ADM de prueba de la EBU se oye en el domo con las posiciones correctas |
| **5. Altura (opcional)** | Si se instalan bocinas arriba del domo: VBAP 3D o libear con elevación | Depende de que haya bocinas de altura |

### Fuente archivo y objetos estéreo (2026-10-20)

- **Por qué:** Luis probó Play Dolby sin la Qu-5 y no sonó nada. El puente se negó a arrancar porque
  la única entrada era el micrófono de la laptop (2 canales) y el ruteo pedía 8. Error en
  `bridge.log`: «El parche solicita entradas que el dispositivo no tiene».
- **Fuente archivo:** un WAV o una carpeta de capas de capas71 hace de consola.
  - Todo se carga a memoria; el callback de salida lo copia al núcleo.
  - Sin servo de deriva (`Core::servo = false`): copia exacta, sin remuestrear.
  - Se repite al terminar.
  - Con una carpeta de capas: 6 objetos estéreo con la coreografía de capas71, las rutas directas en
    Silencio y Espacial encendido.
- **Objetos estéreo:** L = entrada y R = entrada + 1, a azimut ∓ ancho. Las rampas son
  independientes, y al pasar de mono a estéreo primero se baja a cero.
- **Mensaje de error claro:** dice cuántos canales tiene la entrada y qué salida u objeto pide más.
- Pruebas: 65 452 comprobaciones, 0 fallas.
  - El reproductor da el archivo exacto en 40 bloques de 960, con y sin repetición, sin faltas.
  - Objetos estéreo, y lectura de WAV y de carpetas de capas.
- **No verificado:** sonido real con el puente arrancado desde un archivo. Luis lo prueba en sala.

### Fase 4: estado (2026-10-20)

**ADM: hecho con código propio, sin descargar librerías** (regla de Luis: nada se instala sin preguntar).
- `native/espacial/adm.hpp` y `adm71.exe`, con dos órdenes: `exporta` e `importa`.
- Formato: BW64/WAV con `chna` + `axml` (ITU-R BS.2076-2). Se lee RIFF, RF64 y BW64.
- Importa pistas «Objects» y «DirectSpeakers», con coordenadas polares o cartesianas.
- Pruebas (65 439 comprobaciones, 0 fallas):
  - ida y vuelta escena → ADM → escena con fijos, estéreo, giro, vaivén y LFE: menos de −40 dB de
    diferencia;
  - lector XML (espacios de nombres, entidades, comentarios);
  - tiempos por segundos y por muestras;
  - signo del azimut;
  - archivo completo en disco a 24 bit;
  - definiciones comunes de 5.1.
- Con la canción real (Cosmic «estable»): 12 pistas y 3,6 MB de XML. Al reimportar, el 7.1 difiere
  como máximo −90,7 dBFS del render directo, y el LFE sale idéntico. Archivos en `musica-7.1/adm/`.
- **Validado con el renderizador de referencia de la EBU (2026-10-20, descarga autorizada por Luis):**
  - EAR (de github.com/ebu/ebu_adm_renderer) va en `native/tools/ear-venv`, un entorno aislado con
    numpy 2. La versión 2.1.0 de PyPI no corre en Python 3.14 (usa `pkg_resources`).
  - EAR renderizó nuestro ADM de Cosmic con los ángulos del domo (`native/espacial/ear-domo.yaml`).
    Contra nuestro render:
    - C y LFE salen idénticos;
    - FL/FR: −0,08 dB, correlación 0,9988;
    - envolventes: ±0,8 dB, correlación 0,976–0,980. La diferencia es la ley de paneo entre bocinas
      vecinas.
  - `ear-utils make_test_bwf` generó un ADM con movimiento, salto, elevación, DirectSpeakers y LFE, y
    `adm71 importa` lo leyó bien. Esa prueba destapó un error en los bloques no contiguos, ya
    corregido y con su prueba unitaria.
  - Aviso de EAR: el renderizado de la versión BS.2076-2 «no está estandarizado». No es una falla.
- **IAMF: hecho, solo con PCM (2026-10-20, descarga autorizada por Luis):**
  - Fuentes: libiamf v2.0.0 y `oar` en `native/vendor/iamf`. Solo la carpeta `code`; se borraron
    41 MB de WAV de prueba del binaural que no se usan.
  - Compilación: con Zig y sin CMake (`native/build-iamf.ps1`). Produce `iamfdec.exe` e `iamf71.exe`.
  - Los flujos oficiales `test_000206` (7.1) y `test_000038` (Ambisonics de primer orden) se decodifican
    **bit por bit iguales** a la referencia de AOMedia, o a 1 LSB.
  - **Orden de canales:** libiamf entrega el 0+7+0 en orden ITU (laterales antes que traseros);
    `iamf71` reordena a Windows. Verificado canal por canal.
  - Sin Opus, AAC ni FLAC: esos IAMF fallan con un mensaje claro. Habilitarlos requiere tres descargas
    más.
  - El render IAMF usa posiciones ITU (±90°, ±135°), no las del domo.
- Definiciones comunes: solo se reconocen las de 5.1 (`AC_00010001` a `AC_00010006`). El resto se avisa
  y se omite, para no adivinar posiciones.

### Fase 3b: Dynamic Space en la ventana principal (pedida por Luis el 2026-10-20)

**Estado (2026-10-20): implementada y verificada con capturas en esta laptop, sin audio real.**
- Lo que quedó:
  - insignia «DYNAMIC SPACE · N», que se enciende y apaga con un clic;
  - domo en vivo arriba a la derecha, atenuado y con un punto que recorre el aro cuando está apagado;
  - barras FL…SR en las tarjetas;
  - escenas rápidas: Quieto, Órbita, Respira y La de 1:45;
  - velocidad maestra de 0× a 1,5×.
- **Vista previa:** con el puente detenido, la interfaz mueve los puntos con las mismas fórmulas del
  motor, para ensayar sin consola.
- Rendimiento: el camino rápido de la vista se amplió a la zona dinámica, así que la animación no
  redibuja la ventana completa.
- Cambio respecto al plan: con Dynamic Space apagado, en lugar de la órbita decorativa se ve el domo
  atenuado. La órbita no cabía junto a la columna de escenas.

Plan original:

Hoy la ventana principal tiene una órbita decorativa arriba a la derecha y lo espacial vive en un
panel aparte. Esta fase lo vuelve parte de la pantalla principal:

1. **Domo en vivo en lugar de la órbita decorativa:** la misma vista desde arriba, en pequeño, con
   las bocinas de la ficha y los objetos moviéndose de verdad. Si «Espacial» está apagado, conserva
   la animación actual.
2. **Insignia «DYNAMIC SPACE»** junto a «EN VIVO / DETENIDO»: muestra si está encendido y cuántos
   objetos están activos. Un clic la enciende o la apaga.
3. **Tarjetas de salida vivas:** cada tarjeta del paso 03 (FL…SR) se ilumina según cuánto le manda el
   paneo en ese momento. Así se ve el objeto pasar de bocina en bocina.
4. **Escenas rápidas:** botones con coreografías listas para todos los objetos a la vez:
   - «Quieto»: todos fijos;
   - «Órbita»: giros escalonados;
   - «Respira»: vaivenes;
   - «La de 1:45»: la coreografía del tramo que le gustó a Luis, a velocidad normal.
5. **Velocidad maestra:** un control que multiplica todas las rpm, de 0 a 1,5×, para acelerar o
   frenar en vivo sin tocar objeto por objeto.

Reglas: nada de eso arranca audio. La animación lee las posiciones que ya publica el motor (no
agrega trabajo al audio). El panel sigue existiendo para el detalle por objeto.

Sobre la altura: hoy el sistema es 7.1 horizontal. **Ningún software, ni el propio Atmos, puede
poner un sonido arriba si no hay bocinas arriba**. En un domo, la diferencia grande la haría agregar
bocinas en la cúpula, y esa es una decisión de sala.

## Riesgos y reglas

- **Graves:** el renderer no manda nada al LFE salvo que Luis lo decida. El cruce lo hace el NXAMP,
  ver `EQUIPOS-SALA.md`.
- **Licencias:** libear es Apache 2.0 y basta con incluir su aviso. libspatialaudio es LGPL: se usa
  como DLL aparte y no se modifica sin publicar los cambios. Confirmar la licencia de cada archivo
  antes de integrarlo.
- **Nada se instala sin preguntar.** Las librerías se integran como código dentro de `native/vendor`
  y se compilan con el Zig portátil del proyecto.

## Fuentes

- libear (Apache 2.0, BS.2127): https://github.com/ebu/libear · https://libear.readthedocs.io/
- libspatialaudio 0.4 (Ambisonics, ADM, IAMF): https://jbkempf.com/blog/2025/libspatialaudio-0.4 · https://github.com/videolabs/libspatialaudio
- Eclipsa Audio / IAMF: https://opensource.googleblog.com/2025/01/introducing-eclipsa-audio-immersive-audio-for-everyone.html · https://www.phoronix.com/news/AOMedia-IAMF-Audio-Spec
