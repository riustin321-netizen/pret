$ErrorActionPreference = "Stop"
$binary = Join-Path $PSScriptRoot "..\build\HellBoost.exe"
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class HellBoostUiProbe {
    [DllImport("user32.dll")]
    public static extern IntPtr GetDlgItem(IntPtr parent, int id);
    [DllImport("user32.dll")]
    public static extern IntPtr SendMessage(IntPtr target, uint message, IntPtr wparam, IntPtr lparam);
}
'@
$transport = Start-Process -FilePath $binary -ArgumentList "--check-transport" -PassThru -Wait
if ($transport.ExitCode -ne 0) {
    throw "WinHTTP TLS/redirect configuration check failed: $($transport.ExitCode)"
}
$process = Start-Process -FilePath $binary -PassThru
try {
    if (-not $process.WaitForInputIdle(10000)) {
        throw "HellBoost did not initialize its Windows message loop"
    }
    $process.Refresh()
    if ($process.HasExited -or $process.MainWindowHandle -eq [IntPtr]::Zero) {
        throw "HellBoost did not create a main window"
    }
    if ($process.MainWindowTitle -notlike "HellBoost*") {
        throw "Unexpected window title: $($process.MainWindowTitle)"
    }
    $steamCheckbox = [HellBoostUiProbe]::GetDlgItem($process.MainWindowHandle, 112)
    if ($steamCheckbox -eq [IntPtr]::Zero) {
        throw "Steam integration setting is missing"
    }
    $original = [HellBoostUiProbe]::SendMessage($steamCheckbox, 0x00F0, [IntPtr]::Zero, [IntPtr]::Zero)
    [void][HellBoostUiProbe]::SendMessage($steamCheckbox, 0x00F5, [IntPtr]::Zero, [IntPtr]::Zero)
    $changed = [HellBoostUiProbe]::SendMessage($steamCheckbox, 0x00F0, [IntPtr]::Zero, [IntPtr]::Zero)
    if ($changed -eq $original) {
        throw "Steam integration checkbox did not toggle"
    }
    [void][HellBoostUiProbe]::SendMessage($steamCheckbox, 0x00F1, $original, [IntPtr]::Zero)
    if (-not $process.CloseMainWindow()) {
        throw "HellBoost did not accept the close request"
    }
    if (-not $process.WaitForExit(10000)) {
        throw "HellBoost did not close cleanly"
    }
    if ($process.ExitCode -ne 0) {
        throw "HellBoost exited with code $($process.ExitCode)"
    }
    Write-Output "HellBoost native window opened and closed successfully; game launch was not tested."
} finally {
    if (-not $process.HasExited) {
        $process.Kill()
        $process.WaitForExit()
    }
}
