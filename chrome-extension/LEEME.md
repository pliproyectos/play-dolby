Play Dolby 2026.11.3.1 — conexión de Chrome en otra PC

1. Extrae todo el ZIP en una carpeta fija y abre PlayDolby.exe desde ella. Cierra la versión anterior antes de abrir esta copia.
2. En chrome://extensions desactiva la extensión PLI anterior. Activa Modo de desarrollador, pulsa Cargar descomprimida y elige chrome-extension dentro de la carpeta extraída.
3. Comprueba la versión 2026.11.3.1 y el ID ebpjbolnlhmbebbhiooonakajihpknco. Este ID es el mismo en todas las PC.
4. Inicia el puente del EXE. En la pestaña de música selecciona Con Qu-5 / Play Dolby y pulsa Activar. El panel debe indicar Enlace con Play Dolby activo.
5. Para revisar una falla abre Ver diagnóstico de conexión.

El paquete conserva el ejecutable de sala con receptor Chrome. Se sustituyó únicamente el identificador de origen permitido en ese binario, cuya fuente exacta no está disponible en el repositorio. En modo conjunto, el EXE mezcla las dos señales. La escucha con la consola y el receptor físico sigue pendiente.

Si el transporte falla, reintenta automáticamente con una espera máxima de cinco segundos. «Reconectar Chrome» reconstruye su procesamiento sin reiniciar el EXE ni volver a capturar la pestaña. El enlace activo confirma la conexión; no confirma que el EXE esté reproduciendo o que las bocinas reciban señal.

El panel y la barra de Chrome usan el icono pequeño de PLI (16 px y 32 px para pantallas de alta densidad). Se conservan Música clásica, Envolvente, Órbita y PLI Espacial con buffer de análisis de 12 segundos. No separa instrumentos individuales; el BPM es aproximado y el ambiente se obtiene de la diferencia estéreo.

Pruebas:
- node pruebas/prueba-puente.cjs
- node pruebas/prueba-estabilidad.cjs
- node pruebas/prueba-puente-browser.cjs
- pruebas/prueba-motor.html (37 comprobaciones)
- pruebas/prueba-espacial.cjs (requiere Playwright disponible)
