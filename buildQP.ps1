& $PSScriptRoot/build.ps1

$ArchiveName = "Qubes.qmod"
$TempArchiveName = "Qubes.qmod.zip"

Compress-Archive -Path "./libs/arm64-v8a/libqubes.so", ".\extern\libbeatsaber-hook_2_3_2.so", ".\mod.json", ".\icons\*" -DestinationPath $TempArchiveName -Force
Move-Item $TempArchiveName $ArchiveName -Force