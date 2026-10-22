$ErrorActionPreference = 'Stop'
$editorCandidates = @('py', 'python', (Join-Path $env:USERPROFILE '.platformio/penv/Scripts/python.exe'))
$editorPython = $null
foreach ($candidate in $editorCandidates) {
    $command = Get-Command $candidate -ErrorAction SilentlyContinue
    if ($command) {
        & $command.Source -c "import tkinter" 2>$null
        if ($LASTEXITCODE -eq 0) {
            $editorPython = $command.Source
            break
        }
    }
}
if (-not $editorPython) { throw 'Install Python 3 with Tcl/Tk support, then try again.' }
& $editorPython (Join-Path $PSScriptRoot 'face_editor.py')
exit $LASTEXITCODE
