param([switch]$CheckOnly,[switch]$NoBrowser,[switch]$FunctionsOnly)
$ErrorActionPreference='Stop'
$script:DriverVersion=[version]'5.72.0'
$script:DriverUrl='https://www.allen-heath.com/content/uploads/2025/06/AllenHeath-Qu-Windows-Driver-v5.72.0.zip'
$script:ResourceUrl='https://www.allen-heath.com/hardware/qu/qu-5-qu-5d/resources/'

function Convert-DriverVersion([string]$Value) {
    if($Value -match '\d+\.\d+(?:\.\d+){0,2}') { return [version]$Matches[0] }
    return [version]'0.0'
}
function Test-QuName([string]$Value) {
    # SQ/CQ and legacy Qu-16/24/32 packages do not establish Qu-5 support.
    return $Value -match '(?i)\bQu\b' -and $Value -notmatch '(?i)\b(SQ|CQ|Qu[- ]?(16|24|32))\b'
}
function Resolve-QuEvidence($Asio,$Products) {
    foreach($driver in @($Asio)) {
        if(!(Test-QuName $driver.Name) -or !$driver.FileExists) { continue }
        $version=Convert-DriverVersion $driver.Version
        foreach($product in @($Products)) {
            if((Test-QuName $product.Name) -and $product.Name -match '(?i)Allen\s*(&|and)?\s*Heath') {
                $productVersion=Convert-DriverVersion $product.Version
                if($version -eq [version]'0.0'){$version=$productVersion}
            }
        }
        if($version -ge $script:DriverVersion) {
            return [pscustomobject]@{State='Installed';Name=$driver.Name;Version=$version.ToString();Path=$driver.Path}
        }
    }
    return [pscustomobject]@{State='NotConfirmed';Name='';Version='';Path=''}
}
function Get-QuDriverStatus {
    $asio=@();$products=@()
    # Only the native x64 ASIO registration can be loaded by our x64 application.
    $registry=[Microsoft.Win32.RegistryKey]::OpenBaseKey([Microsoft.Win32.RegistryHive]::LocalMachine,[Microsoft.Win32.RegistryView]::Registry64)
    try {
        $root=$registry.OpenSubKey('SOFTWARE\ASIO')
        if($root) {
            try { foreach($name in $root.GetSubKeyNames()) {
                $key=$root.OpenSubKey($name)
                try {
                    $label=$name+' '+[string]$key.GetValue('Description')
                    if(!(Test-QuName $label)){continue}
                    $clsid=[string]$key.GetValue('CLSID')
                    $server=if($clsid){$registry.OpenSubKey('SOFTWARE\Classes\CLSID\'+$clsid+'\InprocServer32')}else{$null}
                    if($server) {
                        try {$path=[Environment]::ExpandEnvironmentVariables(([string]$server.GetValue('')).Trim('"'))}finally{$server.Dispose()}
                        $exists=Test-Path -LiteralPath $path -PathType Leaf
                        $version=if($exists){(Get-Item -LiteralPath $path).VersionInfo.FileVersion}else{''}
                        $asio+= [pscustomobject]@{Name=$label;FileExists=$exists;Version=$version;Path=$path}
                    }
                }finally{if($key){$key.Dispose()}}
            }} finally {$root.Dispose()}
        }
        foreach($uninstallPath in @('SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall','SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall')) {
            $root=$registry.OpenSubKey($uninstallPath)
            if(!$root){continue}
            try {foreach($name in $root.GetSubKeyNames()){
                $key=$root.OpenSubKey($name)
                if(!$key){continue}
                try {$label=[string]$key.GetValue('DisplayName');if(Test-QuName $label){$products+=[pscustomobject]@{Name=$label;Version=[string]$key.GetValue('DisplayVersion')}}}finally{$key.Dispose()}
            }} finally {$root.Dispose()}
        }
    }finally{$registry.Dispose()}
    return Resolve-QuEvidence $asio $products
}
function Test-DriverArchive([string]$Path) {
    if(!(Test-Path -LiteralPath $Path -PathType Leaf)){return $false}
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip=$null
    try {
        $zip=[IO.Compression.ZipFile]::OpenRead($Path)
        return @($zip.Entries | Where-Object {$_.FullName -match '(?i)\.exe$' -and $_.Length -gt 100000}).Count -gt 0
    }catch{return $false}finally{if($zip){$zip.Dispose()}}
}
function Test-CachedDriver([string]$Path) {
    if(!(Test-DriverArchive $Path) -or !(Test-Path -LiteralPath ($Path+'.json'))){return $false}
    try {
        $receipt=Get-Content -LiteralPath ($Path+'.json') -Raw | ConvertFrom-Json
        return $receipt.Source -eq $script:DriverUrl -and $receipt.SHA256 -eq (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
    }catch{return $false}
}
function Invoke-DriverCheck {
    $architecture=if($env:PROCESSOR_ARCHITEW6432){$env:PROCESSOR_ARCHITEW6432}else{$env:PROCESSOR_ARCHITECTURE}
    if($architecture -ne 'AMD64') {
        Write-Host 'El driver Qu-5/6/7 requiere Windows Intel/AMD x64. No se descargara un paquete incompatible.'
        return 20
    }
    try {$status=Get-QuDriverStatus} catch {
        Write-Host 'No se pudo consultar el registro del controlador. No se asume que falte.'
        Write-Host $script:ResourceUrl
        return 20
    }
    if($status.State -eq 'Installed') {
        Write-Host ('Qu ASIO x64 encontrado, version '+$status.Version+'. Conecta la Qu-5 por USB-C y selecciona su entrada en la app.')
        return 0
    }
    Write-Host 'No se pudo confirmar el controlador Qu-5/6/7 ASIO x64 compatible.'
    if($CheckOnly){return 10}
    $directory=Join-Path (Split-Path $PSScriptRoot -Parent) 'drivers\Qu-5'
    New-Item -ItemType Directory -Force -Path $directory | Out-Null
    $archive=Join-Path $directory 'AllenHeath-Qu-Windows-Driver-v5.72.0.zip'
    if(!(Test-CachedDriver $archive)) {
        Write-Host 'Descargando controlador desde Allen & Heath...'
        try {
            [Net.ServicePointManager]::SecurityProtocol=[Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
            Invoke-WebRequest -UseBasicParsing -Uri $script:DriverUrl -OutFile ($archive+'.partial') -TimeoutSec 30 -ErrorAction Stop
            if(!(Test-DriverArchive ($archive+'.partial'))){throw 'La respuesta no contiene el paquete ZIP del controlador.'}
            Move-Item -LiteralPath ($archive+'.partial') -Destination $archive -Force
            @{Source=$script:DriverUrl;Version=$script:DriverVersion.ToString();SHA256=(Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash;DownloadedUtc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json | Set-Content -LiteralPath ($archive+'.json') -Encoding UTF8
        }catch {
            Write-Host 'La descarga automatica no pudo completarse (conexion o proteccion del sitio).'
            if($NoBrowser){Write-Host 'Abre el enlace oficial para descargar el ZIP.'}else{Write-Host 'Se abrira el enlace oficial en el navegador. Si aparece una verificacion, completala para descargar el ZIP.'}
            Write-Host $script:DriverUrl
            if(!$NoBrowser){Start-Process $script:DriverUrl}
            return 20
        }
    }
    Write-Host ('Controlador descargado y disponible en: '+$archive)
    Write-Host 'Extrae el ZIP y ejecuta su instalador. Despues vuelve a abrir este acceso para comprobar ASIO.'
    Write-Host 'La descarga no instala el controlador ni confirma que la consola este conectada.'
    return 10
}
if(!$FunctionsOnly){exit (Invoke-DriverCheck)}
