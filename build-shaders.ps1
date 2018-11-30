try
{
	foreach($file in (Get-ChildItem -File -Recurse "$PSScriptRoot/shader" -Exclude *.spv))
    {
        Write-Host "Compiling shader $file"
        & "$env:VULKAN_SDK/bin/glslangValidator" -V $file.FullName -o "$($file.Directory.FullName)\$($file.Name).spv" | Out-Null
    }
}
catch
{
    Write-Error $_
	exit 1
}

