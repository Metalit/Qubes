$NDKPath = Get-Content ./ndkpath.txt

$stackScript = "$NDKPath/ndk-stack"
if (-not ($PSVersionTable.PSEdition -eq "Core")) {
    $stackScript += ".cmd"
}

Get-Content ./RecentCrash.log | & $stackScript -sym ./build/debug/ > UnstrippedCrash.log