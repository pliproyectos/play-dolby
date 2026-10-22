# Kit de pruebas del domo 7.1

**Versión 1.0.0 · 2026-10-20.** Generador y archivos propios, con cabecera WAV hecha a mano y verificada.

**Para qué sirve:** comprobar, bocina por bocina, la cadena **PC → HDMI → Integra DRX-8.4 → domo**: qué canal suena en qué bocina, orden, polaridad y niveles.

## Qué está verificado y qué no

| | Estado |
|---|---|
| Archivos WAV: cabecera, máscara, niveles, aislamiento entre canales, polaridad | **VERIFICADO** en escritorio. Ver `salida/VERIFICACION.md` |
| Que el verificador detecta fallas | **VERIFICADO**. Con copias dañadas a propósito detectó los cinco casos: máscara 0xFF, una muestra fuera de lugar, BL↔SL cruzados, −6 dB en un canal y huella alterada. El archivo intacto pasó |
| Lector independiente | **VERIFICADO**. El módulo `wave` de Python 3.13 abre todos los archivos y entrega exactamente los mismos bytes. **No** se probó con ffprobe ni con MediaInfo, porque no están instalados |
| La cadena real (Windows, controlador AMD, HDMI, Integra, bocinas) | **NO VERIFICADO.** Eso es lo que se prueba en el domo con `hoja-de-campo.html` |
| Ángulos de las bocinas en `domo.json` | **PROVISIONALES**: son nominales de una sala 7.1, no medidos. Afectan solo el orden de la vuelta y de los pares de polaridad |

## Contenido

```
salida/48kHz/  y  salida/96kHz/   los programas (.wav) y su manifiesto (.json)
salida/GUION.md                   línea de tiempo de cada programa: qué suena, cuándo y dónde
salida/VERIFICACION.md            reporte de la última verificación
hoja-de-campo.html                para imprimir y llenar en el domo (se abre en el navegador → Imprimir)
domo.json                         ficha del domo: canal → bocina, ángulos y límites de protección
voces/                            voces en español (Windows, voz Raúl es-MX)
generar.py, verificar.py, wav71.py, senales.py, voces.ps1     el generador (Python 3.12 o más nuevo)
verificar.bat, generar-todo.bat   lanzadores
```

Los archivos usan el orden de canales de Windows 7.1, máscara **0x63F**: FL FR FC LFE BL BR SL SR, en PCM de 24 bit.

## Los programas

| Archivo | Dura | Qué hace | Qué debe pasar |
|---|---|---|---|
| `01-identificacion` | 41 s | Una voz dice el nombre de cada canal y después suena ruido rosa en ese canal | Cada voz sale por la bocina que nombra. «Subgrave» se dice por el central y luego suena solo el sub |
| `02-vuelta-al-domo` | 57 s | Un ruido gira por el anillo: dos vueltas en sentido horario y dos en antihorario, 12 s por vuelta. Arranca en el central | Giro suave y continuo. **Un salto** indica un canal cruzado; **un hueco** indica polaridad o nivel |
| `03-polaridad` | 83 s | Para cada par de bocinas vecinas, «uno» es el ruido en fase y «dos» el mismo ruido con una bocina invertida | «Uno» suena más lleno y enfocado entre las dos bocinas. Si «dos» suena mejor, una de las dos está invertida |
| `04-calibracion` | 3:06 | Ruido rosa de 500 Hz a 2 kHz a **−20 dBFS RMS**, 20 s por canal. En el LFE, de 20 a 120 Hz | Con el sonómetro en la butaca central se anota el nivel de cada canal. **Es el más fuerte del kit** |
| `05-rutina-apertura` | 33 s | Voces rápidas por canal y una vuelta | Revisión antes de cada función |
| `06-ref-*` | 5–31 s | Referencias en mono, estéreo, 5.1 con traseros (0x3F), 5.1 con laterales (0x60F) y 7.1 | Muestran adónde manda Windows o el receptor cada formato. Por ejemplo, si el 5.1 con traseros acaba en las laterales o en las traseras |

Todo va a −30 dBFS RMS, salvo la calibración. Las voces llegan a un pico de −14 dBFS. El techo de protección es −3 dBFS de pico: el generador se niega a pasarlo.

## Orden de uso en el domo

**Antes de empezar (seguridad):**
1. Pon el volumen del Integra muy bajo y súbelo poco a poco.
2. Usa el modo de escucha **Direct** o **Multichannel**, sin upmix (Dolby Surround ni Neural:X). Lo de los modos no está verificado en este equipo.
3. Para identificar canales, apaga la corrección de sala (AccuEQ o Dirac).

**P1 — Reproductor del PC directo a HDMI, sin Play Dolby.** Cierra Play Dolby antes, porque usa el modo exclusivo.
1. Revisa en Windows (Sonido → Integra/AMD HDMI → Configurar) que esté en **7.1**.
2. Revisa en Propiedades → Opciones avanzadas la frecuencia del formato predeterminado y usa la carpeta que coincida: `48kHz` o `96kHz`. Si no sabes cuál, usa 48 kHz.
3. Reproduce en este orden: `01` → `02` → `03` → `06` → `04`.
4. Anota en la hoja lo que muestra la pantalla de información del Integra. Debería decir PCM multicanal (8 canales) y la frecuencia; esto no está verificado en este modelo.

**P2 — Tonos internos de Play Dolby.** El mismo canal debe sonar en la misma bocina que en P1. Si no, el cruce está en la app.

**P3 — Consola Qu-5 → USB → Play Dolby**, y **P4 — Nightshade → toda la cadena**. Solo cuando P1 y P2 pasen. Si P1 pasa y P3 falla, el problema está entre la consola y la app.

La falla queda entre el último punto que pasó y el primero que falló.

## En otra computadora

- **Para reproducir:** basta con los WAV. No hace falta Python.
- **Para comprobar que la copia llegó intacta:** doble clic en `verificar.bat`. Necesita Python 3.12 o más nuevo; si no está, el reporte original es `salida/VERIFICACION.md`.
- **Para regenerar todo** (por ejemplo, con la ficha ya medida): edita `domo.json` y abre `generar-todo.bat`. A 96 kHz tarda 1 o 2 minutos.
- **Si cambian los nombres de las bocinas** en la ficha, regenera las voces con `voces.ps1`.

## Pendiente

1. **Ficha real del domo:** ángulos, distancias, cuál es el frente. Se mide en sitio o se toma del menú de bocinas del Integra.
2. **Claqueta en domemaster y prueba de sincronía audio-video** (etapa E de la SPEC). Necesitan ffmpeg y saber qué formatos acepta Nightshade.
3. **Medición con micrófono** (fase 4).
4. **Verificación con un segundo lector externo** (ffprobe o MediaInfo), si se decide instalarlos.
