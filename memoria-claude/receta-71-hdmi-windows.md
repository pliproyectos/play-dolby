---
name: receta-71-hdmi-windows
description: "Receta verificada para que Windows saque 7.1 por HDMI a un receptor (EDID, FxSound, mmsys 7.1, VLC directo al dispositivo, WASAPI exclusivo)"
metadata:
  node_type: memory
  type: project
  originSessionId: d0c97d44-cdbe-4b6c-844f-da7a94ce1940
  modified: 2026-10-20T10:36:19.474Z
---

Verificada el 2026-10-19 con un Integra DRX-8.4 y una GPU AMD. Sirve para cualquier «la PC solo detecta estéreo por HDMI».

1. **¿El receptor puede?** Leer el EDID en `HKLM\SYSTEM\CurrentControlSet\Enum\DISPLAY\*\Device Parameters\EDID` y buscar en los descriptores de audio (SAD) **LPCM con 8 canales**. Si no aparece, es problema de hardware o de algún intermediario, no de Windows.
2. **Quitar mejoradores virtuales:** FxSound es un dispositivo de 2 canales que se pone delante de la salida real. Se vuelve a abrir solo al iniciar Windows.
3. `mmsys.cpl` → la salida HDMI → Configurar → **7.1 envolvente**. Comprobar en el registro (MMDevices\Audio\Render\{id}\Properties, valor `{f19f064d-...},0`): 8 canales, máscara **0x63F**.
4. **Reproducir directo al receptor** con VLC: `--mmdevice-audio-device={0.0.0.00000000}.{id}`, sin depender de la salida predeterminada.
5. Un programa propio que quiera baja latencia o 96 kHz: **WASAPI exclusivo** con máscara 0x63F (en compartido, 96 kHz se rechaza).

Todo esto está automatizado, en solo lectura, en `native\scripts\diagnostico-audio.ps1`.

**Why:** costó un diagnóstico completo descubrir que el hardware sí soportaba 7.1 y que el tope era la configuración.
**How to apply:** ante «solo estéreo», correr el diagnóstico antes de proponer hardware o software nuevo. Cambiar la configuración de sonido de Windows lo hace Luis; yo solo verifico. Ver [[parche-dolby-estado]].
