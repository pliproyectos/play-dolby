# Play Dolby · PLI Ingeniería Aplicada

Paquete actual **2026.10.30.2**. Conserva el EXE que entregó Luis, incorpora PLI Espacial para Chrome y añade un BAT generador de señales 7.1 offline.

- [Play Dolby.exe](native/bin/PlayDolby.exe)
- [Generar las pruebas de audio](native/bin/GENERAR-PRUEBAS-7.1.bat): crea cuatro archivos PCM 24 bit a 48 o 96 kHz.
- [Actualizar extensión Chrome](Actualizar%20extension%20Chrome.bat): la copia a su carpeta actual para conservar el ID.
- [Descargar actualización y SHA-256](https://github.com/pliproyectos/play-dolby/releases/tag/v2026.10.30.2)

Para escuchar Chrome junto con la Qu-5, inicia el puente del EXE y selecciona **Con Qu-5 / Play Dolby**. Para Chrome solo, detén el puente y selecciona **Solo Chrome**.

El ZIP del Release trae el EXE y la extensión; excluye los WAV de demos y música. Al ejecutar el BAT se generan tonos técnicos en la carpeta que el usuario elija. El repositorio mantiene los pequeños fixtures de audio que pertenecen a las pruebas del código fuente.

La fuente actualizada que produjo el EXE del 28 de octubre permanece en la otra computadora. El `native/src` disponible es anterior y no reconstruye el binario actual. La prueba física prolongada con Qu-5 y el receptor sigue pendiente.

Lee [el estado y los pasos de uso](docs/ESTADO-ACTUAL-2026-10-30.md).

El [refuerzo del central hacia FL/FR](docs/REFUERZO-CENTRAL-PENDIENTE.md) tiene un módulo probado para integrar cuando llegue la fuente actual del EXE. Todavía no forma parte de la aplicación publicada.
