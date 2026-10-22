> MODELO CORREGIDO: el usuario confirmó Qu-5. Esta investigación SQ-5 queda como referencia de otro modelo; no usar su cableado ni su driver para la Qu-5. Consultar GUIA-CONEXION-WINDOWS-QU5.html en la carpeta principal.

# Investigación SQ-5: USB, controlador y diagnóstico automático

Fecha: 17 de octubre de 2026. Investigación contrastada con otro agente y documentación oficial. Modelo asumido por contexto: Allen & Heath SQ-5; confirmar etiqueta física si se trata de otra consola.

## Requisitos

- Windows 10/11 Intel/AMD x64: SQ USB Audio Driver v4.86.0, según la página oficial de recursos consultada.
- Instalar siguiendo las instrucciones oficiales antes de conectar la consola.
- Puerto USB-B trasero de SQ-5 para audio/MIDI. El puerto SQ-Drive es para almacenamiento.
- Un cable USB-C de datos del PC a USB-B de la consola es una adaptación del conector; su compatibilidad con el controlador USB de ese PC debe verificarse. El manual describe USB-A a USB-B, no certifica todos los puertos/adaptadores USB-C.
- Interfaz 32x32 a 48/96 kHz. Para multicanal utilizar ASIO; WDM usa USB 1/2.
- El puente actual trabaja a 48 kHz: fijar USB de SQ a 48 kHz antes de abrir los streams.
- El parche USB de la consola debe enviar las señales deseadas al PC; elegir un driver no configura este parche automáticamente.

## Audio versus configuración

USB transporta audio y MIDI. El protocolo MIDI permite un conjunto documentado de parámetros. Para configurar la consola con SQ-MixPad se requiere Ethernet/red y versiones compatibles: las primeras dos cifras de firmware/app deben coincidir. La web lista firmware 1.6.3 y MixPad 1.6.1. No se propone actualizar firmware sin conocer la versión instalada.

## Diagnóstico automático propuesto (pendiente de implementar)

1. Enumerar USB/PnP con SetupAPI/Configuration Manager e identificar SQ por IDs de hardware verificados, no solamente nombres amigables.
2. Leer estado PnP y problema: código 28 indica ausencia de controlador seleccionado. Otros códigos deben conservar su causa específica.
3. Consultar registros ASIO x64/x86 y verificar el controlador SQ y su DLL; un registro aislado no acredita funcionamiento.
4. Enumerar entradas PortAudio ASIO y comprobar disponibilidad de formato 48 kHz y cantidad de entradas requerida por las rutas. No abrir captura ni emitir tonos al detectar la consola.
5. Separar estados: sin conexión identificada; hardware presente sin controlador; controlador registrado sin hardware; controlador con error; dispositivo enumerado pendiente de validación; formato compatible. No llamar 'audio validado' hasta probar la cadena real.
6. Reaccionar a conexión/desconexión con diagnóstico en segundo plano y actualizar la UI. No reinicializar PortAudio mientras hay streams activos ni iniciar reproducción automáticamente.
7. Mostrar botón a descargas oficiales; después de la instalación, volver a comprobar. Mantener la instalación del controlador como acción explícita.

## Resultado de la inspección local

La consulta de solo lectura no encontró dispositivos presentes cuyo nombre coincidiera con Allen/Heath/SQ-5/SQ5/SQ USB. Tampoco encontró controladores en las claves ASIO consultadas. Esto es indicio de que no hay controlador ASIO registrado; no prueba ausencia de hardware, ya que un dispositivo desconocido puede no tener el nombre de la consola. No se instaló nada.

## Fuentes

- https://www.allen-heath.com/hardware/sq/sq-5/resources/
- https://support.allen-heath.com/hc/en-gb/articles/45947079593105-SQ-SQ-Drive-and-USB-B
- https://support.allen-heath.com/hc/en-gb/articles/41332471203985-SQ-5-SQ-6-SQ-7-Getting-Started-Guide
- https://www.allen-heath.com/hardware/sq/sq-mixpad/
- https://www.allen-heath.com/content/uploads/2023/05/SQ-MIDI-Protocol-Issue4.pdf
- https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/device-manager-problem-codes
