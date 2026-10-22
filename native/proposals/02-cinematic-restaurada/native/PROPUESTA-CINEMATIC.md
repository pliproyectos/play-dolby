# Cinematic restaurada · Allen & Heath Qu-5

Abrir `iniciar-propuesta-cinematic.bat` desde la carpeta principal. El acceso comprueba que existen el ejecutable y PortAudio; si falta el ejecutable, intenta reconstruirlo con el compilador incluido.

Diseño recuperado del historial y fotografía del usuario: fondo #111119, superficies oscuras, acentos violeta #A835F7 / #C187FF, logo PLI blanco con transparencia y proporción original, título “El sonido. En su lugar.” en dos líneas y anillos violetas animados sobre el fondo. Ocho tarjetas FL, FR, C, LFE, BL, BR, SL, SR con ruteo USB, medidores sin parpadeo y botones de prueba. Icono de Windows incorporado y marcos oscuros.

El probador de canales comparte entrada, salida, perfil, ganancia y rutas con la ventana principal. Incluye secuencia con pausas y escucha de entrada real. Escape y Detener cancelan la reproducción sin reiniciar automáticamente el puente.

Entrada de consola objetivo: Allen & Heath Qu-5, USB-C de datos, controlador oficial Qu-5/6/7, Multitrack a 48 kHz. Salida HDMI WASAPI de 2, 6 u 8 canales según los que anuncie Windows. Se conservan el núcleo de baja latencia, sincronización por bloques y comprobaciones de formato. La latencia total requiere medir los equipos físicos.

Para compilar: `powershell -NoProfile -ExecutionPolicy Bypass -File native/build-cinematic.ps1`. El compilador y la DLL de audio se incluyen localmente. El respaldo anterior a la restauración está en `native/proposals/antes-restauracion-cinematic`.

## Corrección del parpadeo

La presentación usa dos superficies persistentes: lienzo lógico y fotograma final al tamaño de la ventana. El fondo y la animación se componen fuera de pantalla; la zona invalidada llega a la ventana con una única copia, evitando el borrado visible previo. La superficie final se recrea al cambiar dimensiones y se libera al cerrar. Se mantienen F11, escalado y actualización limitada a la región de anillos. Compilación y pruebas de audio aprobadas; falta comprobar visualmente el movimiento en el equipo del usuario.

## Comprobación del controlador al iniciar

El acceso BAT ejecuta `native/scripts/check-qu5-driver.ps1`. Consulta ASIO x64, el archivo DLL registrado y su versión; si no tiene versión, utiliza como evidencia adicional el producto Qu de Allen & Heath instalado. No confunde un ASIO genérico ni los paquetes SQ/CQ o Qu-16/24/32 con Qu-5. Requiere evidencia de versión compatible 5.72.0 o posterior y no interpreta una consola desconectada como ausencia del driver. Esta revisión confirma registro y archivo, no el funcionamiento físico.

Si no se confirma el controlador, intenta descargar el ZIP Qu-5/6/7 v5.72.0 desde el enlace oficial verificado el 17/10/2026 y guardarlo en `native/drivers/Qu-5`. Rechaza respuestas HTML, comprueba que el ZIP contiene un instalador y guarda URL y SHA256 para reutilizar la descarga sin repetirla. No instala ni ejecuta el paquete automáticamente. Extraer el ZIP e instalar con el asistente del fabricante; después abrir el BAT otra vez.

Si la web protege la descarga o falla la conexión, muestra el aviso y abre el mismo enlace oficial en el navegador, donde puede requerir una verificación manual. No presenta esa situación como descarga completada. Tras leer el aviso puede abrirse Cinematic para probar HDMI. No descarga un controlador incompatible en Windows ARM o x86. Un error consultando el registro se informa sin afirmar que el controlador esté ausente.

Fuente: https://www.allen-heath.com/hardware/qu/qu-5-qu-5d/resources/
