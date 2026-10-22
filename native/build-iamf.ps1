# IAMF (PLI Espacial fase 4): compila libiamf v2.0.0 + oar (sin binaural) + iamfdec con el Zig portátil.
# Solo PCM: sin Opus/AAC/FLAC (serían otras descargas). Fuente: native\vendor\iamf\libiamf (AOMediaCodec, BSD-3-Clause-Clear).
# Uso: pwsh -File native\build-iamf.ps1   → native\bin\iamfdec.exe
$ErrorActionPreference='Stop'
$env:ZIG_GLOBAL_CACHE_DIR=Join-Path $PSScriptRoot '.zig-cache-global'
$env:ZIG_LOCAL_CACHE_DIR=Join-Path $PSScriptRoot '.zig-cache-local'
$zig=Join-Path $PSScriptRoot 'tools\zig-x86_64-windows-0.14.1\zig.exe'
if(-not (Test-Path $zig)){throw 'Falta el compilador Zig. Corre primero: pwsh -File native\tools\obtener-zig.ps1'}
New-Item -ItemType Directory -Force (Join-Path $PSScriptRoot 'bin') | Out-Null
$code=Join-Path $PSScriptRoot 'vendor\iamf\libiamf\code'
$oar=Join-Path $code 'dep_external\src\oar'
$oarSrc=@('common/cvector.c','common/cqueue.c','common/clog.c','common/carray.c','common/cdeque.c','common/chashmap.c','utility/animation.c',
  'renderer/audio_element_renderer.c','renderer/audio_elements_renderer.c','renderer/audio_renderer_base.c','renderer/renderer_library_manager.c',
  'renderer/renderer_library_utils.c','renderer/ear/arch/arm/matrix_render_arm.c','renderer/ear/arch/x86/matrix_render_x86.c','renderer/ear/matrix_render.c',
  'renderer/ear/h2m_rdr.c','renderer/ear/m2m_rdr.c','renderer/ear/ear.c','renderer/downmix/downmix_renderer.c','renderer/downmix/downmix.c',
  'renderer/olr/object_audio_renderer/object_based_audio_renderer.c','renderer/olr/object_audio_renderer/layout.c','renderer/olr/object_audio_renderer/utils.c',
  'renderer/olr/object_audio_renderer/numc.c','renderer/olr/object_audio_renderer/gain_calculator/gain_calculator.c',
  'renderer/olr/object_audio_renderer/gain_calculator/custom_gain_calculator.c','renderer/olr/object_audio_renderer/gain_calculator/convex_hull.c',
  'renderer/olr/object_audio_renderer/gain_calculator/dbap_panner_2d.c','renderer/olr/object_audio_renderer/gain_calculator/vbap_panner_2d.c',
  'renderer/olr/object_audio_renderer/gain_calculator/vbap_region.c','renderer/olr/object_audio_renderer/gain_calculator/vog.c',
  'renderer/olr/object_audio_renderer/block_processor/block_processing_channel.c','renderer/olr/object_audio_renderer/block_processor/interpret_object_metadata.c',
  'renderer/olr/object_audio_renderer/block_processor/processing_block.c','renderer/olr/object_audio_renderer/renderer.c','renderer/olr/olr.c',
  'limiter/oar_limiter.c','oar.c','oar_utils.c') | % { Join-Path $oar "src/$_" }
$iamfSrc=(Get-ChildItem (Join-Path $code 'src') -Recurse -Filter *.c).FullName + (Get-ChildItem (Join-Path $code 'dep_external\src\wav') -Filter *.c).FullName
$toolSrc=(Get-ChildItem (Join-Path $code 'test\tools\iamfdec\src') -Filter *.c).FullName
$inc=@("$code/include","$code/dep_external/include","$code/dep_external/include/wav","$oar/include","$oar/src","$oar/src/common","$oar/src/utility","$oar/src/renderer",
  "$oar/src/renderer/ear","$oar/src/renderer/olr","$oar/src/renderer/olr/object_audio_renderer","$code/src/iamf_dec","$code/src/iamf_dec/codec",
  "$code/src/iamf_dec/codec/pcm","$code/src/iamf_dec/common","$code/src/iamf_dec/obu","$code/test/tools/iamfdec/include") | % { "-I$_" }
$out=Join-Path $PSScriptRoot 'bin\iamfdec.exe'
& $zig cc -std=gnu11 -O2 -w @inc @oarSrc @iamfSrc @toolSrc -o $out
if($LASTEXITCODE){throw 'Error al compilar libiamf / iamfdec.'}
& $zig c++ -std=c++17 -O2 -Wall -Wextra (Join-Path $PSScriptRoot 'espacial\iamf71.cpp') -o (Join-Path $PSScriptRoot 'bin\iamf71.exe')
if($LASTEXITCODE){throw 'Error al compilar iamf71.'}
Write-Host "IAMF: $out · iamf71.exe"
