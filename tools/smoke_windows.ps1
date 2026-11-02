$ErrorActionPreference = "Stop"
$binary = Join-Path $PSScriptRoot "..\build\HellBoost.exe"
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
