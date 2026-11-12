# One command against the owner's speed bar (251).
#
# Runs the background test game (tools/dev-snapshot.ps1, the only allowed
# game launcher) on kladwtwn, newr1, vctyctyd and the klatoxcv cave with a
# warm scene cache (a warmup run, then the measured run), the frame log on
# and F2CE_PERF=1, and prints one table: map, average FPS, 1% low FPS,
# worst frame in ms, load time to the first 3D frame in ms. Exits non-zero
# when a map's 1% low is under -MinFps or its load is over -MaxLoadMs; the
# failing cell is named. A background run is paced differently from a real
# window (157's Log): the thresholds guard against a regression, they do
# not prove the bar. Everything it writes goes under build/.
#
# -SelfTest feeds saved samples to the parsing functions and fails on a
# wrong answer without running the game.
param(
    [double]$MinFps = 60,
    [double]$MaxLoadMs = 2000,
    # Seconds the game stays on the measured map after its snapshot: the sample.
    [int]$HoldSeconds = 10,
    [switch]$SelfTest
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot

# frame_log.csv rows ("frame,ms,load") to the play-only numbers (250: load
# rows are loading-screen presents and stay out of the frame rate). 157: only
# the measured map counts. A run starts in the menu and on the start map, and
# those rows were most of the sample; so the play rows before the last load
# row are dropped, and so is the first second after it (the 3D scene is made
# on the first frames; the load time column covers that).
function Read-FrameLogSummary([string[]]$lines) {
    $play = @()
    $load = 0
    $settleMs = 0.0
    foreach ($line in $lines) {
        if ($line -match '^frame,') { continue }
        $parts = $line -split ','
        if ($parts.Count -lt 3) { continue }
        $ms = 0.0
        if (-not [double]::TryParse($parts[1], [Globalization.NumberStyles]::Float,
                [Globalization.CultureInfo]::InvariantCulture, [ref]$ms)) { continue }
        if ($parts[2].Trim() -eq '1') { $load++; $play = @(); $settleMs = 0.0 }
        elseif ($load -gt 0 -and $settleMs -lt 1000.0) { $settleMs += $ms }
        else { $play += $ms }
    }
    $result = @{ Play = $play.Count; Load = $load; AvgFps = 0.0; OneLowFps = 0.0; WorstMs = 0.0 }
    if ($play.Count -eq 0) { return $result }
    $sorted = $play | Sort-Object
    $total = ($sorted | Measure-Object -Sum).Sum
    $result.AvgFps = $sorted.Count * 1000.0 / $total
    $index = [Math]::Min([int]($sorted.Count * 99 / 100), $sorted.Count - 1)
    $oneLow = $sorted[$index]
    $result.OneLowFps = 1000.0 / $oneLow
    $result.WorstMs = $sorted[$sorted.Count - 1]
    return $result
}

# The load time to the first 3D frame: the last "first_frame" mark of the
# perf load chain (172), written to the run's logs/devperf.log under
# F2CE_PERF=1.
function Read-LoadMs([string[]]$lines) {
    $ms = $null
    foreach ($line in $lines) {
        if ($line -match 'perf load first_frame ms=([0-9.]+) since chain start') {
            $ms = [double]::Parse($Matches[1], [Globalization.CultureInfo]::InvariantCulture)
        }
    }
    return $ms
}

if ($SelfTest) {
    $sampleCsv = @(
        'frame,ms,load',
        '0,0.000,0',
        '1,16.500,0',
        '2,16.500,0',
        '3,100.000,0',
        '4,33.000,1',
        '5,2.000,1',
        '6,900.000,0',
        '7,150.000,0',
        '8,10.000,0',
        '9,40.000,0'
    )
    $s = Read-FrameLogSummary $sampleCsv
    $failed = $false
    function Assert-Equal([string]$what, $got, $want) {
        if ([Math]::Abs([double]$got - [double]$want) -gt 0.001) {
            Write-Output ("selftest: FAIL {0} got {1} want {2}" -f $what, $got, $want)
            $script:failed = $true
        }
    }
    # Rows 0-3 are before the last load row; 6 and 7 fill its first second.
    Assert-Equal 'play' $s.Play 2
    Assert-Equal 'load' $s.Load 2
    Assert-Equal 'avgFps' $s.AvgFps (2 * 1000.0 / 50.0)
    Assert-Equal 'oneLowFps' $s.OneLowFps 25.0
    Assert-Equal 'worstMs' $s.WorstMs 40.0
    # A log with no load row (the log was switched on in play) counts whole.
    $s = Read-FrameLogSummary @('frame,ms,load', '0,0.000,0', '1,20.000,0')
    Assert-Equal 'play without a load row' $s.Play 2
    $sampleLog = @(
        'perf load chain map_change begin',
        'perf load map_tiles ms=12.00 since chain start',
        'perf load first_frame ms=812.34 since chain start',
        'perf load chain map_change begin',
        'perf load first_frame ms=900.10 since chain start'
    )
    $load = Read-LoadMs $sampleLog
    Assert-Equal 'loadMs (last chain wins)' $load 900.10
    if ($failed) { exit 1 }
    Write-Output 'selftest: ok'
    exit 0
}

$maps = @(
    @{ map = 'kladwtwn'; elevation = 0 },
    @{ map = 'newr1'; elevation = 0 },
    @{ map = 'vctyctyd'; elevation = 0 },
    @{ map = 'klatoxcv'; elevation = 2 }
)

$outRoot = Join-Path $Root 'build\perf-check'
New-Item -ItemType Directory -Path $outRoot -Force | Out-Null

$rows = @()
$failures = @()
foreach ($m in $maps) {
    $label = ('{0} e{1}' -f $m.map, $m.elevation)
    $outDir = Join-Path $outRoot ('{0}-e{1}' -f $m.map, $m.elevation)
    # Warmup: the exporter cache and the art fills land on disk here.
    & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'dev-snapshot.ps1') `
        -Map $m.map -Elevation $m.elevation -Set3d 'frame_log=1' `
        -OutputDirectory (Join-Path $outDir 'warmup') -TimeoutSeconds 240 | Out-Null
    # Measured run: F2CE_PERF=1 makes the load chain print; the frame log
    # fills the CSV this pass parses. -KeepRunFolder keeps the run folder
    # (dev-snapshot removes it once the -OutputDirectory copies are done),
    # and its "RUN FOLDER:" line names where frame_log.csv landed.
    $env:F2CE_PERF = '1'
    $measuredOutput = & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'dev-snapshot.ps1') `
        -Map $m.map -Elevation $m.elevation -Set3d 'frame_log=1' -CaptureConsole -KeepRunFolder `
        -OutputDirectory (Join-Path $outDir 'measured') -TimeoutSeconds 240 -HoldSeconds $HoldSeconds
    $env:F2CE_PERF = $null
    $runFolderLine = @($measuredOutput) | Where-Object { $_ -like 'RUN FOLDER: *' } | Select-Object -Last 1
    $csv = $null
    $perfLog = $null
    if ($runFolderLine) {
        $runFolder = $runFolderLine.Substring('RUN FOLDER: '.Length).Trim()
        $csvPath = Join-Path $runFolder 'frame_log.csv'
        if (Test-Path -LiteralPath $csvPath) { $csv = Get-Item -LiteralPath $csvPath }
        # The load chain prints to the run's devperf.log (F2CE_PERF=1), not
        # to the captured stdout.
        $perfLogPath = Join-Path $runFolder 'logs\devperf.log'
        if (Test-Path -LiteralPath $perfLogPath) { $perfLog = Get-Content -LiteralPath $perfLogPath }
    }
    if ($null -eq $csv -or $null -eq $perfLog) {
        $failures += "{0}: no frame_log.csv or logs/devperf.log in the kept run folder of the measured run" -f $label
        continue
    }
    $summary = Read-FrameLogSummary @(Get-Content -LiteralPath $csv.FullName)
    $loadMs = Read-LoadMs @($perfLog)
    $rows += [pscustomobject]@{
        map = $label
        avgFps = [Math]::Round($summary.AvgFps, 1)
        oneLowFps = [Math]::Round($summary.OneLowFps, 1)
        worstMs = [Math]::Round($summary.WorstMs, 1)
        loadMs = if ($null -ne $loadMs) { [Math]::Round($loadMs, 1) } else { -1 }
        play = $summary.Play
        load = $summary.Load
    }
    if ($summary.OneLowFps -lt $MinFps) {
        $failures += "{0}: 1% low {1} fps is under {2}" -f $label, [Math]::Round($summary.OneLowFps, 1), $MinFps
    }
    if ($null -eq $loadMs) {
        $failures += "{0}: no 'perf load first_frame' line in the measured run" -f $label
    } elseif ($loadMs -gt $MaxLoadMs) {
        $failures += "{0}: load {1} ms is over {2}" -f $label, [Math]::Round($loadMs, 1), $MaxLoadMs
    }
}

$rows | Format-Table -AutoSize map, avgFps, oneLowFps, worstMs, loadMs, play, load

if ($failures.Count -gt 0) {
    foreach ($f in $failures) { Write-Output "FAIL $f" }
    exit 1
}
Write-Output ("perf-check: PASS all {0} maps (1% low >= {1} fps, load <= {2} ms)" -f $maps.Count, $MinFps, $MaxLoadMs)
