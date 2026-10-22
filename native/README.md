> Aplicación actual: **Play Dolby**. Abrir `Play Dolby.bat` en la carpeta principal.

# Planetario Bridge — Windows C++

Primera implementación funcional, pendiente de validación de la cadena Qu-5 / HDMI en sala. No es todavía una versión certificada para funciones públicas.

## Operación

Ejecutar `bin/PlanetarioBridge.exe` o el lanzador `iniciar-bridge-cpp.bat` de la carpeta superior. No depende de Python. Se inicia detenido y no cambia el dispositivo predeterminado de Windows.

1. Instalar el controlador oficial de la consola y conectar USB. Conectar el Integra a HDMI y encenderlo.
2. Actualizar dispositivos. Elegir entrada ASIO de la consola y salida WASAPI del Integra.
3. Seleccionar 2, 6 u 8 canales. No se degrada silenciosamente a estéreo. Se abre float32 a 48 kHz o se muestra error. Configurar la consola USB a 48 kHz para este prototipo.
4. Parchear cada canal HDMI a una entrada de la consola (1–32) o Silencio. Los nombres corresponden al orden propuesto por Windows: FL FR C LFE BL BR SL SR; verificar físicamente la asignación. El perfil 5.1 usa los primeros seis (surround back); no confundir con una máscara 5.1 side.
5. Probar tonos a volumen bajo: -36 dBFS, 440 Hz (60 Hz LFE), duración de dos segundos, con rampas. Los tonos prueban solo la salida, no la captura.
6. Iniciar puente. Ganancia inicial -18 dB. Detener con botón o Escape. El panel de parcheo se bloquea durante reproducción para impedir cambios de asignación accidentales.

La entrada y salida tienen relojes independientes. El núcleo usa una cola SPSC preasignada, preroll de 50 ms y ajuste gradual de relación de muestreo según el llenado de la cola (máximo ±3000 ppm). No usa locks, archivos ni asignaciones de memoria en los callbacks.

## Alcance y límites

- Captura ASIO o WASAPI y salida WASAPI mediante dos streams PortAudio independientes.
- Ganancia, mute por ruta, medidores y contadores de falta/exceso de muestras, clipping y avisos del driver.
- Interpolación lineal provisional: debe reemplazarse por SRC de calidad de producción y validarse en frecuencia antes del despliegue definitivo. No es un DSP de upmix.
- No incluye limitador avanzado: recorta picos al rango digital y los cuenta. Evitar saturación en la consola.
- No incorpora control del reloj de video de Nightshade ni compensación de imagen. Latencia de audio incluye driver, preroll y salida; se debe medir frente al domo.
- ASIO depende del controlador instalado. No se garantiza compatibilidad con todas las implementaciones de drivers hasta probar el hardware.
- Un stream detenido provoca cierre del puente; se requiere reinicio manual. No se cambia automáticamente de salida.
- Guardado/carga manual de un perfil en `%LOCALAPPDATA%/PlanetarioBridge/perfil.txt`. Dispositivos se comparan por nombre/API/canales, no por índice de enumeración. Dos dispositivos idénticos con el mismo nombre requieren comprobación manual.
- Logs locales en `%LOCALAPPDATA%/PlanetarioBridge/bridge.log`, sin guardar contenido de audio.

## Compilar y probar

Ejecutar `powershell -ExecutionPolicy Bypass -File native/build.ps1` desde la carpeta del proyecto. Compilador portable Zig 0.14.1 (C++17, Windows x64). Tests de ruteo, silencio, buffers y deriva sintética. Esas pruebas no sustituyen una hora de funcionamiento real con la consola e Integra.

## Dependencias

Header PortAudio v19.7.0: https://github.com/PortAudio/portaudio/tree/v19.7.0

Runtime local `libportaudio64bit-asio.dll`, copiado de los binarios distribuidos con sounddevice 0.5.6. PortAudio tiene licencia MIT; ver vendor/PORTAUDIO-LICENSE.txt. El runtime incluye soporte Steinberg ASIO; conservar avisos y revisar sus condiciones antes de distribuir instaladores a terceros. Esta entrega es un prototipo local, no asigna una licencia de distribución al proyecto del usuario.

Compilador: https://ziglang.org/download/0.14.1/zig-x86_64-windows-0.14.1.zip
SHA-256 verificado: 554f5378228923ffd558eac35e21af020c73789d87afeabf4bfd16f2e6feed2c

## Consola objetivo confirmada

Allen & Heath Qu-5. Consultar ../GUIA-CONEXION-WINDOWS-QU5.html. Conexión USB-C de datos, controlador oficial Qu-5/6/7 ASIO/WDM para Windows x64 y USB en Multitrack a 48 kHz. Seleccionar Qu ASIO como entrada y HDMI WASAPI como salida. El puente trabaja a 48 kHz y hasta 8 salidas; el parche USB se configura en la consola. Detectar cualquier ASIO no identifica automáticamente la Qu-5 ni acredita que su controlador funcione. Pendiente validar físicamente la consola, el ruteo y la salida HDMI.

## Latencia reducida (Cinematic Qu-5)

El lanzador Cinematic abre PlanetarioBridge-Cinematic-Qu5.exe. La cola interna deja de usar 2400 muestras fijas (50 ms): parte de 512 muestras (10,67 ms) y se ajusta al doble del mayor bloque observado más dos muestras, con límite de media capacidad. El objetivo se conserva hasta detener el audio. Un bloque grande del controlador puede elevar esta espera; no representa la latencia total USB → HDMI.

PortAudio solicita la latencia baja anunciada por cada dispositivo y permite al backend elegir su tamaño de bloque, evitando exigir una adaptación fija a 256 muestras. La captura publica bloques de hasta 128 muestras; reproducción publica su avance una vez por callback. Los contadores se acumulan localmente. No hay asignaciones de memoria ni escritura de archivos dentro de estos callbacks.

Pruebas: arranque con bloques pequeños, 128/256/480/1024 muestras con deriva ±1000 ppm, ruteo, silencio, saturación de cola y recuperación tras falta de audio. Falta medir latencia física y estabilidad con Qu-5 e Integra. Usar 48 kHz/Multitrack y el controlador Qu-5/6/7; empezar con 128 o 256 muestras en el panel ASIO si están disponibles, y aumentar si aparecen avisos o cortes. El receptor HDMI también puede añadir procesamiento. El registro bridge.log contiene latencias declaradas por los controladores, no una medición acústica.

## Icono y marcos de Windows

Cinematic integra LOGO/miniatura.png convertido a native/assets/pli-app.ico con nueve tamaños, preservando proporciones y transparencia. El recurso 101 identifica el ejecutable, la ventana principal y el probador. native/generate-icon.py permite regenerarlo con Pillow. Los marcos y la barra de título usan DWM con los colores oscuros de la aplicación en Windows 11; versiones anteriores pueden ignorar los atributos de color que no soporten. Se conserva el comportamiento nativo de la ventana.

## Latencia mínima y prueba física — 2026-10-19

**Verificado con Qu-5 e Integra conectados** (`bin/hw_bridge_test.exe`, salida en silencio digital):

- La Qu-5 entrega USB a **96 kHz**; el puente exigía 48 kHz. Esa era la causa de los `Invalid sample rate` de `bridge.log`. Play Dolby ahora toma la frecuencia de la consola.
- HDMI en **WASAPI exclusivo** con máscara 7.1 (0x63F): acepta 8 canales a 44,1/48/96 kHz. En modo compartido, 96 kHz se rechaza. Mientras el puente corre, ningún otro programa suena por el Integra.
- El periodo mínimo del HDMI es de **960 muestras (10 ms)**, aunque se pida menos o se use polling: es un límite del controlador AMD.
- ASIO Qu acepta bloques de 16 a 1024 muestras. Se fija en **64** (0,67 ms): con 16 funcionó 2 min, pero es frágil.
- Cola = bloque de salida + bloque de entrada + **margen de 192 muestras**. Con margen 32 hubo 50 faltas en 30 s; 96 pasó, pero justo; 192 pasó con holgura.
- Latencia del puente (declarada por los drivers + cola medida): **~43 ms → ~20 ms**. Prueba de 2 min: 0 faltas, 0 desbordes, 0 avisos del driver, corrección de reloj de 20 a 55 ppm.

**Pendiente:** la latencia acústica real (incluye el procesamiento del Integra) no se ha medido; la interfaz gráfica no se probó con estos cambios (solo se compiló); falta una función completa de 60 min.

Prueba física: `Prueba Hardware.bat` en la carpeta principal (cerrar Play Dolby antes).
