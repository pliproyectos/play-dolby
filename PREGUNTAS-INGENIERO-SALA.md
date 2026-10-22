# Preguntas para el ingeniero de la sala

Proyecto PARCHE DOLBY · 2026-10-20 · Preparó: Luis Martínez.
Para la plática con el ingeniero responsable del sonido del planetario. Las respuestas definen cómo
se afina **Play Dolby**, el programa que toma los 8 canales de la consola y los manda al domo.
El detalle técnico de cada equipo está en `EQUIPOS-SALA.md`.

## Cadena tal como la entendemos (confirmar)

```
Consola Qu-5 ──USB 96 kHz──► PC con Play Dolby ──HDMI, PCM 8 canales──► Integra DRX-8.4
   ──PRE OUT──► NXAMP4x1 Mk2 (¿cuántos?) ──► ePS10-EN54 / ePS8-EN54 / eLS18
```

Lo ya comprobado en sitio el 2026-10-19: el Integra recibe y anuncia **PCM de 8 canales** por HDMI;
el puente consola → PC → HDMI corre sin errores con unos **20 ms** de latencia propia; se
escucharon pruebas 7.1 por VLC.

---

## 1. Inventario y posiciones

**Pregunta.** ¿Cuántas ePS10, ePS8 y eLS18 hay, y qué bocina ocupa cada posición del 7.1?

**Por qué importa.** Las ePS10 bajan a 70 Hz y las ePS8 solo a 80 Hz. Además, la ePS8 no tiene
preset oficial con el eLS18: sus subwoofers de fábrica son el eLS400 y el eLS600. Las pruebas de
identificación de canales y las curvas del programa dependen de qué bocina hay en cada lugar.

| Posición | Bocina (modelo) | Cantidad | Altura / orientación |
|---|---|---|---|
| Frontal izquierda (FL) | | | |
| Centro (C) | | | |
| Frontal derecha (FR) | | | |
| Lateral izquierda (SL) | | | |
| Lateral derecha (SR) | | | |
| Trasera izquierda (BL) | | | |
| Trasera derecha (BR) | | | |
| Subwoofer (LFE) | | | |

Extra: las ePS8 van en cuna horizontal (VNU-HBRK425). **¿Se giró la bocina de agudos** al acostarlas,
como indica el manual?  ☐ Sí  ☐ No  ☐ No se sabe

---

## 2. Amplificación: cuántos NXAMP, qué preset y qué entrada

**Pregunta.** ¿Cuántos NXAMP4x1 Mk2 hay? En cada canal, ¿qué preset NEXO está cargado y qué salida
del Integra lo alimenta (RCA o XLR)?

**Por qué importa.** Cada NXAMP tiene 4 canales. Un eLS18 ocupa dos canales en puente. Un 7.1
completo necesita unos 9 canales, o sea, 3 amplificadores como mínimo. El Integra solo tiene XLR
balanceado en FL, C, FR y los dos subwoofers; el resto sale por RCA, y en tiradas largas eso
importa. El preset define el corte de graves de cada bocina (60/70, 85 o 120 Hz).

| NXAMP # | Canal | Bocina conectada | Preset cargado | Entrada (salida del Integra, RCA/XLR) |
|---|---|---|---|---|
| | 1 | | | |
| | 2 | | | |
| | 3 | | | |
| | 4 | | | |

¿Hay una sesión de **NeMo** guardada con esta configuración?  ☐ Sí → pedir copia  ☐ No

---

## 3. Manejo de graves: ¿quién hace el cruce? (la más importante)

**Pregunta.** ¿El corte entre bocinas y subwoofer lo hace el **Integra** o el **NXAMP**?

**Por qué importa.** Si los dos lo hacen, quedan dos filtros en serie: un hueco en los graves y la
fase desalineada. Hay que elegir uno.

| Opción | Cómo queda | Consecuencia |
|---|---|---|
| A. Lo hace el Integra | Bocinas en «Small» con corte en __ Hz; el Integra manda los graves de los 7 canales al SUB OUT | El NXAMP debe usar el preset de rango completo (60/70 Hz) en las bocinas |
| B. Lo hace el NXAMP | Bocinas en «Large» en el Integra; presets 85/120 Hz en el NXAMP | Los graves de cada canal no llegan al sub; el LFE va solo al sub |
| C. Otra | | |

**Decisión:** ________ · Frecuencia de corte: ______ Hz

Para el programa: **no se va a generar LFE por cuenta propia** hasta tener esta respuesta.

---

## 4. Retardos por distancia

**Pregunta.** ¿Dónde se compensan las distancias de cada bocina al centro del domo: en el **Integra**
(distancias de bocinas) o en el **NXAMP** (retardo por canal, hasta 1 s)?

**Por qué importa.** Si se compensan en los dos lados, el retardo se suma y la imagen se corre.
Solo uno debe hacerlo.

**Respuesta:** ☐ Integra  ☐ NXAMP  ☐ Ninguno  ·  ¿Hay medición de distancias? ☐ Sí → pedir tabla

---

## 5. Dirac Live y modo de escucha

**Pregunta.** ¿El Integra tiene **Dirac Live** calibrado para la sala? ¿Qué modo de escucha se usa en
las funciones (Direct, Pure, Multichannel, Dolby Surround…)?

**Por qué importa.** Dirac corrige la respuesta de la sala, pero añade latencia, y con Direct o
Pure se desactiva. Para que el audio vaya sincronizado con el video hay que saber qué modo queda
fijo, y medir la latencia completa en ese modo.

**Respuesta:** Dirac: ☐ calibrado ☐ no · Modo habitual: __________ · Fecha de la calibración: ______

---

## 6. Alarma de incendio y prioridades

**Pregunta.** Las bocinas son **EN54-24**, es decir, certificadas para evacuación por voz. ¿El
sistema de sonido está ligado a la alarma de incendio o a un sistema de voceo con prioridad?

**Por qué importa.** Si hay prioridad (por GPIO del NXAMP o por la central), el audio puede cortarse
o bajar de golpe durante una función. El programa no debe tomarlo como una falla, ni intentar
«recuperar» el nivel. Además, cualquier cambio en los NXAMP podría tocar una instalación de
seguridad certificada.

**Respuesta:** ☐ No está ligado  ☐ Sí → ¿cómo? ________________________________

---

## Datos que conviene pedir de una vez

- [ ] Dirección IP y red del Integra (para controlarlo desde la PC: necesita estar en la misma red local).
- [ ] Red de control de los NXAMP y si se permite conectarse a ella.
- [ ] Copia de la sesión NeMo y de los presets.
- [ ] Plano o croquis del domo con la posición y altura de cada bocina.
- [ ] Nivel de referencia que usan en función (volumen del Integra, por ejemplo −20 dB).
- [ ] ¿Quién autoriza cambios en el Integra y en los NXAMP?

## Notas de la plática

### Respuestas (2026-10-20, por Luis)

1. **Posiciones:** las **ePS10 están en FL, C y FR**. Van acostadas y el motor de agudos **sí se giró**
   correctamente. Falta confirmar: ¿las ePS8 ocupan SL, SR, BL y BR? ¿Cuántos eLS18 hay?
2. **Entradas:** **XLR**. Falta confirmar: según la ficha (dato de terceros), el Integra solo tiene
   XLR en FL, C, FR, SW1 y SW2. ¿Cómo salen SL, SR, BL y BR? (¿cable RCA → XLR?). Siguen sin saberse
   cuántos NXAMP hay ni qué preset tiene cada canal.
3. **Cruce de graves:** lo hace el **NXAMP** (opción B). El Integra debe tener todas las bocinas en
   «Large» y sin cruce propio.
4. **Retardos:** se compensan en el **NXAMP**. En el Integra todas las distancias deben quedar
   iguales, para que no sume retardo.
5. **Dirac y modo de escucha:** **no se sabe.** Se revisa con la lista «Cómo revisar el Integra»
   de `EQUIPOS-SALA.md`.
6. **Alarma de incendio:** **no está ligada.** No hay prioridad ni corte por alarma. Ver
   `NOTA-INSTITUCION-EN54.md`.
