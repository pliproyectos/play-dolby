---
name: kit-pruebas-domo
description: "Estado del kit de pruebas 7.1 del domo (pruebas-domo/), qué está verificado y qué falta en sitio"
metadata:
  node_type: memory
  type: project
  originSessionId: 6ffbf16b-176a-4f92-84a7-f5467d645443
  modified: 2026-10-20T09:31:05.532Z
---

Kit `PARCHE DOLBY\pruebas-domo\` v1.0.0, creado el 2026-10-20 en casa de Luis, sin conexión al domo. Copiado a la USB KINGSTON (`D:\PARCHE DOLBY\pruebas-domo\`) para llevarlo a otra PC con Windows 11.

- **Verificado (escritorio):** 20 WAV a 48 y 96 kHz, 24 bit, cabecera propia EXTENSIBLE con máscara 0x63F. Los abre el `wave` de CPython 3.13. El verificador detectó los 5 daños que se le metieron a propósito.
- **No verificado:** la cadena física (Windows/AMD → HDMI → Integra → bocinas). Se prueba con `hoja-de-campo.html`, por puntos P1 a P4.
- **Ficha `domo.json` PROVISIONAL:** ángulos nominales. Falta medirla en sitio.
- **Pendiente:** claqueta en domemaster y prueba de sincronía (necesitan ffmpeg y los formatos de Nightshade); medición con micrófono (necesita numpy); segundo lector externo (ffprobe).
- **Esta PC:** Python 3.13.7 sin numpy; no hay ffmpeg, SoX ni MediaInfo. TTS en español: `SelectVoice("Microsoft Sabina Desktop")` falla en pwsh 7 y cae a «Microsoft Raul» (es-MX).

**Why:** Luis pidió dejar hecho todo lo que no requiere estar conectado al domo y llevarlo en USB.
**How to apply:** en la siguiente sesión en sitio, arrancar por P1 con la hoja de campo. Con la ficha medida, regenerar con `generar-todo.bat`. Ver [[plan-banco-pruebas-domo]].
