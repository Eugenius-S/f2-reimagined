# Self-test of snapshot-output.ps1 (task 301): fake dev-snapshot runs, each its
# own PowerShell process like the real one, reuse one -OutputDirectory.
$ErrorActionPreference = 'Stop'
$tempParent = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$testRoot = [IO.Path]::GetFullPath((Join-Path $tempParent ('f2r-snapshot-output-test-' + [guid]::NewGuid())))
if (-not $testRoot.StartsWith($tempParent, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe test root' }
New-Item -ItemType Directory -Path $testRoot | Out-Null
$out = Join-Path $testRoot 'out'
$fakeRun = Join-Path $testRoot 'fake-run.ps1'
$script:checks = 0

function Check([bool]$ok, [string]$message) {
    if (-not $ok) { throw $message }
    $script:checks++
}
# A dev-snapshot stand-in: the same calls in the same order, captures written
# where Copy-Item would put them; -FailAt stops it like a failed check.
Set-Content -LiteralPath $fakeRun -Encoding utf8 -Value @"
param([string]`$Out, [string]`$Run, [int]`$Count, [int]`$FailAt = -1)
`$ErrorActionPreference = 'Stop'
function Get-FileHash { throw 'Get-FileHash is absent in the tour host' }
. '$(Join-Path $PSScriptRoot 'snapshot-output.ps1')'
Reset-SnapshotOutput `$Out
for (`$i = 0; `$i -lt `$Count; `$i++) {
    if (`$i -eq `$FailAt) { throw 'the fake game stopped' }
    Set-Content -LiteralPath (Join-Path `$Out "devshot-`$Run-`$i.json") -Value ('{"image":"devshot-' + `$Run + '-' + `$i + '.png"}')
    Add-SnapshotOutput `$Out "devshot-`$Run-`$i.json"
    Set-Content -LiteralPath (Join-Path `$Out "devshot-`$Run-`$i.png") -Value "png `$Run `$i"
    Add-SnapshotOutput `$Out "devshot-`$Run-`$i.png"
}
Complete-SnapshotOutput `$Out
"@
function Run([string]$Name, [int]$Count, [int]$FailAt = -1) {
    # A failed run writes to stderr; under Stop, Windows PowerShell would turn that into this test's own error.
    $ErrorActionPreference = 'Continue'
    & powershell -NoProfile -ExecutionPolicy Bypass -File $fakeRun -Out $out -Run $Name -Count $Count -FailAt $FailAt 2>&1 | Out-Null
    return $LASTEXITCODE
}
function Manifest {
    $raw = [IO.File]::ReadAllBytes((Join-Path $out 'snapshot-output.json'))
    Check ($raw[0] -ne 0xEF) 'the manifest starts with a BOM'
    $data = [Text.Encoding]::UTF8.GetString($raw) | ConvertFrom-Json
    return @{ state = $data.state; files = @($data.files.PSObject.Properties.Name | Sort-Object) }
}
function Captures { return @(Get-ChildItem -LiteralPath $out -Filter 'devshot-*' -File | ForEach-Object Name | Sort-Object) }
function Pair([string]$Run, [int]$Count) {
    return @(0..($Count - 1) | ForEach-Object { "devshot-$Run-$_.json"; "devshot-$Run-$_.png" } | Sort-Object)
}
function Same($a, $b) { return (($a -join '|') -eq ($b -join '|')) }

try {
    New-Item -ItemType Directory -Path $out | Out-Null
    # Not the script's: a report, a note, an older capture from before the manifest.
    Set-Content -LiteralPath (Join-Path $out 'notes.txt') -Value 'mine'
    Set-Content -LiteralPath (Join-Path $out 'report.json') -Value '{"mine": true}'
    Set-Content -LiteralPath (Join-Path $out 'devshot-legacy.json') -Value '{"image":"devshot-legacy.png"}'
    $unrelated = @('devshot-legacy.json', 'notes.txt', 'report.json')
    function Check-Unrelated {
        foreach ($name in $unrelated) { Check (Test-Path -LiteralPath (Join-Path $out $name)) "$name was deleted" }
    }

    Check ((Run 'a' 3) -eq 0) 'run a failed'
    $m = Manifest
    Check ($m.state -eq 'complete') 'run a is not complete'
    Check (Same $m.files (Pair 'a' 3)) "run a lists $($m.files -join ',')"
    Check (Same (Captures) (@((Pair 'a' 3) + 'devshot-legacy.json') | Sort-Object)) "after a: $((Captures) -join ',')"
    Check-Unrelated

    # The owner edits one of run a's images: changed since, it is no longer provably the script's.
    Set-Content -LiteralPath (Join-Path $out 'devshot-a-2.png') -Value 'edited by hand'
    Check ((Run 'b' 1) -eq 0) 'run b failed'
    $m = Manifest
    Check ($m.state -eq 'complete') 'run b is not complete'
    Check (Same $m.files (Pair 'b' 1)) "run b lists $($m.files -join ',')"
    Check (Same (Captures) (@((Pair 'b' 1) + 'devshot-a-2.png' + 'devshot-legacy.json') | Sort-Object)) "after b: $((Captures) -join ',')"
    Check ((Get-Content -Raw -LiteralPath (Join-Path $out 'devshot-a-2.png')).Trim() -eq 'edited by hand') 'the edited image changed'
    Check-Unrelated

    # Run c stops after its first capture: nonzero, incomplete, only its own file listed, run b's gone.
    Check ((Run 'c' 3 1) -ne 0) 'a failed run exited 0'
    $m = Manifest
    Check ($m.state -eq 'incomplete') "a failed run says $($m.state)"
    Check (Same $m.files (Pair 'c' 1)) "run c lists $($m.files -join ',')"
    Check (-not (Test-Path -LiteralPath (Join-Path $out 'devshot-b-0.json'))) 'run b survived a failed run'
    Check-Unrelated

    # A run that stops before any capture still says incomplete, with nothing listed.
    Check ((Run 'd' 2 0) -ne 0) 'a failed run exited 0'
    $m = Manifest
    Check ($m.state -eq 'incomplete' -and $m.files.Count -eq 0) 'run d did not say incomplete with no files'
    Check (-not (Test-Path -LiteralPath (Join-Path $out 'devshot-c-0.json'))) "run c's partial capture survived"

    # A listed name outside the folder is never deleted, even with a matching hash.
    $outside = Join-Path $testRoot 'outside.txt'
    Set-Content -LiteralPath $outside -Value 'outside'
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $outside).Hash
    [IO.File]::WriteAllText((Join-Path $out 'snapshot-output.json'), '{"state":"complete","files":{"..\\outside.txt":"' + $hash + '"}}')
    Check ((Run 'e' 1) -eq 0) 'run e failed'
    Check (Test-Path -LiteralPath $outside) 'a file outside the folder was deleted'

    # A damaged manifest stops the run before anything is deleted.
    Set-Content -LiteralPath (Join-Path $out 'snapshot-output.json') -Value '{ damaged'
    Check ((Run 'f' 1) -ne 0) 'a damaged manifest was ignored'
    Check (Test-Path -LiteralPath (Join-Path $out 'devshot-e-0.json')) 'a damaged manifest still deleted captures'
    Check-Unrelated

    # dev-snapshot.ps1 itself needs the game; check that it makes these calls where the fake run does.
    $source = Get-Content -Raw -LiteralPath (Join-Path $PSScriptRoot 'dev-snapshot.ps1')
    $reset = $source.IndexOf('if ($OutputDirectory) { Reset-SnapshotOutput $OutputDirectory }')
    Check ($reset -gt 0 -and $reset -lt $source.IndexOf('throw ')) 'dev-snapshot does not reset the folder before its first check'
    Check ($source -match "(?m)^function Complete-RunFolder \{\r?\n    if \(\`$OutputDirectory\) \{ Complete-SnapshotOutput") 'Complete-RunFolder does not complete the folder first'
    $copies = [regex]::Matches($source, 'Copy-Item[^\r\n]*(\$OutputDirectory[^\r\n]*)?\r?\n\s*Add-SnapshotOutput').Count
    Check ($copies -eq 3) "dev-snapshot records $copies of its 3 capture copies"

    Write-Host "snapshot-output: $script:checks checks passed"
    $global:LASTEXITCODE = 0
} finally {
    if (Test-Path -LiteralPath $testRoot) {
        if (-not $testRoot.StartsWith($tempParent, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe cleanup path' }
        Remove-Item -LiteralPath $testRoot -Recurse -Force
    }
}
