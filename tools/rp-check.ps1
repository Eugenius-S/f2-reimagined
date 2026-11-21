# The RP boot check over an isolated scratch RP copy (302).
#
# Every requested map is attempted independently: a snapshot that fails or
# times out is a failed row, not an aborted sweep, and later maps still run.
# result.json is always written after preflight: one row per map with
# pass/fail/timeout, the error and the artifact folder, plus the copied
# candidate's sha256. The exit code is nonzero when any row is not a pass.
# A row only passes when the sidecar reports the requested map AND the
# candidate executable's own hash: the sidecar's exe_sha256 is compared to
# the hash this script computed, never trusted as copied.
#
# The isolated test process rules stay: GameRoot must not be the main
# checkout's game folder, and the whole run lives under build/.
#
# -SelfTest drives the same row machinery against a fake snapshot script
# (pass, failure, timeout, wrong candidate hash) and asserts the rows and
# the aggregate without running any game.
param(
    [string]$GameRoot,
    [string]$BuildExecutable,
    [string[]]$Maps = @('artemple', 'kladwtwn', 'denres1', 'abbey', 'epamain1', 'slvcamp'),
    # The snapshot script to invoke; the self-test substitutes a fake.
    [string]$SnapshotScript,
    [switch]$SelfTest
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $SnapshotScript) { $SnapshotScript = Join-Path $PSScriptRoot 'dev-snapshot.ps1' }
if (-not $SelfTest -and (-not $GameRoot -or -not $BuildExecutable)) {
    throw 'GameRoot and BuildExecutable are required (or run -SelfTest).'
}

# The candidate's own hash: what every passing row must report (302).
function Get-FileSha256([string]$path) {
    return (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLower()
}

# One map attempt. Returns the row; never throws up to the sweep: a failure
# or timeout is this row's status, and the later maps still run.
function Invoke-MapRow([string]$map, [string]$candidate, [string]$candidateSha, [string]$gamePath,
    [string]$snapshotScript, [string]$resultsRoot, [string]$testDirectory, [string]$runId) {
    if ($map -notmatch '^[A-Za-z0-9_-]+$') {
        return [pscustomobject]@{ map = $map; status = 'fail'; error = "invalid map name"
            artifact = $null; exe_sha256 = $null }
    }
    $mapOutput = Join-Path $resultsRoot $map
    $relativeOutput = "build/rp-check-runs/$runId/$map"
    $rowError = $null
    $status = 'pass'
    $sidecarSha = $null
    $sidecarMap = $null
    try {
        $snapshotArgs = @(
            '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $snapshotScript,
            '-Map', $map, '-Classic', '-CaptureConsole',
            '-GameRoot', $gamePath,
            '-BuildExecutable', $candidate,
            '-TestDirectory', "$testDirectory-$map",
            '-OutputDirectory', $relativeOutput,
            '-TimeoutSeconds', '90', '-WaitMinutes', '0'
        )
        # Windows PowerShell can promote redirected native stderr under Stop.
        # Capture both streams without losing the child's exit code or message.
        $previousErrorAction = $ErrorActionPreference
        try {
            $ErrorActionPreference = 'Continue'
            $snapshotOutput = & powershell.exe @snapshotArgs 2>&1
            $snapshotExit = $LASTEXITCODE
        } finally {
            $ErrorActionPreference = $previousErrorAction
        }
        if ($snapshotExit -ne 0) {
            $reason = $snapshotOutput | Where-Object { $_ -is [System.Management.Automation.ErrorRecord] -and "$_".Trim() } | Select-Object -First 1
            if (-not $reason) {
                $reason = $snapshotOutput | Where-Object { "$_".Trim() } | Select-Object -Last 1
            }
            if (-not $reason) { $reason = "snapshot exited with code $snapshotExit" }
            throw "$reason"
        }

        $sidecars = @(Get-ChildItem -LiteralPath $mapOutput -Filter 'devshot-*.json' -File -ErrorAction SilentlyContinue)
        if ($sidecars.Count -ne 1) { throw "expected one snapshot sidecar; found $($sidecars.Count)" }
        $metadata = Get-Content -Raw -LiteralPath $sidecars[0].FullName | ConvertFrom-Json
        $sidecarSha = [string]$metadata.exe_sha256
        $sidecarMap = [string]$metadata.map
        if ($sidecarMap -ine "$($map.ToUpperInvariant()).MAP") {
            throw "requested '$map' but the loaded snapshot reports '$sidecarMap'"
        }
        if (-not $sidecarSha -or $sidecarSha.ToLower() -ne $candidateSha) {
            throw "the snapshot reports a different executable than the candidate ($candidateSha)"
        }
    } catch {
        $message = $_.Exception.Message -replace '\r?\n', ' '
        if ($message.Length -gt 300) { $message = $message.Substring(0, 300) }
        # A watchdog or snapshot timeout is its own status: the machine was
        # fine, the map hung.
        $status = if ($message -match 'No snapshot|timeout|watchdog') { 'timeout' } else { 'fail' }
        $rowError = $message
    }
    return [pscustomobject]@{
        map = $map
        status = $status
        error = $rowError
        artifact = $relativeOutput
        exe_sha256 = $sidecarSha
    }
}

if ($SelfTest) {
    $failed = $false
    function Assert-True([string]$what, [bool]$got) {
        if (-not $got) {
            Write-Output ("selftest: FAIL {0}" -f $what)
            $script:failed = $true
        }
    }

    # A scratch game root and a fake snapshot script whose behavior is keyed
    # on the map name: pass, failure, timeout, wrong candidate hash.
    $temp = Join-Path ([System.IO.Path]::GetTempPath()) ("f2r-rp-check-selftest-" + [guid]::NewGuid().ToString('N'))
    $fakeRoot = Join-Path $temp 'game'
    New-Item -ItemType Directory -Path (Join-Path $fakeRoot 'mods') -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $fakeRoot 'ddraw.ini') -Value '[rp]' -Encoding ascii
    Set-Content -LiteralPath (Join-Path $fakeRoot 'mods\rpu.dat') -Value 'rpu' -Encoding ascii
    $fakeExe = Join-Path $temp 'candidate.exe'
    Set-Content -LiteralPath $fakeExe -Value 'candidate-bytes' -Encoding ascii
    $candidateSha = Get-FileSha256 $fakeExe
    $fakeScript = Join-Path $temp 'fake-snapshot.ps1'
    # The fake resolves a relative -OutputDirectory against the selftest's
    # temp root, the way dev-snapshot resolves one against the repository.
    $fakeBody = @"
param([string]`$Map, [switch]`$Classic, [switch]`$CaptureConsole, [string]`$GameRoot,
    [string]`$BuildExecutable, [string]`$TestDirectory, [string]`$OutputDirectory,
    [int]`$TimeoutSeconds, [int]`$WaitMinutes)
`$base = '$temp'
if (![IO.Path]::IsPathRooted(`$OutputDirectory)) { `$OutputDirectory = Join-Path `$base `$OutputDirectory }
`$sha = (Get-FileHash -LiteralPath `$BuildExecutable -Algorithm SHA256).Hash.ToLower()
New-Item -ItemType Directory -Path `$OutputDirectory -Force | Out-Null
if (`$Map -eq 'broken') { Write-Output 'the fake snapshot exploded'; exit 1 }
if (`$Map -eq 'hung') { Write-Output 'starting snapshot'; throw 'No snapshot within 90 s.' }
`$reported = if (`$Map -eq 'wrongexe') { 'deadbeef' } else { `$sha }
@{ map = `$Map.ToUpperInvariant() + '.MAP'; exe_sha256 = `$reported } |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path `$OutputDirectory 'devshot-0001.json') -Encoding utf8
exit 0
"@
    Set-Content -LiteralPath $fakeScript -Value $fakeBody -Encoding utf8

    # Drive the real row machinery through the same entry the sweep uses.
    $maps = @('artemple', 'broken', 'hung', 'kladwtwn', 'wrongexe')
    # The results root mirrors the real sweep's <root>/build/... layout: the
    # fake resolves a relative output against its base, so the rows look in
    # base/build/rp-check-runs/selftest/<map>.
    $resultsRoot = Join-Path $temp 'build\rp-check-runs\selftest'
    $rows = [System.Collections.Generic.List[object]]::new()
    foreach ($map in $maps) {
        $rows.Add((Invoke-MapRow $map $fakeExe $candidateSha $fakeRoot $fakeScript $resultsRoot 'fake-test' 'selftest'))
    }
    Assert-True 'every requested map has a row' ($rows.Count -eq 5)
    Assert-True 'the pass rows passed' (($rows | Where-Object { $_.map -in 'artemple', 'kladwtwn' } | Where-Object { $_.status -eq 'pass' }).Count -eq 2)
    Assert-True 'the failure is a failed row' (($rows | Where-Object map -eq 'broken').status -eq 'fail')
    Assert-True 'the hang is a timeout row' (($rows | Where-Object map -eq 'hung').status -eq 'timeout')
    Assert-True 'the hang preserves the child stderr reason' (($rows | Where-Object map -eq 'hung').error -match '^No snapshot within 90 s\.')
    Assert-True 'the wrong hash is a failed row' (($rows | Where-Object map -eq 'wrongexe').status -eq 'fail')
    Assert-True 'a failed map did not omit the later rows' (@($rows | Where-Object { $_.map -eq 'kladwtwn' -and $_.status -eq 'pass' }).Count -eq 1)
    $aggregateFail = ($rows | Where-Object status -ne 'pass').Count -gt 0
    Assert-True 'the aggregate fails on any non-pass row' $aggregateFail
    Assert-True 'the pass rows report the candidate hash' (@($rows | Where-Object status -eq 'pass' | Where-Object { $_.exe_sha256 -eq $candidateSha }).Count -eq 2)

    Remove-Item -LiteralPath $temp -Recurse -Force
    if ($failed) { exit 1 }
    Write-Output 'selftest: ok'
    exit 0
}

$gamePath = [System.IO.Path]::GetFullPath($GameRoot)
$buildPath = if ([System.IO.Path]::IsPathRooted($BuildExecutable)) {
    [System.IO.Path]::GetFullPath($BuildExecutable)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $root $BuildExecutable))
}
$mainWorktree = (git -C $root worktree list --porcelain | Select-Object -First 1) -replace '^worktree ', ''
$ownerGamePath = [System.IO.Path]::GetFullPath((Join-Path $mainWorktree 'game'))
if ($gamePath.TrimEnd('\') -ieq $ownerGamePath.TrimEnd('\')) {
    throw 'GameRoot must be an isolated scratch copy, not the main checkout game folder.'
}
if (-not (Test-Path -LiteralPath $gamePath -PathType Container)) { throw "No game root: $gamePath" }
if (-not (Test-Path -LiteralPath (Join-Path $gamePath 'ddraw.ini') -PathType Leaf)) {
    throw 'GameRoot has no ddraw.ini; this does not look like an installed RP copy.'
}
if (-not (Test-Path -LiteralPath (Join-Path $gamePath 'mods\rpu.dat') -PathType Leaf)) {
    throw 'GameRoot has no mods/rpu.dat; this does not look like an installed RP copy.'
}
if (-not (Test-Path -LiteralPath $buildPath -PathType Leaf)) { throw "No CE executable: $buildPath" }
if ($Maps.Count -eq 0) { throw 'Specify at least one map.' }

$runId = [guid]::NewGuid().ToString('N')
$relativeCandidate = "build/rp-check-candidates/$runId/fallout2-ce.exe"
$candidate = Join-Path $root ($relativeCandidate -replace '/', '\')
$testDirectory = "build/rp-check-test/$runId"
$resultsRoot = Join-Path $root ("build/rp-check-runs/$runId" -replace '/', '\')
New-Item -ItemType Directory -Path (Split-Path -Parent $candidate), $resultsRoot -Force | Out-Null
Copy-Item -LiteralPath $buildPath -Destination $candidate
$candidateSha = Get-FileSha256 $candidate

# After preflight every map runs on its own; result.json is written no
# matter how many rows fail (302).
$results = [System.Collections.Generic.List[object]]::new()
foreach ($map in $Maps) {
    $results.Add((Invoke-MapRow $map $relativeCandidate $candidateSha $gamePath $SnapshotScript $resultsRoot $testDirectory $runId))
}

$nonPass = @($results | Where-Object { $_.status -ne 'pass' })
$summary = [pscustomobject]@{
    result = if ($nonPass.Count -gt 0) { 'fail' } else { 'pass' }
    candidate_sha256 = $candidateSha
    maps = @($results)
}
$summary | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $resultsRoot 'result.json') -Encoding utf8
foreach ($row in $nonPass) {
    Write-Output ("RP_CHECK {0} {1}: {2}" -f $row.status.ToUpper(), $row.map, $row.error)
}
Write-Output ("RP_CHECK {0} maps={1} failed={2} output=build/rp-check-runs/{3}" -f
    $summary.result.ToUpper(), $results.Count, $nonPass.Count, $runId)
if ($nonPass.Count -gt 0) { exit 1 }
