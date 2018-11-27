$projectDir = Resolve-Path "$PSScriptRoot/.."
$sourcePath = "$projectDir/extern/libsdl"
$buildPath = "$projectDir/extern/libsdl/msvc2017_build"
set-location $sourcePath
cmake -G "Visual Studio 15 2017" -S $sourcePath -B $buildPath

& "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\MSBuild\15.0\Bin\MSBuild.exe" "$buildPath/SDL2.vcxproj"