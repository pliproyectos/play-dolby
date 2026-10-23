# Qu-5 → Play Dolby: 8 entradas estéreo por USB

Fecha: 2026-10-22 · Menús según la guía oficial **Qu-5/6/7 User Guide** de Allen & Heath (Mixer Setup,
Patching, Routing, Qu Drive and USB audio, Storing and Recalling Settings).

## La idea

```
8 fuentes estéreo ──► Qu-5: Local 1-16 ──► Canales 1-16 en 8 pares estéreo
                      ──► Direct Out ──► USB-C salidas 1-16 ──► PC: Play Dolby (USB 1-2 … 15-16)
                      ──► PLI Espacial: 8 objetos estéreo ──► HDMI 7.1 ──► domo
```

| Fuente | Entradas Qu-5 (Local) | Canales Qu-5 | USB a la PC | Play Dolby |
|---|---|---|---|---|
| 1 | 1 (L) · 2 (R) | 1-2 estéreo | 1-2 | Objeto 1 · «USB 1-2 estéreo» |
| 2 | 3 · 4 | 3-4 estéreo | 3-4 | Objeto 2 · «USB 3-4 estéreo» |
| 3 | 5 · 6 | 5-6 estéreo | 5-6 | Objeto 3 · «USB 5-6 estéreo» |
| 4 | 7 · 8 | 7-8 estéreo | 7-8 | Objeto 4 · «USB 7-8 estéreo» |
| 5 | 9 · 10 | 9-10 estéreo | 9-10 | Objeto 5 · «USB 9-10 estéreo» |
| 6 | 11 · 12 | 11-12 estéreo | 11-12 | Objeto 6 · «USB 11-12 estéreo» |
| 7 | 13 · 14 | 13-14 estéreo | 13-14 | Objeto 7 · «USB 13-14 estéreo» |
| 8 | 15 · 16 | 15-16 estéreo | 15-16 | Objeto 8 · «USB 15-16 estéreo» |

Anota en la tabla qué fuente es cada una (por ejemplo «1 = Nightshade», «2 = música»). El orden lo
decides tú; lo importante es que **L vaya al número impar y R al par**.

## Por qué no se puede «cargar la configuración desde la PC»

La configuración de la Qu-5 se guarda en un **Show**, que es un formato propio de Allen & Heath **sin
documentación pública**. Un Show fabricado fuera de la consola podría ser rechazado o, peor, cargar
algo mal hecho. Lo seguro es configurarla **una vez** con esta guía y guardarla en un Show **desde la
consola**. Después solo se carga ese Show.

## Paso a paso en la consola (una sola vez, ≈ 10 min)

**Antes:** volumen del Integra bajo y Play Dolby detenido.

1. **USB en multicanal y a 96 kHz**
   `SETUP › Audio › Sync&USB`
   - **USB Stream Mode = Multitrack.** En «Stereo» solo pasan los canales USB 1-2.
   - **Sample rate = 96 kHz.** Es lo que se verificó con Play Dolby el 2026-10-19; la consola
     procesa siempre a 96 kHz.

2. **Canales en pares estéreo**
   `SETUP › Config › Input Stereo`
   - Toca **Mono/Stereo** en los pares 1-2, 3-4, 5-6, 7-8, 9-10, 11-12, 13-14 y 15-16 → **Apply**.
   - Por omisión, cargar una *escena* no cambia el modo mono/estéreo (lo bloquean los Global
     Filters). Por eso esto queda guardado en el Show (paso 6).

3. **Cada canal toma su entrada física**
   `I/O › Inputs`
   - Canal 1 ← Local 1, canal 2 ← Local 2 … canal 16 ← Local 16. Normalmente ya viene así. Si no,
     usa **Quick Patch** (menú ☰ arriba a la derecha) o **+1**.
   - Ajusta la ganancia (preamp) de cada par con la fuente sonando.

4. **Salidas USB = Direct Out de cada canal**
   `I/O › Outputs › USB`
   - USB 1 ← **Input Direct Out** canal 1, USB 2 ← canal 2 … USB 16 ← canal 16. Patcha el primero y
     pulsa **+1** quince veces.
   - Ojo: **por omisión, USB 1-2 lleva la mezcla Main LR**. Hay que cambiarlo a Direct Out 1-2.

5. **Cómo se comportan los Direct Out** (ajuste global; se cambia desde la pestaña Direct Out de
   cualquier canal)
   `ROUTING › Direct Out` (con un canal de entrada seleccionado) › **Global Direct Out Settings**
   - **Source:** el punto de la cadena de donde sale (post-EQ/compresor, por ejemplo).
   - **Follow Fader** encendido: los faders de la consola controlan lo que llega al domo.
     Recomendado, para mandar desde la consola.
   - **Follow Mute** encendido: el MUTE del canal también calla lo que va al domo. Recomendado.
   - **Level** de cada canal (en la misma pestaña): déjalo en 0 dB.

   La decisión de Follow Fader y Follow Mute es tuya. Con las dos encendidas, la consola manda.

6. **Guardar el Show en una USB**
   - Usa **otra USB, no la KINGSTON**: la consola la tiene que formatear (FAT32 con su carpeta
     `AH_QU`).
   - Formato (una sola vez): `UTILITY › USB Utility › Status/Format`.
   - Guardar: `UTILITIES › Data › Shows` › **Store New** › nombre **PLAY DOLBY 8 ST**.
   - Guardar o cargar un Show **corta un momento el audio USB-C**. Hazlo antes de la función.

## En Play Dolby

1. **01 Conecta tu consola:** la Qu-5 (ASIO).
2. **✦ PLI Espacial** → botón **«8 estéreo de la consola»**. Pone los 8 objetos en USB 1-2 … 15-16
   (estéreo), repartidos alrededor del domo, deja las salidas directas en «Silencio» y enciende
   Dynamic Space.
3. **02:** el receptor. **▶ Iniciar puente.** Mueve los objetos o usa las escenas rápidas.

PLI Espacial tiene **8 objetos**: cada fuente estéreo es uno. El sub (LFE) no recibe nada de los
objetos. Si quieres graves al sub, asigna un canal en la tarjeta LFE del paso 03.

## La próxima vez

Conecta la USB de la consola → `UTILITIES › Data › Shows` → **PLAY DOLBY 8 ST** → **Recall**. Queda
todo como lo dejaste.

## Comprobar que llega a la PC

En Play Dolby, con el puente en vivo, los medidores de las tarjetas (o del panel PLI Espacial) deben
moverse al sonar cada fuente. Si un par no llega, revisa en este orden: el patch USB del paso 4, que
el canal no esté en MUTE (con Follow Mute) y que el fader no esté abajo (con Follow Fader).
