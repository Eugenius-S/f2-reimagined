# One command measures every playable map and elevation (271).
#
# Walks the export groups' map list (tools/map_health_maps.py, default: all
# 152 maps with squares in the game data), and for every map elevation runs
# the background test game twice like perf-check does: a warmup (the scene
# cache and the art fills land on disk) and a measured run (F2CE_PERF=1, the
# frame log on, -CaptureConsole). Each finished row lands in the state file
# as one JSON line, so an interrupted or timed-out sweep resumes where it
# stopped: rows already measured on the same build are kept, a map that hung
# is a TIMEOUT row (retried up to -MaxAttempts, then it says so in the table),
# and the row never pretends a timeout is a number.
#
# Every row holds: average FPS, 1% low FPS, worst frame ms, load ms to the
# first 3D frame, the CPU load at the start of the measurement, the share of
# black pixels (the table builder computes it from the row's snapshot PNG),
# and any 2D fallback line the run printed ("stays 2D" or "uses 3D ground
# plane" in the captured stdout). The run's devshot sidecar names the exact
# exe (exe_sha256); a resumed sweep only reuses rows whose exe matches the
# current one, and the table builder refuses rows that mix builds.
#
# 290: each row is measured under the machine-wide mutex
# Global\fallout2-ce-perf-check, held across that row's warmup and measured
# run (both burn CPU) and released between rows, so another agent's
# perf-check can slip in between maps but never overlaps a measurement.
# A row measured under more than -NoisyCpuPercent CPU load is marked noisy
# and is not a valid result; the sweep retries noisy rows once on the next
# run when the machine may be free.
#
# The table itself is a separate step: tools/map_health_table.py reads the
# state file and writes the sorted table (its parser has the CI self-test).
#
# Exit codes: 0 every row has a valid result, 1 the sweep is incomplete
# (rows still timing out or failing after -MaxAttempts), 2 rows are noisy
# (rerun on a free machine; the valid rows are still written). Never runs a
# build; build the Release exe first, the sweep refuses a missing one.
#
# -SelfTest feeds saved samples to the row-parsing functions and fails on a
# wrong answer without running the game.
param(
    # Export groups or single map names, comma-separated (map_health_maps.py).
    [string]$Groups = 'all',
    [double]$MaxLoadMs = 2000,
    # The owner's bar is an 80 fps 1% low; the gate stays at 60 (271).
    [double]$BarOneLowFps = 80,
    [double]$GateOneLowFps = 60,
    [int]$HoldSeconds = 10,
    # One warmup run may build the whole scene cache for its elevation
    # (minutes on a cold cache), so its timeout is generous.
    [int]$WarmupTimeoutSeconds = 900,
    [int]$MeasuredTimeoutSeconds = 300,
    [double]$NoisyCpuPercent = 30,
    [ValidateRange(0, 240)]
    [int]$WaitMinutes = 30,
    # A hung map is retried this many attempts in total, then the row says
    # TIMEOUT for good (no more than two repeats of one failed attempt).
    [ValidateRange(1, 5)]
    [int]$MaxAttempts = 2,
    [string]$StateFile,
    # Only the first N rows of the list (a smoke run).
    [ValidateRange(1, 500)]
    [int]$Limit = 0,
    # Only rows matching these "map:e" labels (a targeted rerun).
    [string[]]$Only,
    # Drop the state file and start from an empty table.
    [switch]$Fresh,
    [switch]$SelfTest
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot

# The play-only frame numbers, exactly perf-check's reading (250: load rows
# are loading-screen presents; 157: only the measured map counts; the first
# second after the last load row settles).
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
    $result.OneLowFps = 1000.0 / $sorted[$index]
    $result.WorstMs = $sorted[$sorted.Count - 1]
    return $result
}

function Read-LoadMs([string[]]$lines) {
    $ms = $null
    foreach ($line in $lines) {
        if ($line -match 'perf load first_frame ms=([0-9.]+) since chain start') {
            $ms = [double]::Parse($Matches[1], [Globalization.CultureInfo]::InvariantCulture)
        }
    }
    return $ms
}

# The 2D fallback lines a run printed (scene_view.cc writes them to stdout
# and logs/scene.log; -CaptureConsole keeps stdout as process.stdout.log).
function Read-FallbackLines([string[]]$lines) {
    return @($lines | Where-Object {
        $_ -match 'scene: .* stays 2D:' -or $_ -match 'scene: .* uses 3D ground plane:'
    })
}

# 290: the machine-wide measurement lock, shared with perf-check.
function Acquire-PerfLock([string]$name, [int]$minutes) {
    $mutex = New-Object System.Threading.Mutex($false, $name)
    $held = $false
    try { $held = $mutex.WaitOne([TimeSpan]::FromMinutes($minutes)) }
    catch [System.Threading.AbandonedMutexException] { $held = $true }
    if (-not $held) { $mutex.Dispose() }
    return @{ Mutex = $mutex; Held = $held }
}
function Release-Lock($lock) {
    if ($null -ne $lock -and $lock.Held) {
        $lock.Held = $false
        $lock.Mutex.ReleaseMutex()
        $lock.Mutex.Dispose()
    }
}

function Read-CpuLoadPercent {
    $samples = @()
    foreach ($i in 1..4) {
        $samples += (Get-CimInstance Win32_Processor |
            Measure-Object -Property LoadPercentage -Average).Average
        Start-Sleep -Milliseconds 250
    }
    $avg = ($samples | Where-Object { $null -ne $_ } | Measure-Object -Average).Average
    if ($null -eq $avg) { return $null }
    return [Math]::Round($avg, 0)
}

function Test-NoisyRow([double]$loadPercent, [double]$noisyAbove) {
    return $loadPercent -gt $noisyAbove
}

if ($SelfTest) {
    $failed = $false
    function Assert-Equal([string]$what, $got, $want) {
        if ([Math]::Abs([double]$got - [double]$want) -gt 0.001) {
            Write-Output ("selftest: FAIL {0} got {1} want {2}" -f $what, $got, $want)
            $script:failed = $true
        }
    }
    function Assert-True([string]$what, $got) {
        if (-not $got) {
            Write-Output ("selftest: FAIL {0}" -f $what)
            $script:failed = $true
        }
    }
    $s = Read-FrameLogSummary @(
        'frame,ms,load', '0,0.000,0', '1,16.500,0', '2,16.500,0', '3,100.000,0',
        '4,33.000,1', '5,2.000,1', '6,900.000,0', '7,150.000,0', '8,10.000,0', '9,40.000,0'
    )
    Assert-Equal 'play' $s.Play 2
    Assert-Equal 'avgFps' $s.AvgFps (2 * 1000.0 / 50.0)
    Assert-Equal 'oneLowFps' $s.OneLowFps 25.0
    $load = Read-LoadMs @(
        'perf load chain map_change begin',
        'perf load first_frame ms=812.34 since chain start',
        'perf load chain map_change begin',
        'perf load first_frame ms=900.10 since chain start'
    )
    Assert-Equal 'loadMs (last chain wins)' $load 900.10
    Assert-True 'loadMs empty stays empty' ($null -eq (Read-LoadMs @('perf load chain map_change begin')))
    $fb = Read-FallbackLines @(
        'some other line',
        'scene: KLATOXCV elevation 2 stays 2D: walls.bin is missing',
        'scene: denbus1 elevation 0 uses 3D ground plane: projection.bin sha mismatch'
    )
    Assert-Equal 'fallback lines' $fb.Count 2
    Assert-True 'stays 2D wording' ($fb[0] -like '*stays 2D*')
    Assert-True 'ground plane wording' ($fb[1] -like '*uses 3D ground plane*')
    Assert-True 'load 31 at 30 is noisy' (Test-NoisyRow 31 30)
    Assert-True 'load 30 at 30 is clean' (-not (Test-NoisyRow 30 30))
    if ($failed) { exit 1 }
    Write-Output 'selftest: ok'
    exit 0
}

# The sweep needs the freshly built test exe; it never builds one itself.
$buildExe = Join-Path $Root 'build\fallout2-ce.exe'
if (-not (Test-Path -LiteralPath $buildExe)) {
    throw "No build\fallout2-ce.exe in this worktree: build the Release exe first (tools/configure-build.ps1), then run the sweep."
}
$devSnapshot = Join-Path $PSScriptRoot 'dev-snapshot.ps1'
$mapListScript = Join-Path $PSScriptRoot 'map_health_maps.py'

if (-not $StateFile) { $StateFile = Join-Path $Root 'build\map-health\state.jsonl' }
$stateDir = Split-Path -Parent $StateFile
New-Item -ItemType Directory -Path $stateDir -Force | Out-Null
if ($Fresh -and (Test-Path -LiteralPath $StateFile)) { Remove-Item -LiteralPath $StateFile -Force }

$gameRoot = Join-Path $Root 'game'
$listAll = & py -3 $mapListScript --game-root $gameRoot --groups $Groups
if ($LASTEXITCODE -ne 0) { throw "map_health_maps.py failed (exit $LASTEXITCODE); see its output above." }
$list = ($listAll | Select-Object -First 1) | ConvertFrom-Json
$targets = @($list.maps)
if ($Only.Count -gt 0) {
    $wanted = @{}
    foreach ($o in $Only) { $wanted[$o] = $true }
    $targets = @($targets | Where-Object { $wanted.ContainsKey(('{0}:e{1}' -f $_.map, $_.elevation)) })
}
if ($Limit -gt 0 -and $targets.Count -gt $Limit) { $targets = @($targets | Select-Object -First $Limit) }
if ($targets.Count -eq 0) { throw 'The map list is empty: check -Groups against the game data.' }

# Resume: keep finished rows whose exe is the current one; a row retried in
# this run replaces its own previous line, never another row's.
$exeHash = (Get-FileHash -LiteralPath $buildExe -Algorithm SHA256).Hash.ToLower()
$rows = [ordered]@{}
$noisyKept = @()
if (Test-Path -LiteralPath $StateFile) {
    foreach ($line in @(Get-Content -LiteralPath $StateFile -ErrorAction SilentlyContinue)) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        try { $row = $line | ConvertFrom-Json } catch { continue }
        $key = '{0}:e{1}' -f $row.map, $row.elevation
        if ($row.exe_sha256 -ne $exeHash) {
            $noisyKept += "$key was measured on build $($row.exe_sha256), this sweep rebuilds it from scratch"
            continue
        }
        $rows[$key] = $row
    }
}
if ($noisyKept.Count -gt 0) {
    Write-Output ("map-health: {0} state rows belong to another build and start fresh: {1}" -f $noisyKept.Count, ($noisyKept | Select-Object -First 3) -join '; ')
}

function Save-Row($row) {
    # One JSON line per row, appended: the state file is the resume journal.
    $json = $row | ConvertTo-Json -Compress -Depth 4
    Add-Content -LiteralPath $StateFile -Value $json -Encoding utf8
}

# Pending: rows with no result, rows that failed or timed out under the
# attempt cap, and noisy rows (not valid results) until their cap. An ok,
# clean row is never measured twice on the same build.
$pending = @()
foreach ($t in $targets) {
    $key = '{0}:e{1}' -f $t.map, $t.elevation
    $row = $rows[$key]
    if ($null -eq $row) { $pending += $t; continue }
    if ($row.status -eq 'ok' -and -not $row.noisy) { continue }
    if ([int]$row.attempts -ge $MaxAttempts -and -not ($row.status -eq 'ok' -and $row.noisy)) { continue }
    if ($row.status -eq 'ok' -and $row.noisy -and [int]$row.attempts -ge ($MaxAttempts + 1)) { continue }
    $pending += $t
}

$total = $targets.Count
$doneCount = $total - $pending.Count
Write-Output ("map-health: {0} map elevations, {1} already measured, {2} to run (build {3})" -f $total, $doneCount, $pending.Count, $exeHash.Substring(0, 12))
foreach ($m in $list.missing) { Write-Output "map-health: map missing from the game data, not playable: $m" }

$incomplete = @()
$noisyRows = @()
foreach ($t in $pending) {
    $map = $t.map
    $elevation = [int]$t.elevation
    $label = '{0}:e{1}' -f $map, $elevation
    $outDir = Join-Path $stateDir ($map + '-e' + $elevation)
    $prior = $rows[$label]
    $attempts = if ($null -ne $prior) { [int]$prior.attempts } else { 0 }
    $attempts++
    $row = [ordered]@{
        map = $map; elevation = $elevation; exe_sha256 = $exeHash; attempts = $attempts
        status = 'failed'; started = (Get-Date).ToString('o')
        avgFps = $null; oneLowFps = $null; worstMs = $null; loadMs = $null
        cpu = $null; noisy = $false; play = 0; loads = 0
        fallback = @(); snapshot = $null; scene_3d_enabled = $null; renderer = $null
        breach = ''
    }
    $lock = $null
    try {
        $lock = Acquire-PerfLock 'Global\fallout2-ce-perf-check' $WaitMinutes
        if (-not $lock.Held) { throw "The measurement lock stayed busy for $WaitMinutes minutes; rerun the sweep when the other run ends." }
        Write-Output ("map-health: [{0}/{1}] {2} warmup (attempt {3})" -f ($doneCount + [array]::IndexOf($pending, $t) + 1), $total, $label, $attempts)

        & powershell -NoProfile -ExecutionPolicy Bypass -File $devSnapshot `
            -Map $map -Elevation $elevation -Set3d 'frame_log=1' `
            -OutputDirectory (Join-Path $outDir 'warmup') -TimeoutSeconds $WarmupTimeoutSeconds | Out-Null
        if ($LASTEXITCODE -ne 0) { throw "warmup dev-snapshot exited $LASTEXITCODE" }

        $cpu = Read-CpuLoadPercent
        $noisy = ($null -ne $cpu) -and (Test-NoisyRow $cpu $NoisyCpuPercent)

        $env:F2CE_PERF = '1'
        $measuredOutput = & powershell -NoProfile -ExecutionPolicy Bypass -File $devSnapshot `
            -Map $map -Elevation $elevation -Set3d 'frame_log=1' -CaptureConsole -KeepRunFolder `
            -OutputDirectory (Join-Path $outDir 'measured') -TimeoutSeconds $MeasuredTimeoutSeconds -HoldSeconds $HoldSeconds
        $env:F2CE_PERF = $null
        if ($LASTEXITCODE -ne 0) { throw "measured dev-snapshot exited $LASTEXITCODE" }

        $runFolderLine = @($measuredOutput) | Where-Object { $_ -like 'RUN FOLDER: *' } | Select-Object -Last 1
        $measuredDir = Join-Path $outDir 'measured'
        $csvPath = $null
        $perfLogPath = $null
        if ($runFolderLine) {
            $runFolder = $runFolderLine.Substring('RUN FOLDER: '.Length).Trim()
            $csvPath = Join-Path $runFolder 'frame_log.csv'
            $perfLogPath = Join-Path $runFolder 'logs\devperf.log'
        }
        if (-not (Test-Path -LiteralPath $csvPath) -or -not (Test-Path -LiteralPath $perfLogPath)) {
            throw "no frame_log.csv or logs/devperf.log in the kept run folder"
        }
        $summary = Read-FrameLogSummary @(Get-Content -LiteralPath $csvPath)
        $loadMs = Read-LoadMs @(Get-Content -LiteralPath $perfLogPath)
        if ($summary.Play -eq 0) { throw 'the frame log has no play rows: the sample says nothing' }
        if ($null -eq $loadMs) { throw "no 'perf load first_frame' line in the measured run" }

        # The sidecar names the build, the frame's 3D state and its image.
        $sidecarPath = Get-ChildItem -LiteralPath $measuredDir -Filter 'devshot-*.json' -File -ErrorAction SilentlyContinue |
            Sort-Object LastWriteTime -Descending | Select-Object -First 1
        $scene3d = $null
        $renderer = $null
        $snapshotPath = $null
        if ($sidecarPath) {
            $sidecar = Get-Content -Raw -LiteralPath $sidecarPath.FullName | ConvertFrom-Json
            $scene3d = $sidecar.scene_3d_enabled
            $renderer = $sidecar.renderer
            if ($sidecar.image) {
                $candidate = Join-Path $sidecarPath.DirectoryName $sidecar.image
                if (Test-Path -LiteralPath $candidate) { $snapshotPath = Get-Item -LiteralPath $candidate }
            }
        }

        $fallback = Read-FallbackLines @(Get-Content -LiteralPath (Join-Path $measuredDir 'process.stdout.log') -ErrorAction SilentlyContinue)

        $row.status = 'ok'
        $row.avgFps = [Math]::Round($summary.AvgFps, 1)
        $row.oneLowFps = [Math]::Round($summary.OneLowFps, 1)
        $row.worstMs = [Math]::Round($summary.WorstMs, 1)
        $row.loadMs = [Math]::Round($loadMs, 1)
        $row.cpu = $cpu
        $row.noisy = [bool]$noisy
        $row.play = $summary.Play
        $row.loads = $summary.Load
        $row.fallback = @($fallback)
        if ($snapshotPath) { $row.snapshot = $snapshotPath.FullName }
        $row.scene_3d_enabled = $scene3d
        $row.renderer = $renderer
        $breaches = @()
        if ($row.oneLowFps -lt $GateOneLowFps) { $breaches += ('1% low {0} under the 60 fps gate' -f $row.oneLowFps) }
        if ($row.loadMs -gt $MaxLoadMs) { $breaches += ('load {0} ms over {1} ms' -f $row.loadMs, $MaxLoadMs) }
        if ($row.oneLowFps -lt $BarOneLowFps) { $breaches += ('1% low {0} under the 80 fps bar' -f $row.oneLowFps) }
        $row.breach = ($breaches -join '; ')
    } catch {
        # Only a real hang counts as TIMEOUT; a crash or a missing artifact
        # is FAILED (the table never calls a crash a timeout).
        $row.status = if ($_.Exception.Message -match 'No snapshot|timeout|watchdog') { 'timeout' } else { 'failed' }
        $row.error = ($_.Exception.Message -replace '\r?\n', ' ')
        if ($row.error.Length -gt 300) { $row.error = $row.error.Substring(0, 300) }
        Write-Output ("map-health: {0} {1}: {2}" -f $row.status.ToUpper(), $label, $row.error)
    } finally {
        Release-Lock $lock
        Save-Row $row
        $rows[$label] = [pscustomobject]$row
    }
    if ($row.status -ne 'ok') {
        $incomplete += "$label $($row.status) after $attempts attempts"
    } elseif ($row.noisy) {
        $noisyRows += $label
    }
}

# Final sweep status: valid rows stand on their own, the rest is listed.
$valid = 0
foreach ($t in $targets) {
    $row = $rows[('{0}:e{1}' -f $t.map, $t.elevation)]
    if ($null -ne $row -and $row.status -eq 'ok' -and -not $row.noisy) { $valid++ }
}
Write-Output ("map-health: {0} of {1} rows are valid results (build {2})" -f $valid, $targets.Count, $exeHash.Substring(0, 12))
if ($noisyRows.Count -gt 0) {
    Write-Output ("map-health: NOISY rows (not valid results, rerun on a free machine): {0}" -f ($noisyRows -join ', '))
    exit 2
}
if ($incomplete.Count -gt 0) {
    Write-Output ("map-health: INCOMPLETE rows: {0}" -f ($incomplete -join ', '))
    exit 1
}
exit 0
