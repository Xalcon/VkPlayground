
$projectDir = Resolve-Path "$PSScriptRoot/.."
$sourcePath = "$projectDir/extern/libsdl"
$buildPath = "$projectDir/extern/libsdl/msvc_build"

Write-Host "Configuring LibSDL via cmake, this will take a while. Grab a cup of coffee, do the laundry and walk the dog. Maybe make some food as well." -ForegroundColor Green

$sw = [Diagnostics.Stopwatch]::StartNew()
cmake -G "Visual Studio 15 2017" -S $sourcePath -B "$buildPath/x86"
cmake -G "Visual Studio 15 2017 Win64" -S $sourcePath -B "$buildPath/x64"

# Replace all occurences of SDL2d with SDL2
# > I cant figure out how to override the debug suffix through the cmake commandline for the output files
# > so this is the workaround
foreach($file in (Get-ChildItem -Recurse $buildPath\**\*.vcxproj))
{
    Write-Host "Replacing occurences of SDL2d in $file"
    (Get-Content $file.FullName).replace('SDL2d', 'SDL2') | Set-Content $file.FullName
}

cmake --build "$buildPath/x86" --config debug
cmake --build "$buildPath/x86" --config release
cmake --build "$buildPath/x64" --config debug
cmake --build "$buildPath/x64" --config release
$sw.Stop();

Write-Host "Build LibSDL in $($sw.Elapsed)" -ForegroundColor Green