"""Empaqueta la carpeta operativa recibida, sin recompilar ni reemplazar su EXE.
Uso: python native/empaquetar-actual.py --origen "E:/New folder"
"""
from pathlib import Path
import argparse, hashlib, json, zipfile

args = argparse.ArgumentParser()
args.add_argument('--origen', required=True)
opts = args.parse_args()
repo = Path(__file__).resolve().parent.parent
source = Path(opts.origen).resolve()
inventory_path = repo / 'docs/PAQUETE-ACTUAL-2026-10-30.json'
inventory = json.loads(inventory_path.read_text(encoding='utf-8'))
version = inventory['version_paquete']
output = repo / 'publicar'
output.mkdir(exist_ok=True)
files = inventory['archivos']

def pack(name, entries):
    target = output / name
    if target.exists():
        raise RuntimeError(f'Ya existe {target}; no se sobrescribe una versión preparada.')
    partial = target.with_suffix(target.suffix + '.partial')
    done = 0
    total = sum(e['bytes'] for e in entries)
    with zipfile.ZipFile(partial, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=1, allowZip64=True) as archive:
        for entry in entries:
            relative = entry['archivo']
            path = (source / relative).resolve()
            if not path.is_relative_to(source):
                raise RuntimeError('Ruta de inventario fuera del paquete')
            digest = hashlib.sha256()
            with path.open('rb') as reader, archive.open(relative, 'w', force_zip64=True) as writer:
                while chunk := reader.read(1024 * 1024):
                    digest.update(chunk)
                    writer.write(chunk)
                    done += len(chunk)
            if digest.hexdigest() != entry['sha256']:
                raise RuntimeError(f'Archivo modificado desde el inventario: {relative}')
            if entry['bytes'] > 20*1024*1024:
                print(f'{name}: {done*100/total:.0f}% — {relative}', flush=True)
        archive.write(inventory_path, 'INVENTARIO-SHA256.json')
    partial.replace(target)
    digest = hashlib.sha256()
    with target.open('rb') as reader:
        while chunk := reader.read(4*1024*1024):
            digest.update(chunk)
    target.with_suffix(target.suffix+'.sha256').write_text(digest.hexdigest()+'  '+name+'\n', encoding='ascii')
    print(f'LISTO {target.name}: {target.stat().st_size} bytes; SHA-256 {digest.hexdigest()}', flush=True)

# GitHub receives only the small app/extension package. Never publish audio files.
software = [e for e in files if not e['archivo'].startswith('demo/') and Path(e['archivo']).suffix.lower() != '.wav']
if any(Path(e['archivo']).suffix.lower() == '.wav' for e in software):
    raise RuntimeError('El paquete de GitHub debe contener cero archivos WAV.')
pack(f'PlayDolby-{version}.zip', software)
