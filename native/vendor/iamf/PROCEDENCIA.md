# Procedencia de `native/vendor/iamf`

Copia **sin modificar** del código de terceros que usa `native/build-iamf.ps1`. Descarga autorizada
por Luis el 2026-10-20.

| Componente | Repositorio | Versión | Commit | Licencia |
|---|---|---|---|---|
| libiamf (solo la carpeta `code`) | https://github.com/AOMediaCodec/libiamf | tag `v2.0.0` (2026-09-21) | `1c78d481d6ca16b9ec9855ed599de3d20cb4b6c8` | BSD-3-Clause-Clear (`libiamf/LICENSE`) |
| oar (submódulo `code/dep_external/src/oar`) | https://github.com/AOMediaCodec/oar | 2026-07-13 | `3d1d23b807543f993a1d0cf0a9839c7f0746d94b` | BSD-3-Clause-Clear (`oar/LICENSE`) |

Cambios respecto al original:
- Se descargó solo `code/` (*sparse checkout*); la carpeta `tests/` de 3,3 GB no se incluye.
- Se borraron dos WAV de prueba del renderizador binaural (`oar/src/renderer/obr/.../cli/testdata/*.wav`,
  41 MB), porque el binaural no se compila.
- Se quitaron los metadatos `.git` para poder versionar esta copia dentro del repositorio del proyecto.

Los flujos de prueba de `pruebas/` (`test_000206`, `test_000038` y sus WAV de referencia) vienen de la
carpeta `tests/` del mismo commit.
