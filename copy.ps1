& $PSScriptRoot/build.ps1
if ($?) {
    adb push build/libqubes.so /sdcard/Android/data/com.beatgames.beatsaber/files/mods/libqubes.so
    if ($?) {
        adb shell am force-stop com.beatgames.beatsaber
        adb shell am start com.beatgames.beatsaber/com.unity3d.player.UnityPlayerActivity
        if ($args[0] -eq "--log") {
            $timestamp = Get-Date -Format "MM-dd HH:mm:ss.fff"
            adb logcat -c
            adb logcat -T "$timestamp" main-modloader:W QuestHook[Qubes`|v0.1.0]:* QuestHook[UtilsLogger`|v1.0.12]:* AndroidRuntime:E *:S
        }
    }
}
