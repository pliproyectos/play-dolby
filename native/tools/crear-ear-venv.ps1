# Recrea el entorno aislado del renderizador ADM de referencia de la EBU (EAR) en native\tools\ear-venv.
# Un venv de Python NO se puede copiar entre PC (guarda rutas absolutas): en una PC nueva, bórralo y corre esto.
# Requiere Python 3.12+ en el PATH e internet. Descarga ≈ 55 MB (todo queda dentro de esta carpeta).
#   pwsh -File native\tools\crear-ear-venv.ps1
# Notas (2026-10-20):
#  - Se usa EAR del GitHub de la EBU: la versión 2.1.0 de PyPI no corre en Python 3.14 (usa pkg_resources).
#  - EAR declara numpy~=1.14; se instala con numpy 2 a propósito (no hay numpy 1.x para Python 3.14). Funcionó.
#  - Para correr EAR con el YAML del domo (tiene acentos): $env:PYTHONUTF8='1'
$ErrorActionPreference='Stop'
$venv=Join-Path $PSScriptRoot 'ear-venv'
if(Test-Path $venv){ throw "Ya existe $venv. Bórralo primero si quieres recrearlo." }
python -m venv $venv
$py=Join-Path $venv 'Scripts\python.exe'
& $py -m pip install --disable-pip-version-check --no-deps "ear @ https://github.com/ebu/ebu_adm_renderer/archive/refs/heads/master.zip"
& $py -m pip install --disable-pip-version-check --only-binary=:all: numpy==2.5.4 scipy==1.18.1 lxml==6.1.3 PyYAML==6.0.3 attrs==26.1.0 six==1.17.0 multipledispatch==1.0.0 importlib_resources==7.1.0
& (Join-Path $venv 'Scripts\ear-render.exe') --help | Select-Object -First 3
Write-Host "EAR listo en $venv"
