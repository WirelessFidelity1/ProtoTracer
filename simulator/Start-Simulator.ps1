param([switch]$Build, [switch]$SelfTest)
$ErrorActionPreference = 'Stop'
$simulatorRoot = $PSScriptRoot
$simulatorExe = Join-Path $simulatorRoot 'build/ProtoTracerSimulator.exe'
$simulatorNeedsBuild = $Build -or -not (Test-Path -LiteralPath $simulatorExe)
if (-not $simulatorNeedsBuild) {
    $simulatorBuildTime = (Get-Item -LiteralPath $simulatorExe).LastWriteTimeUtc
    $simulatorSourceRoots = @((Join-Path $simulatorRoot '../lib/ProtoTracer'), (Join-Path $simulatorRoot 'compat'))
    $simulatorChangedSources = @(Get-ChildItem -LiteralPath $simulatorSourceRoots -Recurse -File |
        Where-Object { $_.Extension -in '.h', '.hpp', '.cpp', '.c' -and $_.LastWriteTimeUtc -gt $simulatorBuildTime })
    $simulatorChangedSources += @(Get-ChildItem -LiteralPath $simulatorRoot -File |
        Where-Object { $_.Extension -in '.cpp', '.h', '.py' -and $_.LastWriteTimeUtc -gt $simulatorBuildTime })
    $simulatorNeedsBuild = $simulatorChangedSources.Count -gt 0
}
if ($simulatorNeedsBuild) {
    $simulatorRunning = Get-Process ProtoTracerSimulator -ErrorAction SilentlyContinue
    foreach ($simulatorInstance in $simulatorRunning) {
        if ($simulatorInstance.Path -eq $simulatorExe) {
            if (-not $simulatorInstance.CloseMainWindow() -or -not $simulatorInstance.WaitForExit(5000)) {
                throw 'Close the simulator window, then launch again to rebuild the updated face.'
            }
        }
    }
    $simulatorPython = Join-Path $env:USERPROFILE '.platformio/penv/Scripts/python.exe'
    if (-not (Test-Path -LiteralPath $simulatorPython)) {
        $simulatorPython = (Get-Command python -ErrorAction Stop).Source
    }
    & $simulatorPython -c "import importlib.util, sys; sys.exit(0 if importlib.util.find_spec('ziglang') else 1)"
    if ($LASTEXITCODE -ne 0) {
        & $simulatorPython -m pip install ziglang==0.16.0
        if ($LASTEXITCODE -ne 0) { throw 'Could not install the simulator compiler.' }
    }
    & $simulatorPython (Join-Path $simulatorRoot 'build.py')
    if ($LASTEXITCODE -ne 0) { throw 'Simulator build failed.' }
}
if ($SelfTest) {
    $simulatorProcess = Start-Process -FilePath $simulatorExe -ArgumentList '--self-test' -WorkingDirectory (Join-Path $simulatorRoot 'build') -WindowStyle Hidden -Wait -PassThru
    Get-Content (Join-Path $simulatorRoot 'build/self-test.txt')
    exit $simulatorProcess.ExitCode
}
Start-Process -FilePath $simulatorExe -WorkingDirectory (Join-Path $simulatorRoot 'build')
