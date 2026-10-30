# PLI Espacial 7.1 — extensión 2026.10.29.3

Para Chrome y Qu-5 juntos, usa «Con Qu-5 / Play Dolby». El EXE mezcla las fuentes y es el único que abre HDMI. «Solo Chrome» se usa con el puente del EXE detenido.

Actualiza en la misma carpeta que Chrome ya tiene cargada, conservando el identificador. El EXE recibido acepta blhfjpfedgoeolfcpejpehpikainihpd. Si se cambia de ruta al reinstalar, el identificador puede cambiar y el EXE rechazar la conexión. Consulta «Ver diagnóstico de conexión».

Después de copiar los archivos, pulsa Recargar en chrome://extensions y activa otra vez la captura en la pestaña de música. En el paquete USB, ACTUALIZAR-EXTENSION.bat ayuda a copiar a la carpeta existente y guarda un respaldo.

Si el transporte falla, reintenta automáticamente con una espera máxima de cinco segundos. «Reconectar Chrome» reconstruye su procesamiento sin reiniciar el EXE ni volver a capturar la pestaña. El enlace activo confirma la conexión; no confirma que el EXE esté reproduciendo o que las bocinas reciban señal.

El panel incluye el logo PLI. Se conservan Música clásica, Envolvente, Órbita y PLI Espacial con buffer de análisis de 12 segundos. No separa instrumentos individuales; el BPM es aproximado y el ambiente se obtiene de la diferencia estéreo.

Pruebas:
- node pruebas/prueba-puente.cjs
- node pruebas/prueba-estabilidad.cjs
- node pruebas/prueba-puente-browser.cjs
- pruebas/prueba-motor.html (37 comprobaciones)
- pruebas/prueba-espacial.cjs (requiere Playwright disponible)

Ver LEER-PRIMERO-2026-10-29.md junto al EXE para el procedimiento completo y las limitaciones pendientes de verificar en sala.
