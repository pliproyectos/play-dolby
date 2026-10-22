# Equipos de la sala — ficha técnica

Fecha: 2026-10-20. Fuente del inventario: fotos de la lista de equipo que pasó Luis.
Marcas: **[F]** dato del fabricante (manual u hoja oficial) · **[T]** dato de terceros (tienda, foro):
tomarlo con reserva · **[S]** verificado en sitio el 2026-10-19 · **[R]** respuesta de sala dada por
Luis el 2026-10-20 · **[?]** pendiente de preguntar o medir.

## Inventario

| Equipo | Qué es | Papel probable en la cadena |
|---|---|---|
| Allen & Heath **Qu-5** | Mezcladora digital, 38 canales, 18 previos | Fuente: manda los 8 canales por USB a la PC |
| **Integra DRX-8.4** | Receptor A/V, 11.4 canales, 7.2.4, Atmos/DTS:X | Decodifica y reparte el 7.1 que llega por HDMI; procesa y saca por PRE OUT |
| NEXO **NXAMP4x1 Mk2** | Amplificador clase D de 4 canales con DSP de NEXO (TDcontroller) | Amplifica y protege las bocinas; hace el cruce y la EQ de cada gabinete |
| NEXO **ePS10-EN54** | Bocina pasiva de 2 vías, 10" + motor de 1,4" | **FL, C y FR**, acostadas con el motor girado [R] |
| NEXO **ePS8-EN54** | Bocina pasiva de 2 vías, 8" + motor de 1,4" | Probablemente envolventes SL/SR/BL/BR [?] |
| NEXO **eLS18** («ELS18 gabinete acústico») | Subwoofer de 18" de alta excursión | LFE y graves |
| NEXO **VNU-HBRK425** | Cuna horizontal para ePS8 (y eLS400) | Montaje: los ePS8 van **acostados** |

Cantidades: **[?]** ¿cuántos ePS10, ePS8, eLS18 y NXAMP hay?

## Datos de cada equipo

### NEXO ePS10-EN54 [F]
- Respuesta 70 Hz–20 kHz (−6 dB); pico de 130 dB SPL; 101 dB SPL a 1 W/1 m [T]; **8 Ω**.
- Cobertura asimétrica rotable: horizontal de 50° a 100°, vertical +25°/−30°.
- Cortes del NXAMP: **60 Hz** (rango completo), **85 Hz** o **120 Hz** (con subwoofer). Combinaciones
  oficiales: ePS10 solo, + eLS600, o **+ eLS18**.
- EN54-24: 250 W continuos y 32 Vrms nominales.
- Con NXAMP4x1mk2, la opción recomendada es **1 ePS10 por canal**. La tabla del manual no indica qué
  columna corresponde a cada amplificador, así que esto es una lectura mía que hay que confirmar.

### NEXO ePS8-EN54 [F]
- Respuesta 80 Hz–20 kHz (−6 dB); pico de 128 dB; **8 Ω**; 8,1 kg.
- Cortes del NXAMP: **70 Hz** (rango completo), **85 Hz** o **120 Hz**. Sus subwoofers oficiales son
  el **eLS400 y el eLS600, no el eLS18.** Si en la sala se usa con eLS18, la alineación de fase del
  preset no es la de fábrica [?].
- EN54-24: 170 W continuos y 29 Vrms nominales.
- Se monta en horizontal con la VNU-HBRK425. **Al acostarlo hay que girar la bocina del motor de agudos**
  (el manual explica cómo); si no se giró, la cobertura queda cruzada [?].

### NEXO eLS18 [F]
- Respuesta 32–150 Hz (−6 dB); pico de 138 dB; 106 dB SPL a 1 W/1 m; **8 Ω**; 41 kg; reflejo de bajos.
- Cortes de 85, 120 o 150 Hz. Con **NXAMP4x1mk2: hasta 2 eLS18 por canal puenteado**, es decir, un
  sub ocupa dos canales del amplificador.

### Qué implica «EN54» [F]
- La EN54-24 es la certificación de **bocina para evacuación por voz** en sistemas de alarma contra
  incendio. Las bocinas son de instalación (IP55, bornes de tornillo, contratuerca).
- [R] **El sistema de sonido no está ligado a la alarma de incendio.** No hay prioridad ni corte
  por GPIO. Que las bocinas tengan certificación EN54-24 **no** convierte la instalación en un sistema
  de evacuación por voz. Ver `NOTA-INSTITUCION-EN54.md`.

### NEXO NXAMP4x1 Mk2 [F]
- 4 canales (puenteables de 2 en 2): 600 W a 8 Ω, 900 W a 4 Ω, 1300 W a 2 Ω. En puente, 1800 W a 8 Ω.
- Entradas: **4 XLR balanceadas analógicas**, 20 kΩ, **sensibilidad de +13 dBu** (salida plena a
  8 Ω), ganancia nominal de 26 dB. Ranura para tarjeta Dante, AES/EBU o EtherSound con 4 entradas
  digitales y respaldo analógico automático.
- DSP: convertidores de 24 bit a 96 kHz, **latencia de 0,58 ms** con preset plano.
- Por canal: ruteo de entradas, volumen (−90 a 0 dB), **retardo de hasta 1 s**, ganancia (±18 dB),
  Array EQ, EQ de usuario de 8 bandas y alineación de tiempo en las entradas.
- **Protección propia**: limita por temperatura de bobina y por excursión del cono. Es la última
  defensa de las bocinas, por debajo de cualquier limitador de Play Dolby.
- 32 escenas, que se pueden llamar por GPIO. Control por Ethernet con **NeMo** (iPad/Mac) o
  AVS-Monitor (Windows). **El protocolo IP no está publicado en el manual**: no prometer que el
  software lo lea.

### Integra DRX-8.4 [F/T]
- 11 canales de 150 W (8 Ω, 2 canales excitados) [F]; procesa 11.4 canales [F].
- **PRE OUT**: RCA en los 11 canales; **XLR solo en FL, C, FR, SW1 y SW2** [T, coinciden dos
  fuentes]. Nivel de salida: aprox. 1 Vrms en RCA y 2 Vrms en XLR [T, foro, sin confirmar].
- 7 entradas HDMI (6 traseras y 1 frontal) y 3 salidas con eARC [F].
- **Dirac Live** incluido (corrección de sala de 20 Hz a 20 kHz) [F].
- Control por IP, IR y RS-232 [T]. eISCP en el puerto 60128: no respondió desde la Wi-Fi «IPN» [S].
- [S] Por EDID anuncia LPCM de 8 canales hasta 192 kHz, además de AC-3, E-AC-3, DTS-HD y TrueHD.

### Allen & Heath Qu-5 [T/S]
- Núcleo FPGA a 96 kHz, 16 previos mic/línea combinados, 12 mezclas [T]. USB-C de **32×32** [T].
- [S] Por ASIO la ve Windows con 32 entradas y 32 salidas, **solo a 96 kHz** (48 kHz rechazado).
  Acepta bloques de 16 a 1024 muestras; Play Dolby usa 64.

## La cadena completa y lo que falta saber

```
Qu-5 ──USB/ASIO 96 kHz──► PC (Play Dolby) ──HDMI LPCM 8 can.──► Integra DRX-8.4
   ──PRE OUT (RCA/XLR) [?]──► NXAMP4x1 Mk2 (×? unidades, 4 canales c/u) ──bocina──► ePS10 / ePS8 / eLS18
```

Un 7.1 con un eLS18 en puente necesita al menos **7 canales para las bocinas + 2 para el sub = 9
canales**, o sea **3 NXAMP4x1 Mk2** como mínimo [cuenta mía, sin confirmar].

### Respuestas de sala — 2026-10-20 [R] (Luis)

| # | Tema | Respuesta | Qué falta |
|---|---|---|---|
| 1 | Posiciones | **ePS10 en FL, C y FR**, acostadas, con el motor de agudos girado correctamente | ¿Las ePS8 van en SL, SR, BL y BR? ¿Cuántos eLS18 hay? |
| 2 | Entradas del NXAMP | **XLR** | El Integra solo tiene XLR en FL, C, FR y SW [T]: ¿cómo salen los cuatro envolventes? ¿Cuántos NXAMP hay y con qué preset? |
| 3 | Cruce de graves | **Lo hace el NXAMP** | El preset de cada canal (60/70, 85 o 120 Hz) |
| 4 | Retardos | **En el NXAMP** | Tabla de distancias o retardos cargados |
| 5 | Dirac / modo | No se sabe | Revisarlo con la lista de abajo |
| 6 | Alarma de incendio | **No está ligada** | Ver `NOTA-INSTITUCION-EN54.md` |

**Cómo debe quedar el Integra con estas respuestas** (propuesta; la decisión es de Luis):
- Todas las bocinas en **Large** (o «Full Band»). Sin cruce en el Integra. Si hubiera dos cruces,
  quedaría un hueco en los graves.
- **Distancias iguales** en las 7 bocinas y en el sub. Así el Integra no suma retardo, porque los
  retardos ya los pone el NXAMP.
- Niveles de canal en 0 dB. Los niveles ya los pone el NXAMP.
- «Double Bass» o «LFE + Main» apagados, si existen en el menú: esas funciones mandan graves de los
  frentes al sub, y eso equivale a un segundo cruce.
- Modo de escucha **Direct** con PCM de 8 canales: no aplica ningún upmix ni tono. Dirac apagado,
  o en Direct, que lo omite [dato del manual de Dirac para Denon/Marantz; sin confirmar en el
  DRX-8.4].

**Consecuencia para los graves (importante):** con el cruce en el NXAMP y las bocinas en Large,
**nadie manda al eLS18 los graves de los 7 canales**. Al sub solo llega el canal LFE. Por debajo del
corte del preset (85/120 Hz, o 60/70 Hz con el preset de rango completo), los graves de los canales
principales se pierden.
- Con material 7.1 que ya trae LFE: se oye bien.
- Con estéreo convertido a 7.1 (upmix, `capas71`): el sub solo suena si el programa **genera LFE**.
  La regla «no generar LFE» venía de no saber quién cruzaba. Ahora generarlo es una decisión que
  hay que probar en sala: un filtro pasa-bajos a la frecuencia del preset, con nivel ajustable.
  Esa decisión es de Luis.

### Cómo revisar el Integra (pregunta 5, paso a paso)

Con el volumen bajo y la PC mandando audio por HDMI:
1. **Formato de entrada:** botón de información del control remoto o pantalla frontal. Debe decir
   «PCM» o «Multich PCM» con 7.1 o 8 canales. Si dice 2.0, Windows no está en 7.1.
2. **Modo de escucha:** la pantalla frontal lo muestra al cambiar de fuente. Anotar el nombre exacto
   (Direct, Pure Audio, Multichannel, Dolby Surround, DTS Neural:X…). Para el domo conviene
   **Direct**. Ojo: **Pure Audio** apaga además el video del receptor. Si el video de Nightshade
   pasa por el Integra, no usarlo.
3. **Dirac Live:** en el menú rápido del control (botón **Q** en los Onkyo/Integra de esta familia)
   o en Setup → Speaker. Ver si aparece Dirac Live, si dice On u Off y qué filtro tiene cargado.
   **Si nadie corrió la calibración con la app de Dirac y su micrófono, no está calibrado.** Si está
   en On, anotar el nombre del filtro y la fecha. Si alguien cambió la distribución de bocinas
   después de calibrar, la calibración se borró sola.
4. **Bocinas:** Setup → Speaker → Configuration / Crossover. Anotar si cada canal dice Large o Small,
   la frecuencia de cruce, las distancias y los niveles. Tomar **foto de cada pantalla**.
5. **Red:** Setup → Network. Anotar la IP, para el control eISCP desde la PC.

Las rutas de menú son las de la familia Onkyo/Integra y no están verificadas en este aparato. Si no
coinciden, buscar en el menú de bocinas las palabras «Crossover», «Distance» y «Dirac».

### Preguntas para Luis (deciden criterios de ingeniería y de sala)
1. Cantidades, y qué bocina va en cada posición del 7.1: ¿ePS10 al frente y ePS8 en laterales y
   traseras?
2. ¿Cuántos NXAMP hay, qué preset tiene cargado cada canal y qué salidas del Integra los alimentan
   (RCA o XLR)?
3. **Manejo de graves:** ¿quién hace el cruce, el Integra (bocinas en «Small») o el NXAMP (preset
   de 85/120 Hz)? Hacerlo en los dos encadena dos filtros y deja un hueco. Es una decisión de sala.
4. **Retardos por distancia:** ¿en el Integra o en el NXAMP? En uno solo.
5. ¿Dirac Live está calibrado? Con modo Direct o Pure se desactiva, y además cambia la latencia.
6. ¿Hay prioridad de la alarma de incendio sobre el sonido?

## Consecuencias para el software

- **Nivel y protección:** el NXAMP protege por temperatura y excursión, así que Play Dolby no
  tiene que hacer de limitador de bocinas. Aun así se conservan la ganancia inicial de −18 dB y el
  recorte que cuenta saturaciones.
- **Graves y LFE:** los ePS8 cortan en 80 Hz y los ePS10 en 70 Hz. El LFE de las pruebas y del
  upmix (por debajo de 80 Hz) solo tiene sentido si llega al eLS18. **No generar LFE por cuenta
  propia sin saber quién hace el cruce** (ya lo decía el SPEC §3).
- **Señales de prueba:** el ruido rosa para las bocinas principales conviene filtrarlo por encima
  de 80–100 Hz (en el ePS8 no hay nada útil por debajo de 80 Hz). El tono de 50 Hz es para el
  eLS18.
- **Presupuesto de latencia:** Qu (64 muestras) + Play Dolby (~20 ms medidos) + Integra (depende
  del modo; Dirac añade) + NXAMP (0,58 ms). Hay que medirlo en acústico.
- **Mapa de canales:** el orden del Integra (PRE OUT) → NXAMP → bocina se fija con la ficha
  `pruebas-domo/domo.json` y se verifica con los programas de identificación.
- **Monitoreo futuro:** el Integra se controla por IP y el NXAMP tiene protocolo propio sin
  publicar. Integrar cualquiera de los dos requiere estar en la misma red y documentarlo antes.

## Fuentes
- ePS8-EN54: https://www.nexo-sa.com/products/eps8-en54/ · manual https://www.nexo-sa.com/wp-content/uploads/ePS8-EN54-User-Manual_Rev01-EN.pdf
- ePS10-EN54: https://www.nexo-sa.com/products/eps10-en54 · manual https://www.nexo-sa.com/wp-content/uploads/ePS10-EN54-User-Manual_Rev01-EN.pdf
- eLS18: https://www.nexo-sa.com/products/els18/
- NXAMP4x1 Mk2: https://www.nexo-sa.com/products/nxamp4x1mk2 · manual https://www.nexo-sa.com/wp-content/uploads/NXAMPmk2-User-Manual-Rev01a-EN.pdf
- Integra DRX-8.4: https://integrahometheater.com/products/drx-8-4-11-4-channel-network-av-receiver · reseña con detalle de PRE OUT https://www.worldwidestereo.com/blogs/guides/integra-drx84av-receiver-review · nivel de PRE OUT (foro) https://forums.audioholics.com/forums/goto/post?id=1601360
- Qu-5: https://www.markertek.com/product/ah-qu-5/allen-heath-qu-5-32-channel-compact-digital-mixer-96khz-fpga-core-with-16-1-onboard-preamps-and-12-line-outputs · https://www.scan.co.uk/products/allen-and-heath-qu-5-digital-mixing-console-with-16-faders
