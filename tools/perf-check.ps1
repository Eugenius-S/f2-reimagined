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
# 290: the whole run holds one machine-wide named mutex
# (Global\fallout2-ce-perf-check), so two agents never measure speed at
# once; a busy lock is waited for (-WaitMinutes), and worktrees line up
# behind it instead of measuring and building side by side. The table
# prints the CPU load seen at the start of each map; a map measured under
# more than -NoisyCpuPercent load is marked noisy: its numbers say nothing
# about the game, so a threshold breach there is not a FAIL, it is a NOISY
# line asking for a rerun on a free machine. The thresholds do not move
# for noise. 310: a row whose load could not be read is noisy too, and a
# run with any noisy row never prints PASS. Exit codes: 0 PASS, 1 FAIL (a
# clean row breached the bar, or an artifact is missing), 2 NOISY (rerun
# needed, no verdict).
#
# -SelfTest feeds saved samples to the parsing functions and fails on a
# wrong answer without running the game.
param(
    [double]$MinFps = 60,
    [double]$MaxLoadMs = 2000,
    # Seconds the game stays on the measured map after its snapshot: the sample.
    [int]$HoldSeconds = 10,
    # A map measured under more CPU load than this is noisy (290): the
    # numbers were taken on a busy machine, rerun instead of judging.
    [double]$NoisyCpuPercent = 30,
    # Minutes to wait for the machine-wide measurement lock (290).
    [ValidateRange(0, 240)]
    [int]$WaitMinutes = 30,
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

# The machine-wide measurement lock (290): a named mutex, the same for every
# worktree, so agents line up behind one speed run instead of measuring side
# by side. A zero wait is an immediate try; an abandoned lock (its holder
# died) counts as taken over.
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

# The CPU load at the start of a map (290): the kernel's busy time of all
# cores (PercentProcessorTime of _Total) sampled four times over about a
# second, so one twitch does not decide. Locale-free, unlike Get-Counter's
# localized counter names. 310: Win32_Processor.LoadPercentage read 15-35
# points over this on a near-idle machine and did not follow a known load
# (runs/310-perf-noise-reads-real-cpu-load). $null when the read says
# nothing, and also when any one sample is missing or not a percent:
# [double]$null is 0 in PowerShell, so a raw value is checked before it is
# converted (review of #521). Test-NoisyRow counts $null as noisy.
# -ReadSample and -SleepMs exist for -SelfTest only.
function Read-CpuLoadPercent([scriptblock]$ReadSample = {
        Get-CimInstance Win32_PerfFormattedData_PerfOS_Processor -Filter "Name='_Total'" -ErrorAction Stop
    }, [int]$SleepMs = 250) {
    $samples = @()
    foreach ($i in 1..4) {
        try { $instance = & $ReadSample } catch { return $null }
        if ($null -eq $instance -or @($instance).Count -ne 1) { return $null }
        $property = $instance.PSObject.Properties['PercentProcessorTime']
        if ($null -eq $property) { return $null }
        $value = ConvertTo-CpuPercent $property.Value
        if ($null -eq $value) { return $null }
        $samples += $value
        if ($SleepMs -gt 0) { Start-Sleep -Milliseconds $SleepMs }
    }
    return [Math]::Round(($samples | Measure-Object -Average).Average, 0)
}

# A finite number from 0 to 100, or $null: no missing, empty, text, NaN,
# infinite or out-of-range value ever becomes a load.
function ConvertTo-CpuPercent($raw) {
    if ($null -eq $raw -or $raw -is [bool]) { return $null }
    $value = 0.0
    if (-not [double]::TryParse([string]$raw, [Globalization.NumberStyles]::Float,
            [Globalization.CultureInfo]::InvariantCulture, [ref]$value)) { return $null }
    if ([double]::IsNaN($value) -or [double]::IsInfinity($value) -or $value -lt 0 -or $value -gt 100) { return $null }
    return $value
}

# A map measured over this load is noisy (290); at the limit it is not.
# An unknown or invalid load is noisy (310): no PASS without evidence of a
# free machine.
function Test-NoisyRow($loadPercent, [double]$noisyAbove) {
    $value = ConvertTo-CpuPercent $loadPercent
    return ($null -eq $value) -or ($value -gt $noisyAbove)
}

# 310: FAIL wins, then any noisy row; PASS only with neither.
function Get-RunVerdict([int]$failures, [int]$noisyRows) {
    if ($failures -gt 0) { return 'FAIL' }
    if ($noisyRows -gt 0) { return 'NOISY' }
    return 'PASS'
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
    function Assert-True([string]$what, $got) {
        if (-not $got) {
            Write-Output ("selftest: FAIL {0}" -f $what)
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
    # 290: strictly over the limit is noisy, at it is not.
    Assert-True 'load 29 at 30 is clean' (-not (Test-NoisyRow 29 30))
    Assert-True 'load 30 at 30 is clean' (-not (Test-NoisyRow 30 30))
    Assert-True 'load 31 at 30 is noisy' (Test-NoisyRow 31 30)
    Assert-True 'an unread load is noisy' (Test-NoisyRow $null 30)
    Assert-True 'a NaN load is noisy' (Test-NoisyRow ([double]::NaN) 30)
    Assert-True 'a negative load is noisy' (Test-NoisyRow -5 30)
    # 310 (review of #521): the reader end to end on fake counter answers.
    # Every missing or invalid sample makes the whole read unknown.
    function Read-Fake([object[]]$answers, [int]$throwAt = 0) {
        $calls = @{ n = 0 }
        $reader = {
            $calls.n++
            if ($calls.n -eq $throwAt) { throw 'injected counter failure' }
            $answers[[Math]::Min($calls.n - 1, $answers.Count - 1)]
        }.GetNewClosure()
        return Read-CpuLoadPercent -ReadSample $reader -SleepMs 0
    }
    function Pct($v) { [pscustomobject]@{ PercentProcessorTime = $v } }
    $unknown = [ordered]@{
        'no instance' = @(, $null)
        'no property' = @([pscustomobject]@{ Name = '_Total' })
        'null value' = @(Pct $null)
        'empty value' = @(Pct '')
        'text value' = @(Pct 'bad')
        'negative value' = @(Pct -10)
        'over 100' = @(Pct 110)
        'NaN value' = @(Pct ([double]::NaN))
        'infinite value' = @(Pct ([double]::PositiveInfinity))
        'two instances' = @(, @((Pct 5), (Pct 5)))
        'one missing among valid' = @((Pct 5), (Pct 5), (Pct 5), $null)
    }
    foreach ($case in $unknown.Keys) {
        $read = Read-Fake $unknown[$case]
        Assert-True "read with $case is unknown" ($null -eq $read)
        Assert-True "read with $case is noisy" (Test-NoisyRow $read 30)
    }
    Assert-True 'a counter that throws first is unknown' ($null -eq (Read-Fake @(Pct 5) 1))
    Assert-True 'a counter that throws third is unknown' ($null -eq (Read-Fake @(Pct 5) 3))
    Assert-Equal 'a steady 29 reads 29' (Read-Fake @(Pct 29)) 29
    Assert-True 'a read of 30 at 30 is clean' (-not (Test-NoisyRow (Read-Fake @(Pct 30)) 30))
    Assert-True 'a read of 31 at 30 is noisy' (Test-NoisyRow (Read-Fake @(Pct 31)) 30)
    Assert-Equal 'a UInt64 counter value reads' (Read-Fake @(Pct ([uint64]12))) 12
    Assert-Equal 'samples 0,0,0,100 average to 25' (Read-Fake @((Pct 0), (Pct 0), (Pct 0), (Pct 100))) 25
    Assert-True 'a clean breach fails' ((Get-RunVerdict 1 0) -eq 'FAIL')
    Assert-True 'a clean breach fails beside a noisy row' ((Get-RunVerdict 1 2) -eq 'FAIL')
    Assert-True 'a noisy row without a breach is no PASS' ((Get-RunVerdict 0 1) -eq 'NOISY')
    Assert-True 'all clean rows pass' ((Get-RunVerdict 0 0) -eq 'PASS')
    # The CPU read itself: a number within 0..100 on a live machine, or an
    # honest $null where the counter says nothing.
    $cpu = Read-CpuLoadPercent
    if ($null -ne $cpu -and $null -eq (ConvertTo-CpuPercent $cpu)) {
        Write-Output ("selftest: FAIL cpu read out of range: {0}" -f $cpu)
        $failed = $true
    }
    # 290: the machine-wide lock wait. A holder process takes a test mutex
    # for three seconds; this process must see it busy, and free once the
    # holder lets go. A name of its own, so runs never fight the real one.
    $probeName = 'Global\fallout2-ce-perf-check-selftest'
    $holderCommand = "`$m = New-Object System.Threading.Mutex(`$false, '{0}'); [void]`$m.WaitOne(20000); Start-Sleep -Seconds 3; `$m.ReleaseMutex(); `$m.Dispose()" -f $probeName
    $holder = Start-Process powershell -PassThru -WindowStyle Hidden -ArgumentList @('-NoProfile', '-Command', $holderCommand)
    $busySeen = $false
    $deadline = (Get-Date).AddSeconds(20)
    while ((Get-Date) -lt $deadline) {
        $probe = Acquire-PerfLock $probeName 0
        if (-not $probe.Held) { $busySeen = $true; break }
        Release-Lock $probe
        Start-Sleep -Milliseconds 300
    }
    Assert-True 'the lock is seen busy while another process holds it' $busySeen
    [void]$holder.WaitForExit(30000)
    $probe = Acquire-PerfLock $probeName 1
    $freedSeen = $probe.Held
    Release-Lock $probe
    Assert-True 'the lock is free again after the holder lets go' $freedSeen
    if ($failed) { exit 1 }
    Write-Output 'selftest: ok'
    exit 0
}

# One speed measurement on the whole machine (290): take the lock before the
# first map and hold it to the end, so warmups and measured runs of two
# agents never overlap. The wait is patient: a full check is four maps
# times two runs, and it says so while it waits. The trap lets go on any
# exit path.
$script:perfLock = $null
function Release-PerfLock {
    Release-Lock $script:perfLock
    $script:perfLock = $null
}
trap { Release-PerfLock; break }
$script:perfLock = Acquire-PerfLock 'Global\fallout2-ce-perf-check' 0
$waitTold = $false
$waitDeadline = (Get-Date).AddMinutes($WaitMinutes)
while (-not $perfLock.Held) {
    if (-not $waitTold) {
        Write-Output "perf-check: the measurement lock is busy, another run is measuring; waiting up to $WaitMinutes minutes"
        $waitTold = $true
    }
    if ((Get-Date) -gt $waitDeadline) {
        throw "The perf-check lock stayed busy for $WaitMinutes minutes: another run is measuring on this machine. Rerun when it ends."
    }
    Release-Lock $perfLock
    Start-Sleep -Seconds 5
    $script:perfLock = Acquire-PerfLock 'Global\fallout2-ce-perf-check' 1
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
$reruns = @()
$noisyRows = 0
foreach ($m in $maps) {
    $label = ('{0} e{1}' -f $m.map, $m.elevation)
    $outDir = Join-Path $outRoot ('{0}-e{1}' -f $m.map, $m.elevation)
    # Warmup: the exporter cache and the art fills land on disk here.
    & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'dev-snapshot.ps1') `
        -Map $m.map -Elevation $m.elevation -Set3d 'frame_log=1' `
        -OutputDirectory (Join-Path $outDir 'warmup') -TimeoutSeconds 240 | Out-Null
    # 290: the load seen at the start of this map's measurement. A row over
    # -NoisyCpuPercent is noisy: a breach there asks for a rerun, it does
    # not judge the game.
    $cpu = Read-CpuLoadPercent
    $noisy = Test-NoisyRow $cpu $NoisyCpuPercent
    if ($noisy) { $noisyRows++ }
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
        cpu = if ($null -ne $cpu) { $cpu } else { '?' }
        noisy = if ($noisy) { 'noisy' } else { '' }
        play = $summary.Play
        load = $summary.Load
    }
    if ($summary.OneLowFps -lt $MinFps) {
        $msg = "{0}: 1% low {1} fps is under {2}" -f $label, [Math]::Round($summary.OneLowFps, 1), $MinFps
        if ($noisy) { $reruns += $msg } else { $failures += $msg }
    }
    if ($null -eq $loadMs) {
        $failures += "{0}: no 'perf load first_frame' line in the measured run" -f $label
    } elseif ($loadMs -gt $MaxLoadMs) {
        $msg = "{0}: load {1} ms is over {2}" -f $label, [Math]::Round($loadMs, 1), $MaxLoadMs
        if ($noisy) { $reruns += $msg } else { $failures += $msg }
    }
}

Release-PerfLock

$rows | Format-Table -AutoSize map, avgFps, oneLowFps, worstMs, loadMs, cpu, noisy, play, load

foreach ($r in $reruns) { Write-Output "NOISY $r" }
$verdict = Get-RunVerdict $failures.Count $noisyRows
if ($verdict -eq 'FAIL') {
    foreach ($f in $failures) { Write-Output "FAIL $f" }
    exit 1
}
if ($verdict -eq 'NOISY') {
    Write-Output ("perf-check: NOISY {0} of {1} maps were measured under CPU load over {2} percent or an unread load: the gate says nothing about them, and this is not a PASS. Rerun perf-check when the machine is free." -f $noisyRows, $maps.Count, $NoisyCpuPercent)
    exit 2
}
Write-Output ("perf-check: PASS all {0} maps (1% low >= {1} fps, load <= {2} ms)" -f $maps.Count, $MinFps, $MaxLoadMs)
