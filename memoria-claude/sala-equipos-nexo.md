---
name: sala-equipos-nexo
description: "Equipos de audio del planetario (Qu-5, Integra DRX-8.4, NXAMP4x1 Mk2, NEXO ePS10/ePS8-EN54, eLS18) y respuestas de sala del 2026-10-20 (cruce y retardos en NXAMP, sin alarma)"
metadata:
  node_type: memory
  type: project
  originSessionId: d0c97d44-cdbe-4b6c-844f-da7a94ce1940
  modified: 2026-10-20T18:36:06.400Z
---

Inventario que pasó Luis el 2026-10-20 (fotos de la lista de equipo): Allen & Heath **Qu-5** → PC (Play Dolby) → **Integra DRX-8.4** → **NEXO NXAMP4x1 Mk2** (amplificador con DSP) → bocinas pasivas **ePS10-EN54** (10"+1,4") y **ePS8-EN54** (8"+1,4") y subwoofer **eLS18** (18"). Ficha completa, con fuentes: `EQUIPOS-SALA.md`.

Hechos de manual que cambian el software:
- Las bocinas **exigen procesador NEXO**: el NXAMP hace el cruce, la EQ y la protección. Play Dolby no hace de limitador de bocinas.
- El ePS8 baja a 80 Hz y el ePS10 a 70 Hz. **El ePS8 no tiene preset oficial con el eLS18.**
- NXAMP: latencia de 0,58 ms, retardo de hasta 1 s por canal, protocolo IP no publicado.
- El Integra solo tiene XLR en FL, C, FR, SW1 y SW2 [dato de terceros].

**Respuestas de Luis, 2026-10-20:**
1) Las ePS10 están en FL, C y FR, acostadas y con el motor girado.
2) Las entradas son XLR (no cuadra con el Integra: falta ver cómo salen los envolventes).
3) **El cruce lo hace el NXAMP.**
4) **Los retardos, en el NXAMP.**
5) Dirac y modo: no se sabe; hay una lista paso a paso en `EQUIPOS-SALA.md`.
6) **No hay liga con la alarma de incendio.** Nota para la institución en `NOTA-INSTITUCION-EN54.md`. No afirmar «cumplimos EN54»: eso aplica al sistema completo (EN54-2/-4/-16), no a las bocinas.

**Faltan:** las posiciones de las ePS8, cuántos eLS18 y NXAMP hay, el preset de cada canal, las distancias medidas y el estado de Dirac.

**Why:** con el cruce en el NXAMP y el Integra en Large, al eLS18 solo llega el LFE; con contenido estéreo convertido, el sub no suena a menos que se genere LFE.
**How to apply:** proponer el Integra en Large, con distancias iguales y en Direct. Generar LFE ya no está vetado, pero lo decide y prueba Luis en sala. No tocar presets del NXAMP. Las decisiones de sala son de Luis. Ver [[parche-dolby-estado]].
