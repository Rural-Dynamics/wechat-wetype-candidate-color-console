$ErrorActionPreference = 'Stop'

$root = $PSScriptRoot
$build = Join-Path $root 'build'
$resource = Join-Path $build 'WeTypeSkin.res.o'
$output = Join-Path $build 'WeTypeSkinLauncher.exe'

$gxx = (Get-Command g++.exe -ErrorAction Stop).Source
$windres = (Get-Command windres.exe -ErrorAction Stop).Source

New-Item -ItemType Directory -Path $build -Force | Out-Null

& $windres (Join-Path $root 'WeTypeSkin.rc') -O coff -o $resource
if ($LASTEXITCODE -ne 0) { throw 'windres failed' }

& $gxx `
    -std=c++17 -O2 -Wall -Wextra -municode -mwindows `
    -static -static-libgcc -static-libstdc++ -s `
    (Join-Path $root 'WeTypeSkinLauncher.cpp') $resource `
    -o $output `
    -lgdiplus -ltaskschd -lole32 -loleaut32 -luuid -lversion -lcomdlg32 -lgdi32
if ($LASTEXITCODE -ne 0) { throw 'C++ build failed' }

Write-Host "Built: $output"

