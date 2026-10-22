$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot '..\scripts\check-qu5-driver.ps1') -FunctionsOnly
function Assert($Condition,[string]$Message){if(!$Condition){throw $Message}}
function Evidence([string]$Name,[string]$Version,[bool]$Exists=$true){return [pscustomobject]@{Name=$Name;Version=$Version;FileExists=$Exists;Path='test.dll'}}
Assert ((Resolve-QuEvidence @() @()).State -eq 'NotConfirmed') 'No driver must not be accepted.'
Assert ((Resolve-QuEvidence @((Evidence 'Generic ASIO' '5.72.0')) @()).State -eq 'NotConfirmed') 'Unrelated ASIO accepted.'
Assert ((Resolve-QuEvidence @((Evidence 'Allen & Heath SQ ASIO' '5.72.0')) @()).State -eq 'NotConfirmed') 'SQ accepted as Qu.'
Assert ((Resolve-QuEvidence @((Evidence 'Qu-16 ASIO' '5.72.0')) @()).State -eq 'NotConfirmed') 'Legacy Qu accepted.'
Assert ((Resolve-QuEvidence @((Evidence 'Qu ASIO' '4.86.0')) @()).State -eq 'NotConfirmed') 'Old driver accepted.'
Assert ((Resolve-QuEvidence @((Evidence 'Qu ASIO' '5.72.0' $false)) @()).State -eq 'NotConfirmed') 'Missing DLL accepted.'
Assert ((Resolve-QuEvidence @((Evidence 'Qu ASIO' '5.72.0')) @()).State -eq 'Installed') 'Current Qu ASIO rejected.'
Assert ((Resolve-QuEvidence @((Evidence 'Qu ASIO' '5.73.0')) @()).State -eq 'Installed') 'Newer Qu ASIO rejected.'
$product=@([pscustomobject]@{Name='Allen & Heath Qu-5/6/7 Driver';Version='5.72.0'})
Assert ((Resolve-QuEvidence @() $product).State -eq 'NotConfirmed') 'Uninstall registration alone accepted.'
Assert ((Resolve-QuEvidence @((Evidence 'Qu ASIO' '4.86.0')) $product).State -eq 'NotConfirmed') 'Product entry masks outdated DLL.'
Assert ((Resolve-QuEvidence @((Evidence 'Qu ASIO' '')) $product).State -eq 'Installed') 'Product version fallback failed.'
$temporary=Join-Path $PSScriptRoot ('driver-fixture-'+[guid]::NewGuid().ToString()+'.zip')
try {
    Set-Content -LiteralPath $temporary -Value '<html>Just a moment</html>'
    Assert (!(Test-DriverArchive $temporary)) 'HTML challenge accepted as ZIP.'
    Remove-Item -LiteralPath $temporary -Force
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip=[IO.Compression.ZipFile]::Open($temporary,[IO.Compression.ZipArchiveMode]::Create)
    $entry=$zip.CreateEntry('Setup.exe');$stream=$entry.Open()
    try {$stream.Write((New-Object byte[] 100001),0,100001)}finally{$stream.Dispose();$zip.Dispose()}
    Assert (Test-DriverArchive $temporary) 'Installer archive rejected.'
    Assert (!(Test-CachedDriver $temporary)) 'Archive without receipt accepted as cached download.'
    @{Source=$script:DriverUrl;SHA256=(Get-FileHash -LiteralPath $temporary -Algorithm SHA256).Hash} | ConvertTo-Json | Set-Content -LiteralPath ($temporary+'.json')
    Assert (Test-CachedDriver $temporary) 'Valid cached package rejected.'
    @{Source=$script:DriverUrl;SHA256='changed'} | ConvertTo-Json | Set-Content -LiteralPath ($temporary+'.json')
    Assert (!(Test-CachedDriver $temporary)) 'Changed archive hash accepted.'
}finally{
    Remove-Item -LiteralPath $temporary,($temporary+'.json') -Force -ErrorAction SilentlyContinue
}
Write-Host 'PASS: Qu identity, supported versions, missing DLL, registration, HTML rejection, ZIP and download cache.'
