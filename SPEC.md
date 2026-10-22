# Planetario Audio 7.1 — Especificación de desarrollo

Estado: borrador v0.3, Integra DRX-8.4 y procesamiento en vivo confirmados; pendiente de conexión y pruebas físicas.
Fecha: 2026-10-08.
Metodología: desarrollo guiado por especificaciones (Spec-Driven Development).

## 1. Del punto A al punto B

**Punto A:** planetario con instalación de altavoces 7.1 y reproducción mediante Nightshade G3 / StratoScript. Receptor AV Integra DRX-8.4 confirmado. El sistema todavía no está conectado. Se sospecha Linux en el equipo de Nightshade, sin confirmación. Equipo reproductor y formatos de películas pendientes.

**Punto B:** reproducir películas con audio sincronizado por ocho canales hacia el sistema 7.1; conservar mezclas nativas y ofrecer conversión opcional de mono, estéreo o 5.1. Validar primero una aplicación en Windows y después portar a macOS.

La especificación no implica que Nightshade esté instalado en este PC ni que su instalación admita Windows o macOS. La aplicación de audio es el componente que se desarrollará primero para Windows.

## 2. Decisiones de transporte

- Ruta principal: PC → salida HDMI de GPU/interfaz → entrada HDMI del receptor/procesador → amplificación → altavoces.
- Formato inicial objetivo: PCM lineal, ocho canales, 48 kHz; negociar profundidad y formato real con el dispositivo. Procesamiento interno en float32.
- El receptor y el controlador deben anunciar y aceptar ocho canales. Un puerto HDMI por sí solo no garantiza esa capacidad; verificar EDID, intermediarios y configuración.
- Si “fibra óptica” significa TOSLINK/S/PDIF convencional: normalmente PCM estéreo o surround comprimido compatible, habitualmente 5.1. No se acepta como transporte de PCM 7.1.
- Si se trata de ADAT, MADI u otro enlace óptico profesional, identificar protocolo e interfaces antes de evaluar su uso. El medio óptico no define por sí solo el número de canales.
- No confundir HDMI directo con ARC. Si existen ARC/eARC, extractores, matrices o extensores, verificar cada equipo y formato de extremo a extremo.
- El prototipo no requiere codificar Dolby/DTS: la salida PCM evita incorporar un codificador propietario. Cualquier distribución de codecs o bibliotecas requiere revisar sus licencias.

## 3. Qué significa emular 7.1

La conversión (upmix) crea una distribución espacial a partir de los canales disponibles. No reconstruye posiciones ni canales originales ausentes.

Modos:
1. Nativo: mantener los ocho canales sin upmix.
2. Estéreo a 7.1: conservar frente L/R, extraer centro de manera regulable y generar ambiente lateral/trasero mediante componente diferencial y decorrelación.
3. 5.1 a 7.1: conservar frente, centro y LFE; derivar canales traseros a partir del ambiente con balance regulable.
4. Mono: centro principal y ambiente opcional, conservador.
5. Bypass: conservar contenido original y dejar sin señal los canales no presentes.

No duplicar la señal completa a todas las bocinas como modo predeterminado. Preservar inteligibilidad del narrador y comprobar compatibilidad mono. El LFE es un canal de efectos; el manejo de graves del receptor no es equivalente. Dejar generación de LFE desactivada inicialmente para evitar doble procesamiento.

## 4. Alcance del primer producto

Aplicación local de escritorio, sin servicio en nube. Panel para seleccionar dispositivo, comprobar capacidad de ocho canales, identificar bocinas, cargar audio de prueba, seleccionar modo, ajustar intensidad, monitorear niveles y guardar perfiles.

El primer prototipo reproducirá archivos de audio controlados. No capturará automáticamente cualquier película de Nightshade. Esa integración es una etapa posterior con una prueba independiente de sincronización y encaminamiento.

Fuera del MVP: driver virtual propio, Dolby Atmos, seguimiento de objetos, separación por IA, control completo del domo y garantía de compatibilidad con contenido protegido.

## 5. Arquitectura propuesta

Archivo/entrada autorizada → decodificación → normalización explícita de layout → remuestreo si hace falta → DSP de upmix o bypass → ganancias y protección de picos → salida nativa del sistema → HDMI.

Núcleo de audio portable en C++ con CMake; evaluar JUCE para dispositivos e interfaz, verificando licencia antes de adoptarlo. Backend Windows mediante WASAPI y macOS mediante Core Audio. Mantener DSP y perfiles independientes de la plataforma. Evaluar FFmpeg para inspección/decodificación, documentando configuración y licencia concreta antes de distribuirlo.

La interfaz web puede servir como panel, pero no se confiará en el navegador como motor de salida HDMI de ocho canales.

Contrato del núcleo: bloques PCM float32, frecuencia conocida y etiquetas explícitas de canales. Layout lógico: FL, FR, FC, LFE, SL, SR, BL, BR. Traducir al orden real del backend; no asumir que el índice de archivo coincide con una bocina. Rechazar layouts ambiguos o pedir asignación explícita.

El hilo de audio no debe realizar I/O de archivos, asignaciones dinámicas ni trabajo de interfaz. Registrar underruns, formato negociado y cambios de dispositivo fuera de ese hilo.

## 6. Integración con Nightshade y StratoScript

La documentación oficial confirma reproducción de varias fuentes de audio/video en G3, pero esta investigación no confirma una API de audio externo, selección de ocho canales ni control remoto adecuado para la instalación concreta.

Orden de evaluación:
1. Probar primero salida 7.1 nativa del reproductor existente; si funciona, usarla para contenido nativo.
2. Evaluar conversión previa de archivos autorizados. Conservar originales y validar formato admitido, duración y sincronización en Nightshade.
3. Evaluar motor externo sincronizado solo si hay comandos/interfaz documentados que permitan iniciar, pausar, buscar y detener de forma fiable.
4. Evaluar captura en tiempo real si las anteriores no resuelven el uso. WASAPI loopback captura un endpoint en modo compartido; no garantiza captura de streams exclusivos ni de contenido protegido. Evitar recapturar la propia salida y generar realimentación. Puede requerir un endpoint virtual de terceros, sujeto a validación y licencia.

No inventar comandos StratoScript. Toda integración debe registrar versión, comando documentado y prueba reproducible. Si Nightshade corre en otra computadora, definir enlace y reloj; loopback de este PC no captura audio remoto.

## 7. Requisitos verificables

- R01: enumerar dispositivos y mostrar capacidad/formato realmente verificados.
- R02: impedir iniciar salida 7.1 si no se puede abrir un formato de ocho canales; explicar la causa sin degradar silenciosamente a estéreo.
- R03: activar cada bocina por separado con nivel conservador, silencio inicial y parada inmediata.
- R04: conservar layout y contenido nativo, salvo ganancias/protección elegidas explícitamente.
- R05: ofrecer upmix opcional y bypass, con parámetros reproducibles y guardados.
- R06: evitar clipping y valores no finitos; documentar margen y comportamiento del limitador.
- R07: ante desconexión HDMI, detener de forma segura y solicitar reselección; no reproducir inesperadamente en otra salida.
- R08: sincronizar audio/video en inicio, pausa, búsqueda y reanudación para el modo integrado aprobado.
- R09: mostrar y guardar incidencias técnicas sin recopilar contenido audiovisual.
- R10: portar el mismo DSP a macOS y verificar nuevamente el orden físico de canales.

## 8. Etapas y puertas de aceptación

### A — Inventario y diagnóstico
Entregable: diagrama físico, modelos/versiones y tabla de formatos.
Aceptación: determinar qué equipo reproduce, quién recibe HDMI, protocolo óptico y si Windows anuncia ocho canales. Si no existe esa capacidad, resolver hardware/configuración antes del DSP.

### B — Salida Windows de ocho canales
Entregable: herramienta de enumeración y prueba individual de canales.
Aceptación: ocho destinos correctos, sin cruces; receptor confirma PCM multicanal. Probar primero a volumen bajo. Registrar frecuencia, formato, controlador y ruta.

### C — Motor de archivos y bypass
Entregable: reproducción de archivos de referencia mono, estéreo, 5.1 y 7.1.
Aceptación: layout correcto, duración correcta, sin clipping ni pérdidas en 30 minutos continuos. El contenido 7.1 conserva sus canales.

### D — Upmix
Entregable: perfiles conservadores para narración, documental y música.
Aceptación: pruebas DSP con impulsos, silencio, señal correlacionada y diferencial; salida finita y picos controlados. Escucha en sala confirma voz estable y ambiente útil, sin afirmar equivalencia a una mezcla nativa.

### E — Integración con planetario
Entregable: una película de prueba autorizada con ruta elegida y procedimientos de operación.
Aceptación: objetivo provisional de desfase audiovisual absoluto ≤40 ms, medido al inicio y tras 60 minutos, incluyendo pausa/búsqueda cuando aplique. Acordar medición y tolerancia final con el operador. Registrar deriva, underruns y recuperación de fallos. No prometer latencia hasta medir hardware real.

### F — Piloto y operación
Entregable: paquete Windows, perfiles y guía de conexión/recuperación.
Aceptación: función completa supervisada en sala, con retorno operativo probado a la reproducción existente.

### G — macOS
Entregable: backend Core Audio y paquete para la arquitectura de Mac elegida.
Aceptación: repetir B–F; verificar adaptador HDMI, capacidad anunciada y configuración de Audio MIDI. La portabilidad del código no garantiza la del hardware.

## 9. Pruebas y evidencia

Separar pruebas automáticas DSP de validación física. Guardar resultados de canal identificado, picos, conteo de underruns, desconexión/reconexión y sincronía. Usar material sintético o autorizado. No afirmar que la instalación funciona por haber pasado pruebas de software.

Estado actual: se elaboró la especificación y se consultó documentación; no hay aplicación implementada, pruebas de audio ejecutadas ni validación del sistema instalado.

## 10. Información pendiente del usuario

1. Forma prevista de conexión de las bocinas al Integra DRX-8.4 y existencia de amplificación externa.
2. Modelo y sistema operativo del equipo que ejecuta Nightshade; versión exacta de G3.
3. GPU o dispositivo HDMI y presencia de matrices, extractores o extensores.
4. Qué significa fibra óptica en esta instalación: TOSLINK, ADAT, MADI u otro.
5. Archivo de ejemplo autorizado y sus pistas/layouts; si el audio vive dentro de la película o separado.
6. Distribución física de bocinas y ubicación de narración/centro en el domo.
7. Requisito confirmado: procesamiento en vivo; no sustituirlo por conversión previa de películas.

## 11. Fuentes técnicas

- Nightshade G3: https://www.nightshadesoftware.org/projects/nightshade/wiki/Nightshade_G3
- Diferencias de scripting G3/NG: https://www.nightshadesoftware.org/projects/nightshade/wiki/Nightshade_G3_and_NG_Scripting_Differences_Compared_to_Legacy
- Microsoft WASAPI loopback y límite de modo exclusivo: https://learn.microsoft.com/en-gb/windows/win32/coreaudio/loopback-recording
- Dolby: receptor HDMI debe aceptar PCM de ocho canales: https://professionalsupport.dolby.com/s/question/0D54u00009otwEWCAY/renderer-direct-ouput-to-hdmi?language=en_US
- GDC: ejemplo explícito de HDMI PCM 7.1 frente a óptico PCM 2.0/AC3 5.1 (no sustituye el manual del equipo instalado): https://www.gdc-tech.com/wp-content/uploads/2025/03/AIB-3000-16-Channel-Brochure_EN_PB-1470-2503-V5E_SP.pdf

## 12. Decisión de alcance: procesamiento en vivo (v0.2)

Confirmado por el usuario: procesador Integra, instalación aún sin conectar y procesamiento en vivo. Linux en el equipo de Nightshade sigue siendo una hipótesis. La documentación consultada de G3 no identifica el sistema operativo de esta instalación.

El requisito final es capturar el audio real que reproduce Nightshade, procesarlo y entregar PCM 7.1 por HDMI de forma continua. La reproducción de archivos de la etapa C se mantiene únicamente como herramienta de prueba del motor. La conversión offline no satisface el producto solicitado.

### Dos rutas de despliegue a validar

A. Motor en el mismo equipo de Nightshade: audio del reproductor → encaminamiento interno/monitor → DSP → HDMI → Integra. Si se confirma Linux, se necesita un backend Linux acorde con el servidor de audio disponible (ALSA/PulseAudio/PipeWire u otro). Requiere acceso al equipo y validar compatibilidad con el sistema administrado por Digitalis. No se instalará nada ni se modificará el sistema sin identificarlo y disponer de recuperación.

B. Procesador externo Windows, después macOS: salida real del equipo Nightshade → interfaz de captura compatible → motor DSP → salida HDMI → Integra. La entrada HDMI del Integra no es una interfaz de captura para el PC. Un puerto HDMI de salida de GPU tampoco recibe audio. La interfaz debe exponer el audio entrante al sistema operativo, con canales, frecuencia y latencia verificados. Capturar estéreo permite upmix; capturar solo estéreo de una mezcla 7.1 pierde los canales discretos y no satisface preservación nativa.

La ruta B permite mantener Windows como primer destino sin asumir que puede capturar remotamente un equipo Linux. Añade captura, buffers y posible diferencia de relojes; hay que medir latencia, deriva y sincronía con el video del domo. Si la ruta es solo audio y este llega tarde, retrasar más el audio no lo corrige: será necesario un ajuste de video compatible o reducir latencia. No prometer sincronía sin esa prueba.

### Puerta previa al desarrollo de integración

Elegir A o B con evidencia del sistema operativo y conectores disponibles. Para B, verificar captura y salida simultáneas antes de comprar o elegir una interfaz. Para A, verificar encaminamiento de audio documentado y permitido en la instalación. Si hay dos relojes, implementar compensación de deriva mediante remuestreo adaptativo y medirla durante una función completa.

### Formato de entrega

No se requiere un archivo espacial ni metadatos Atmos para distribuir audio a ocho bocinas. El motor genera ocho señales PCM etiquetadas y el enlace HDMI las transporta al Integra, si ese modelo admite PCM multicanal. Como referencia, el manual oficial del Integra DHC-60.7 enumera PCM de hasta ocho canales; eso no confirma las capacidades del equipo del usuario.

El Integra podría incluir un modo de upmix suficiente para el objetivo. Revisar el manual del modelo y probarlo forma parte del diagnóstico, antes de desarrollar una función que el receptor ya resuelva. No combinar upmix del programa y del receptor sin una decisión explícita.

Fuentes adicionales:
- Digitalis, G3 y reproductor de medios: https://www.digitaliseducation.com/about
- Digitalis, sistema integrado y compatibilidad: https://www.digitaliseducation.com/faq.html
- Ejemplo Integra DHC-60.7, PCM hasta ocho canales: https://integrahometheater.com/wp-content/uploads/2021/06/DHC-60.7_BAS_ADV_En_web.pdf

## 13. Equipo confirmado: Integra DRX-8.4 (v0.3)

El usuario confirmó el modelo DRX-8.4. Es un receptor AV, con procesamiento y amplificación; comprobar si la instalación usa sus salidas amplificadas o PRE OUT con amplificadores externos.

La documentación oficial del modelo describe entrada PCM multicanal y modos de expansión en tiempo real. Dolby Audio - Surr (DSur) permite ampliar entradas de dos canales o 5.1 a 7.1, según configuración de bocinas y formato compatible. DTS Neural:X ofrece otra opción, sujeta a la tabla de formatos/modos. No hace falta crear un archivo Atmos para este objetivo.

### Primera prueba obligatoria de viabilidad

1. Identificar una salida de audio utilizable en el equipo Nightshade. HDMI es la ruta preferida para conservar mezclas 7.1 nativas; no se ha confirmado que esté libre o disponible.
2. Conectar esa salida a una entrada HDMI del DRX-8.4 y configurar la asignación de entrada. Si también transporta imagen del domo, verificar toda la ruta de video antes de modificarla.
3. Configurar distribución 7.1 real: frontales L/R, centro, laterales L/R, traseros L/R y subwoofer. Confirmar terminales, niveles y distancias con tonos internos.
4. Reproducir material estéreo autorizado, comprobar en información del receptor que la entrada es de dos canales y seleccionar Dolby Audio - Surr. Verificar salida a las siete bocinas y graves según configuración.
5. Repetir con 5.1 y 7.1 conocidos. Para 7.1 nativo usar un modo que conserve la mezcla y verificar información de entrada, sin tomar actividad en todas las bocinas como prueba de canales discretos.
6. Evaluar narración, ambiente, sincronía y estabilidad durante una función. AllCh Stereo se reserva como comparación: distribuye estéreo por varias bocinas y no equivale a una mezcla espacial dirigida.

Esta prueba puede satisfacer el requisito de upmix en vivo directamente en el receptor, evitando la captura y la latencia de un PC intermedio. El proyecto de software sigue disponible para funciones que la prueba demuestre necesarias: perfiles específicos del domo, ajustes de matriz, diagnóstico y monitorización. No declarar necesaria una aplicación de upmix antes de comparar la solución incorporada.

La prueba Windows del motor propio y la portabilidad a Mac siguen en la hoja de ruta; la conexión directa Nightshade → Integra sirve como referencia de calidad y latencia. Si no hay HDMI utilizable, estudiar salida óptica/analógica para upmix de fuentes reducidas, dejando claro que no conserva PCM 7.1 nativo.

Fuentes oficiales del modelo:
- Modos, incluyendo DSur, DTS Neural:X y Multichannel: https://us-integrasupport.zendesk.com/hc/en-us/articles/17985846230164-DRX-8-4-Listening-Mode-Effect
- Manual DRX-8.4: https://assets.integrahometheater.com/product-manuals/SN29404075C_DRX-8-4_En_250131.pdf
- Producto: https://integrahometheater.com/products/drx-8-4-11-4-channel-network-av-receiver

Estado: documentación revisada; no se han realizado conexiones ni pruebas físicas.

## 14. Implementación C++ iniciada — 2026-10-17

Se creó `native/` con aplicación Win32 C++17, ejecutable local `native/bin/PlanetarioBridge.exe` y runtime PortAudio con soporte ASIO. Entrada ASIO/WASAPI y salida WASAPI independientes; frecuencia fija 48 kHz; perfiles de 2/6/8 canales; parcheo por destino desde entradas 1–32; tonos de dos segundos; medidores y contadores; guardado manual de perfil y logs locales.

Validación completada: compilación Windows x64, arranque de interfaz, aislamiento de rutas y silencio, muestras no finitas, límite de buffer y deriva simulada ±1000 ppm por aproximadamente 106 segundos en cada sentido. Prueba de interfaz confirmó rechazo de ocho canales sobre salida estéreo antes de abrir audio. No se realizó reproducción física ni puente con la consola. El modelo objetivo confirmado es Allen & Heath Qu-5; su validación física sigue pendiente.

Pendiente para producción: confirmar modelo/controlador de consola, activar HDMI al Integra, comprobar máscara y orden de canales, sustituir interpolación lineal por remuestreo de mayor calidad, probar reconexión y estabilidad en hardware, medir latencia/deriva audiovisual y completar una función supervisada. No confundir el prototipo compilado con una instalación terminada.
