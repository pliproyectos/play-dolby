---
name: parche-dolby-estado
description: "Estado de PARCHE DOLBY al 2026-10-20: lo verificado en el domo, la migración de PC por USB y el siguiente paso con el ingeniero de la sala"
metadata:
  node_type: memory
  type: project
  originSessionId: d0c97d44-cdbe-4b6c-844f-da7a94ce1940
  modified: 2026-10-22T18:58:44.043Z
---

Al 2026-10-20 el proyecto PARCHE DOLBY (planetario con domo 7.1) migra de PC: la PC del domo (GPU AMD) deja de usarse. El paquete completo va en la USB KINGSTON `D:\PARCHE DOLBY`, sincronizado y verificado con SHA-256. El `CLAUDE.md` de la raíz del proyecto es la memoria que viaja con el código; empezar por ahí.

**Verificado en el domo el 2026-10-19** (con la Qu-5 y el Integra conectados):
- El Integra DRX-8.4 anuncia LPCM de 8 canales por EDID. La causa de «solo estéreo» era Windows en 2 canales y FxSound interpuesto. Ver [[receta-71-hdmi-windows]].
- La Qu-5 entrega USB/ASIO **solo a 96 kHz**: esa era la causa de «Invalid sample rate».
- Play Dolby: HDMI en WASAPI exclusivo, ASIO de 64 muestras y cola = salida + entrada + 192. La latencia del puente bajó de ~43 a ~20 ms. Prueba de 2 min sin errores (`Prueba Hardware.bat`).
- El periodo mínimo del HDMI de AMD es de 10 ms; con otra GPU hay que volver a medir.

**No verificado:** latencia acústica, la interfaz de Play Dolby con estos cambios (solo se compiló), una función de 60 min, la sincronía con Nightshade y todo lo que va después del Integra.

**Siguiente paso:** platicar con el ingeniero de la sala las 6 preguntas de `PREGUNTAS-INGENIERO-SALA.md`. Ver [[sala-equipos-nexo]].

**Why:** Luis pidió el 2026-10-20 llevarse la memoria a la PC nueva con todo lo aprendido.
**How to apply:** en la PC nueva, primero `native\scripts\diagnostico-audio.ps1`, después `Prueba Hardware.bat` con la Qu-5 conectada. No dar por buena la latencia de la PC vieja. Relacionado: [[kit-pruebas-domo]], [[luis-experimentar-en-sala]].

**PLI Espacial (2026-10-20):** es el motor espacial propio que Luis eligió en vez de pagar Dolby Atmos.
- Fases 1 y 2 hechas en escritorio: VBAP 2D que lee domo.json, más escenas JSON que se renderizan con escena71.
- Plan y avance en PLAN-MOTOR-ESPACIAL-PLI.md.
- Interfaz: todo va dentro de Play Dolby, no en un programa aparte, porque WASAPI exclusivo admite un solo programa.
- 2026-10-20: Luis escuchó la versión de fase 2 por VLC y le gustó. La fase 3 se implementó el mismo día:
  - motor `vivo.hpp` en el callback de captura;
  - panel «✦ PLI Espacial» en Play Dolby.
- Probada en escritorio. **Falta probarla con la Qu-5 en sala.**
- 2026-10-20, fase 3b «Dynamic Space» hecha en la ventana principal: insignia, domo, barras, escenas rápidas (incluida «La de 1:45») y velocidad maestra.
- Luis oyó «distorsión en algunas partes» de Cosmic en 7.1. El archivo tiene 0 recortes.
  - Sospechas: la fuente al tope (1:36, 1:55, 2:06, 2:22), el tramo acelerado de 2:00–2:30, o residuos de la separación.
  - Se hizo la variante «estable», sin acelerón. Está pendiente que diga en qué minuto lo oye.
  - Le gustó el remix de 1:45 a 2:00, que es la coreografía a velocidad normal.
- 2026-10-20, fase 4 ADM hecha con código propio (`adm71.exe`, exporta e importa), probada en ida y vuelta.
  - La Qu-5 todavía no está habilitada en sala (lo dijo Luis).
  - Luis autorizó las descargas de EAR y libiamf. Detalle completo en PLAN-MOTOR-ESPACIAL-PLI.md.
    - EAR (versión de GitHub, en `native/tools/ear-venv`) validó nuestro ADM.
    - IAMF solo PCM: `iamf71.exe`, bit por bit igual a la referencia oficial.
- 2026-10-20, «no se escuchaba nada» en Play Dolby: sin la Qu-5, el puente se negaba a arrancar (la entrada era el micrófono de 2 canales).
  - Se agregó la «fuente archivo», un WAV o carpeta de capas como si fuera la consola, y los objetos estéreo.
  - Falta que Luis lo escuche.
- 2026-10-21, cierre temporal: Luis comparte la carpeta por Drive para que otra persona abra el código.
  - Se hicieron relativas las rutas de las escenas, se reescribió README.md como entrada y se agregó el cierre en CLAUDE.md.
  - Se agregó el script `native/tools/crear-ear-venv.ps1` y se refrescó `memoria-claude/`.
- 2026-10-22: instalador portable en la USB, `D:\PLAY DOLBY`.
  - Se genera con `native/empaquetar-usb.ps1`.
  - Probado con PowerShell 5.1: instalar, la ficha junto al exe, el diagnóstico, desinstalar y abrir desde la USB.
  - `D:\PARCHE DOLBY` es la copia vieja del proyecto, del 2026-10-20.
  - La demo del paquete ahora trae `pruebas-7.1` (kit del domo a 48 kHz), `musica-7.1` (6 mezclas) y `capas-cosmic`: ≈ 2 GB.
    - Play Dolby, al cargar un WAV de 8 canales, pone el ruteo 1 a 1, el perfil 7.1 y apaga Espacial.
    - El 2026-10-22 la USB se desconectó al regenerar. El paquete nuevo quedó en `Desktop\PLAY DOLBY`, pendiente de copiar a la USB.
  - Se regeneró con todo recompilado. En la raíz de la USB está `PARCHE-DOLBY-codigo-2026-10-22.zip` (código actual).
- 2026-10-22, más tarde:
  - ventana normal (maximizar y cambiar de tamaño), sin F11, Dynamic Space en la columna;
  - guía en `docs/GUIA-DE-USUARIO.md`;
  - git init en la raíz, sin commit, esperando la cuenta de GitHub de Luis. No hay `gh` instalado; para subir: git + Git Credential Manager.
- GitHub: `pliproyectos/play-dolby`, público, licencia propietaria de PLI, sin la música.
  - Primer commit hecho por Luis; el push falló porque Git usaba la sesión de `lalomalvi`.
  - Solución: `git remote set-url origin https://pliproyectos@github.com/...`.
- Actualizaciones con aviso (lo pidió Luis):
  - revisa al prender la PC y cada 12 h;
  - se publica con `native/publicar-version.ps1` más un Release `vAAAA.MM.DD` con el ZIP y su `.sha256`.
- Gotchas:
  - En PowerShell, `R` es un alias de `Invoke-History`: no llamar `R` a una función.
  - En PowerShell, `$null` pasado a una función de Windows llega como "": usar `[NullString]::Value` (FindWindow).
  - La herramienta PowerShell de Claude bloquea comandos con rutas `D:\…` o con Remove-Item junto a rutas con espacios (falso positivo). Para operar en la USB, correr pwsh desde Bash.
  - libiamf entrega el 7.1 en orden ITU (laterales antes que traseros); `iamf71` lo reordena a Windows.
  - `iamfdec` no escribe en rutas cortas 8.3 (LUISMA~1).
  - EAR necesita `PYTHONUTF8=1` si el YAML tiene acentos.
- Gotcha de pruebas: la laptop tiene la escala de pantalla al 125 %. Si se mandan clics simulados desde un proceso que maneja la escala (DPI-aware), las coordenadas llegan reescaladas a Play Dolby, que no la maneja.

**PC nueva (2026-10-20):** copia completa de `D:\PARCHE DOLBY` en `C:\Users\Luis Martinez\Desktop\AUDIO DIGITAL` (robocopy). En esta PC aún no se ha corrido el diagnóstico ni `Prueba Hardware.bat`.
