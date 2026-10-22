# Descarga el compilador Zig 0.14.1 (x86_64-windows) del sitio oficial y verifica su SHA-256.
# El repositorio no lo incluye: zig.exe pesa 164 MB y GitHub rechaza archivos de más de 100 MB.
#   pwsh -File native\tools\obtener-zig.ps1        (≈ 78 MB, queda en native\tools\zig-x86_64-windows-0.14.1)
$ErrorActionPreference = 'Stop'
$version = '0.14.1'
$nombre = "zig-x86_64-windows-$version"
$url = "https://ziglang.org/download/$version/$nombre.zip"
$sha256 = '554f5378228923ffd558eac35e21af020c73789d87afeabf4bfd16f2e6feed2c'  # ziglang.org/download/index.json
$destino = Join-Path $PSScriptRoot $nombre
if (Test-Path (Join-Path $destino 'zig.exe')) { Write-Host "Zig $version ya está en $destino"; exit 0 }
$zip = Join-Path $PSScriptRoot "$nombre.zip"
Write-Host "Descargando $url ..."
Invoke-WebRequest -UseBasicParsing -Uri $url -OutFile $zip
$h = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLower()
if ($h -ne $sha256) { Remove-Item -LiteralPath $zip -Force; throw "La huella SHA-256 no coincide ($h). No se usó el archivo." }
Expand-Archive -LiteralPath $zip -DestinationPath $PSScriptRoot -Force
Remove-Item -LiteralPath $zip -Force
if (-not (Test-Path (Join-Path $destino 'zig.exe'))) { throw "No quedó $destino\zig.exe después de descomprimir." }
Write-Host "Zig $version listo en $destino (SHA-256 verificado)."
