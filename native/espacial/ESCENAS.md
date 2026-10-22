# Escenas de PLI Espacial

Una **escena** es un archivo JSON que dice qué sonidos hay y por dónde se mueven en el domo.
`escena71.exe` la convierte a un WAV 7.1 en segundos, sin volver a separar nada.

```
native\bin\escena71.exe musica-7.1\pli-espacial\escena.json
```

Escribe `<nombre> - 7.1-escena.wav` y `<nombre>-escena.txt` (con la medición) junto a la escena.
Si se da un segundo argumento, ese es el WAV de salida.

## Cómo obtener una escena

- **Desde una canción estéreo:** `capas71.exe cancion.wav carpeta --domo pruebas-domo\domo.json`
  separa la canción en 6 capas (`carpeta\capas\*.wav`) y escribe `carpeta\escena.json` con la
  coreografía de siempre. A partir de ahí se edita la escena y se vuelve a correr `escena71`.
- **A mano:** cualquier WAV mono o estéreo sirve como objeto: efectos, voz del narrador, pistas
  exportadas de un DAW (PCM de 16/24 bit o flotante de 32 bit). **Todos a la misma frecuencia.**

## Ángulos

Grados vistos desde arriba: **0 = frente (centro)**, positivo hacia la **derecha**, negativo hacia la
izquierda. 90 = lateral derecho, 180 = atrás, −90 = lateral izquierdo. Se puede pasar de 360 (720 =
dos vueltas). Las bocinas reales salen de la ficha `pruebas-domo/domo.json`.

## Formato

```json
{
  "nombre": "Mi escena",
  "ficha": "../../pruebas-domo/domo.json",
  "velocidad": [[0, 0], [8, 0], [16, 1]],
  "objetos": [
    { "nombre": "narrador", "archivo": "voz.wav", "mono": true, "azimut": 0 },
    { "nombre": "nave", "archivo": "nave.wav", "mono": true, "inicio_s": 30, "nivel_db": -3,
      "movimiento": { "tipo": "puntos", "puntos": [[30, -90], [36, 0], [42, 90], [48, 180]] } },
    { "nombre": "ambiente", "archivo": "amb.wav", "ancho": 90,
      "movimiento": { "tipo": "giro", "inicio": 0, "rpm": 1.5 } },
    { "nombre": "melodia", "archivo": "mel.wav", "mono": true,
      "movimiento": { "tipo": "vaiven", "centro": 0, "amplitud": 25, "periodo_s": 24 } }
  ],
  "lfe": { "fuentes": ["ambiente"], "corte_hz": 80, "ganancia": 0.5 }
}
```

### Escena

| Campo | Qué es | Si falta |
|---|---|---|
| `nombre` | Nombre de la escena y de los archivos de salida | «escena» |
| `ficha` | Ruta a `domo.json`, relativa a la escena o absoluta | `null`: anillo clásico de capas71 |
| `velocidad` | Curva `[[tiempo, factor], …]` que comparten todos los giros y vaivenes que no traen la suya | factor 1 siempre |
| `objetos` | Lista de objetos (al menos uno) | — |
| `lfe` | Qué se manda al subwoofer | **sub en silencio** |

### Objeto

| Campo | Qué es | Si falta |
|---|---|---|
| `nombre` | Único en la escena | obligatorio |
| `archivo` | WAV mono o estéreo, relativo a la escena | obligatorio |
| `mono` | `true`: suma L+R y lo pone como un solo punto | `false` |
| `ancho` | Estéreo: L va a azimut − ancho y R a azimut + ancho (0 a 180) | 0 |
| `nivel_db` | Nivel del objeto | 0 dB |
| `inicio_s` | Segundo de la escena en que empieza a sonar | 0 |
| `azimut` | Posición fija (si no hay `movimiento`) | — |
| `lfe` | `true`: va **directo** al subwoofer (mono, sin paneo ni filtro), como el LFE de un ADM | `false` |
| `movimiento` | Uno de los tres tipos de abajo | — |

### Movimientos

| `tipo` | Campos | Qué hace |
|---|---|---|
| `puntos` | `puntos: [[t, ángulo], …]` | Va de un punto a otro en línea recta. Dos puntos con el mismo `t` hacen un salto |
| `giro` | `inicio` (grados), `rpm` (vueltas por minuto; negativo = antihorario) | Gira sin parar. `velocidad` multiplica las rpm |
| `vaiven` | `centro`, `amplitud` (grados), `periodo_s` | Se mece centro ± amplitud. `velocidad` multiplica la amplitud, con tope de 1 |

Cada movimiento puede traer su propia `velocidad`. Si no la trae, usa la de la escena.
**Los tiempos son siempre de la escena**, no del objeto: un objeto con `inicio_s: 30` y puntos en 30 s
empieza a moverse en cuanto empieza a sonar.

### LFE

`lfe` manda al subwoofer la suma de las `fuentes`, pasada por un pasa-bajos de 24 dB/octava en
`corte_hz` (20 a 200), por `ganancia`. **Sin `lfe` el sub no recibe nada.** En la sala el cruce lo
hace el NXAMP (`EQUIPOS-SALA.md`): con contenido estéreo convertido, el sub solo suena si la escena
lo pide. La decisión es de Luis y se prueba en sala con volumen bajo.

## ADM: intercambio con otros programas (fase 4)

```
native\bin\adm71.exe exporta escena.json salida-adm.wav
native\bin\adm71.exe importa entrada-adm.wav carpeta --domo pruebas-domo\domo.json
```

- **Exportar:**
  - escribe un WAV con una pista por objeto (dos si es estéreo) y los metadatos ADM (ITU-R
    BS.2076: bloques `chna` y `axml`);
  - las trayectorias se muestrean cada 0,1 s;
  - el LFE sale como pista «DirectSpeakers» LFE.
- **Importar:**
  - las pistas «Objects» se vuelven objetos con trayectoria por puntos, y las «DirectSpeakers», fijos
    (o LFE directo);
  - escribe `carpeta\pistas\`, `carpeta\escena.json` (editable) y el 7.1 renderizado.
- **No soportado:** HOA (Ambisonics), Matrix y Binaural se avisan y se omiten. La elevación se
  proyecta al anillo horizontal, con aviso.
- **Convención:** en ADM el azimut positivo es a la izquierda. La conversión de signo es automática.

## Protección

- La salida se normaliza **solo hacia abajo**, con pico máximo de −1 dBFS. El reporte dice cuánto
  se bajó.
- Se rechaza con mensaje claro: nombres repetidos, tiempos fuera de orden, WAV de frecuencias
  distintas, fuentes de LFE que no existen y fichas con huecos de 180° o más entre bocinas.
