# Refuerzo del centro a los frontales — pendiente de integrar

Petición: conservar FC y sumar una copia suave a FL y FR para mejorar la presencia frontal con Nightshade/Qu-5. Punto de partida: −12 dB por copia (0,2511886 lineal), sin retardo ni envío adicional a traseras o SUB.

El módulo `native/src/center_to_front.hpp` está preparado y cuenta con pruebas independientes. Todavía NO está conectado al motor ni incluido en PlayDolby.exe. No se ha publicado una actualización de audio: el código fuente que produjo el EXE operativo sigue en la otra computadora.

Al recibir esa fuente, integrar `bridge::centerToFront(frame, bridge::CenterToFrontMinus12Db)` exclusivamente en la rama de consola, después de su ruteo y antes de sumar Chrome. Aplicar una rampa al activar o modificar la cantidad, ofrecer activación y comparación con bypass, y mantener separado el modo de prueba de bocinas para que FC siga identificando solo la bocina central.

La copia mantiene el signo y la fase de FC. Si la suma supera escala completa, el módulo reduce conjuntamente los ocho canales de ese cuadro para evitar recorte y conservar las relaciones espaciales; en ese caso el centro también se atenúa. Si la mezcla posterior con Chrome puede exceder escala completa, conservar además la protección final del mezclador.

Esto amplía el centro; no demuestra ni corrige por sí solo la causa del predominio trasero. Las señales ya presentes en L/R pueden sumarse o cancelarse según su fase. Validar con voces y contenido real en sala, así como saturación, sincronía y ausencia de procesamiento duplicado, antes de publicar un EXE.
