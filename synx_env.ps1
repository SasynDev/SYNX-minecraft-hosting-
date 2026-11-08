# SYNX HOSTING - Build Environment Initialization Script
# Windows 11 Toolchain Setup (MSVC 2022 C++20 + Windows SDK + Ninja + Qt 6.8.3)

$msvcBin  = "C:\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64"
$msvcInc  = "C:\BuildTools\VC\Tools\MSVC\14.44.35207\include"
$msvcLib  = "C:\BuildTools\VC\Tools\MSVC\14.44.35207\lib\x64"
$sdkVer   = "10.0.26100.0"
$sdkBin   = "C:\Program Files (x86)\Windows Kits\10\bin\$sdkVer\x64"
$sdkInc   = "C:\Program Files (x86)\Windows Kits\10\Include\$sdkVer"
$sdkLib   = "C:\Program Files (x86)\Windows Kits\10\Lib\$sdkVer"
$ninjaBin = "C:\Qt\Tools\Ninja"
$qtBin    = "C:\Qt\6.8.3\msvc2022_64\bin"
$qtLib    = "C:\Qt\6.8.3\msvc2022_64\lib"
$qtInc    = "C:\Qt\6.8.3\msvc2022_64\include"
$javaBin  = "C:\Program Files\Microsoft\jdk-21.0.12.8-hotspot\bin"
$nodeBin  = "D:\"
$pnpmBin  = "$env:APPDATA\npm"

$env:PATH = "$msvcBin;$ninjaBin;$sdkBin;$qtBin;$javaBin;$nodeBin;$pnpmBin;$env:PATH"
$env:INCLUDE = "$msvcInc;$sdkInc\ucrt;$sdkInc\shared;$sdkInc\um;$sdkInc\winrt;$qtInc"
$env:LIB = "$msvcLib;$sdkLib\ucrt\x64;$sdkLib\um\x64;$qtLib"
$env:CMAKE_PREFIX_PATH = "C:\Qt\6.8.3\msvc2022_64"

Write-Host "[SYNX] Build environment initialized successfully for MSVC x64 C++20." -ForegroundColor Cyan

