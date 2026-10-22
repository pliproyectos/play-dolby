# Extrae el texto de un PDF solo con la biblioteca estándar (no hay poppler ni pdftotext en estas PCs).
# Uso: python -I pdf-texto.py manual.pdf > manual.txt
# Sirve para manuales hechos con Word o Acrobat (fuentes WinAnsi); los PDF de Illustrator con
# texto convertido a curvas no traen texto que extraer. Así se leyeron los manuales NEXO el 2026-10-20.
import re, zlib, sys

data = open(sys.argv[1], 'rb').read()
STR = rb'\((?:\\.|[^\\)])*\)'
pages = []
for m in re.finditer(rb'stream\r?\n(.*?)\r?\nendstream', data, re.S):
    try:
        s = zlib.decompress(m.group(1))
    except Exception:
        continue
    if b'BT' not in s:
        continue
    lines = []
    for bt in re.finditer(rb'BT(.*?)ET', s, re.S):
        line = []
        for t in re.finditer(rb'\[((?:' + STR + rb'|[^\]])*)\]\s*TJ|(' + STR + rb')\s*Tj', bt.group(1), re.S):
            chunk = t.group(1) if t.group(1) is not None else t.group(2)
            for a, n in re.findall(rb'(' + STR + rb')|(-?\d+\.?\d*)', chunk):
                if a:
                    txt = a[1:-1]
                    txt = re.sub(rb'\\([()\\])', rb'\1', txt)
                    line.append(txt.decode('latin-1'))
                elif n and float(n) < -200:
                    line.append(' ')
        lines.append(''.join(line))
    pages.append(' | '.join(l for l in lines if l.strip()))
print('\n'.join(pages))
