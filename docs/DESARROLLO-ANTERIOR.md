# Documentación del código anterior al EXE recibido

Este documento describe el código fuente previo. Para usar la versión actual, consultar README.md y docs/ESTADO-ACTUAL-2026-10-29.md.

# PARCHE DOLBY · Play Dolby + PLI Espacial

Audio 7.1 para el domo de un planetario: la consola **Allen & Heath Qu-5** manda sus canales por USB
a una PC con Windows. **Play Dolby** (C++) los rutea y los mueve por el domo con **PLI Espacial**,
el motor espacial propio de PLI, y los entrega por HDMI al receptor **Integra DRX-8.4**. De ahí pasan
a los amplificadores NEXO y a las bocinas.

> Cierre temporal de sesión: **2026-10-21**. El estado completo, qué está verificado y qué falta, está
> en **[CLAUDE.md](CLAUDE.md)**. Empieza por ahí.

## Leer primero

| Archivo | Qué tiene |
|---|---|
| [docs/GUIA-DE-USUARIO.md](docs/GUIA-DE-USUARIO.md) | **Guía de usuario** de Play Dolby, con capturas: instalar, cada parte de la pantalla, recetas paso a paso y errores frecuentes |
| [CLAUDE.md](CLAUDE.md) | Estado del proyecto, lo verificado en sala, lo pendiente y todas las herramientas con su comando. Claude Code lo lee solo al abrir la carpeta |
| [PLAN-MOTOR-ESPACIAL-PLI.md](PLAN-MOTOR-ESPACIAL-PLI.md) | PLI Espacial: por qué existe, sus fases (1 a 4 hechas) y la verificación de cada una |
| [native/espacial/ESCENAS.md](native/espacial/ESCENAS.md) | Formato de escenas (JSON) y uso de ADM |
| [EQUIPOS-SALA.md](EQUIPOS-SALA.md) | Ficha técnica de los equipos de la sala y respuestas del ingeniero |
| [SPEC.md](SPEC.md) | Especificación original del proyecto |
| [PREGUNTAS-INGENIERO-SALA.md](PREGUNTAS-INGENIERO-SALA.md) · [NOTA-INSTITUCION-EN54.md](NOTA-INSTITUCION-EN54.md) | Preguntas de sala (ya respondidas) y borrador sobre la norma EN54 |

## Abrir y compilar (Windows 10/11 de 64 bit)

**No hay que instalar nada para compilar.** Se usa Zig 0.14.1 portátil, en `native/tools/`. Si
clonaste el repositorio de GitHub, bájalo primero: el repositorio no lo trae, porque `zig.exe` pesa
164 MB y GitHub no acepta archivos de más de 100 MB. El script verifica la huella SHA-256 oficial.
Cada script de compilación corre primero sus pruebas y solo compila si pasan.

```
pwsh -File native\tools\obtener-zig.ps1   # solo la primera vez en un clon (≈ 78 MB, de ziglang.org)
pwsh -File native\build-cinematic.ps1     # Play Dolby (+ pruebas del puente y de PLI Espacial)
pwsh -File native\build-espacial.ps1      # capas71, escena71, adm71, upmix71 (+ pruebas)
pwsh -File native\build-iamf.ps1          # iamfdec, iamf71 (libiamf, solo PCM)
```

Para usarlo: **`Play Dolby.bat`**. Los ejecutables quedan en `native/bin/`, que no se versiona.
Para hacer el instalador o la USB portable:
`pwsh -File native\empaquetar-usb.ps1 -Destino "D:\PLAY DOLBY" -Reemplazar` (fuentes en `instalador/`).

Opcional:
- **Python 3.12+**, para el kit `pruebas-domo/` y para el renderizador de referencia EBU EAR. El
  entorno de EAR **no viaja entre PC**: se recrea con `pwsh -File native\tools\crear-ear-venv.ps1`.
- **VLC**, para reproducir los WAV 7.1 directo al receptor.

## Mapa del código

| Carpeta | Qué es |
|---|---|
| `native/src/` | Play Dolby. Puente de audio en `bridge_core.hpp` (anillo, servo de deriva y PLI Espacial en vivo). Interfaz Win32 en `main_cinematic.cpp`, `espacial_panel.hpp` (panel y Dynamic Space) y `fuente_ui.hpp` (fuente archivo) |
| `native/espacial/` | **PLI Espacial**: `pli_espacial.hpp` (VBAP 2D desde `domo.json`), `escena.hpp` (escenas), `vivo.hpp` (objetos en tiempo real), `adm.hpp` (ADM BS.2076), y las herramientas `escena71`, `adm71` e `iamf71` |
| `native/upmix/` | `capas71` (separa una canción en 6 capas y las coreografía) y `upmix71` |
| `native/tests/` | Pruebas: `espacial_tests.cpp` (65 452 comprobaciones), `core_tests.cpp` y `channel_sequence_tests.cpp` |
| `native/vendor/` | Terceros: PortAudio (`portaudio.dll` con ASIO, MIT; ver `PORTAUDIO-*.txt/md`) y libiamf + oar (`vendor/iamf`, BSD-3-Clause-Clear; procedencia en `PROCEDENCIA.md`) |
| `native/tools/` | `obtener-zig.ps1` (compilador) y `crear-ear-venv.ps1` (renderizador EBU EAR). Lo que bajan no se versiona |
| `instalador/` · `docs/` | Fuentes del instalador o USB portable, y la guía de usuario con sus capturas |
| `pruebas-domo/` | Kit de señales de prueba del domo y **`domo.json`**, la ficha con el ángulo de cada bocina. Toda la espacialización sale de ahí |
| `musica-7.1/` | Canciones de ejemplo (`origen/`) y sus versiones 7.1, escenas y ADM |
| `memoria-claude/` | Memoria de Claude Code de este proyecto, para llevarla a otra PC (ver `LEEME.txt`) |

## Qué subir si el espacio importa

Todo lo que sigue es **regenerable**; el resto es fuente y documentación.

| Se puede omitir | Peso | Cómo se recupera |
|---|---|---|
| `native/.zig-cache-global`, `native/.zig-cache-local` | 250 MB | Se rehacen solos al compilar (la primera vez tarda unos minutos) |
| `native/tools/ear-venv` | 208 MB | `native\tools\crear-ear-venv.ps1` (no sirve copiarlo) |
| `pruebas-domo/salida` | 1,5 GB | `pruebas-domo\generar-todo.bat` |
| Todo lo de `musica-7.1/` **excepto `origen/`** | 3,5 GB | `capas71`, `escena71` y `adm71` desde `origen/` y las escenas (ver CLAUDE.md) |
| `native/bin` | 115 MB | Los tres `build-*.ps1` |

Si se omite todo eso, el proyecto baja de ≈ 6 GB a ≈ 0,6 GB. **No omitir** `native/tools/zig-*`,
`native/vendor`, `pruebas-domo/domo.json` ni `musica-7.1/origen/`.

## Reglas de trabajo (de Luis)

Español técnico. No afirmar «funciona» sin probarlo. Las decisiones de sala son de Luis: niveles, qué
va a cada bocina, LFE. No instalar ni descargar nada sin preguntar. **Volumen del receptor bajo al
probar.**
