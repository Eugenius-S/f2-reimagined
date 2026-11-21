# 310: which CPU reading follows a known load? Bounded and repeatable.
#
# Holds perf-check's machine-wide lock (Global\fallout2-ce-perf-check) so it
# never overlaps a measurement. For each k in -Busy (exactly k busy
# processes, never more than half the logical cores) it samples -Samples
# times, about a second each:
#   load     Win32_Processor.LoadPercentage (perf-check's old input)
#   time     Win32_PerfFormattedData_PerfOS_Processor _Total PercentProcessorTime
#   util     ..._Counters_ProcessorInformation _Total PercentProcessorUtility
#   perf     ..._Counters_ProcessorInformation _Total PercentProcessorPerformance
#   procs    the CPU-seconds of every readable process over the sample, per
#            elapsed second and logical core (exited and protected processes
#            are missed: a rough comparator, not a reference)
# Each sample also records how many of the phase's workers were alive at
# its start and end and how long it took; a phase whose workers were not
# all alive through every sample prints INVALID. -RawCsv saves every sample.
#
# Every started worker is tracked at once and stopped (and waited for)
# before the lock is released, on success, on an error and on Ctrl+C. A
# forced kill of this script cannot run that cleanup; the workers then end
# on their own deadline (-WorkerSeconds).
param(
    [int[]]$Busy = @(0, 2, 4, 8),
    [int]$Samples = 6,
    [int]$WaitMinutes = 10,
    [int]$WorkerSeconds = 60,
    [string]$RawCsv = '',
    # For probe-selftest.ps1 only.
    [int]$Cores = [Environment]::ProcessorCount
)
$ErrorActionPreference = 'Stop'
foreach ($k in $Busy) {
    if ($k -lt 0 -or $k -gt [Math]::Floor($Cores / 2)) { throw "busy count ${k}: from 0 to half of $Cores cores" }
}

$mutex = New-Object System.Threading.Mutex($false, 'Global\fallout2-ce-perf-check')
try { $held = $mutex.WaitOne([TimeSpan]::FromMinutes($WaitMinutes)) } catch [System.Threading.AbandonedMutexException] { $held = $true }
if (-not $held) { $mutex.Dispose(); throw 'perf lock busy' }

$workers = [System.Collections.Generic.List[object]]::new()
function Stop-Workers {
    foreach ($w in $workers) {
        Stop-Process -Id $w.Id -Force -ErrorAction SilentlyContinue
        [void]$w.WaitForExit(5000)
    }
    $workers.Clear()
}
function Read-Wmi($class, $property) {
    try { return [double](Get-CimInstance $class -Filter "Name='_Total'" -ErrorAction Stop).$property } catch { return [double]::NaN }
}
function Cpu-Seconds { (Get-Process | Where-Object { $null -ne $_.CPU } | Measure-Object CPU -Sum).Sum }
function Alive { @($workers | Where-Object { -not $_.HasExited }).Count }

$raw = [System.Collections.Generic.List[object]]::new()
try {
    $cpuName = (Get-CimInstance Win32_Processor | Select-Object -First 1).Name
    "cores=$Cores sockets=$(@(Get-CimInstance Win32_Processor).Count) cpu=$("$cpuName".Trim())"
    'k  added  load  time  util  perf  procs  alive  maxSec'
    foreach ($k in $Busy) {
        for ($j = 0; $j -lt $k; $j++) {
            $workers.Add((Start-Process powershell -WindowStyle Hidden -PassThru -ArgumentList '-NoProfile', '-Command',
                "`$e=[DateTime]::UtcNow.AddSeconds($WorkerSeconds); while([DateTime]::UtcNow -lt `$e){}"))
        }
        Start-Sleep -Seconds 2
        $rows = foreach ($i in 1..$Samples) {
            $aliveStart = Alive
            $c0 = Cpu-Seconds; $t0 = [DateTime]::UtcNow
            $load = (Get-CimInstance Win32_Processor | Measure-Object LoadPercentage -Average).Average
            $time = Read-Wmi Win32_PerfFormattedData_PerfOS_Processor PercentProcessorTime
            $util = Read-Wmi Win32_PerfFormattedData_Counters_ProcessorInformation PercentProcessorUtility
            $perf = Read-Wmi Win32_PerfFormattedData_Counters_ProcessorInformation PercentProcessorPerformance
            Start-Sleep -Milliseconds 1000
            $cpuDelta = (Cpu-Seconds) - $c0
            $elapsed = ([DateTime]::UtcNow - $t0).TotalSeconds
            $row = [pscustomobject]@{ k = $k; sample = $i; load = $load; time = $time; util = $util; perf = $perf
                procs = 100 * $cpuDelta / ($elapsed * $Cores); cpuDelta = $cpuDelta; elapsed = $elapsed
                aliveStart = $aliveStart; aliveEnd = Alive }
            $raw.Add($row)
            $row
        }
        Stop-Workers
        $m = { param($n) ($rows | Measure-Object $n -Average).Average }
        $minAlive = ($rows | ForEach-Object { [Math]::Min($_.aliveStart, $_.aliveEnd) } | Measure-Object -Minimum).Minimum
        '{0,-2} {1,5:N0} {2,5:N0} {3,5:N0} {4,5:N0} {5,5:N0} {6,6:N1} {7,4}/{8,-2} {9,5:N1}{10}' -f $k, (100 * $k / $Cores),
            (& $m load), (& $m time), (& $m util), (& $m perf), (& $m procs), $minAlive, $k,
            ($rows | Measure-Object elapsed -Maximum).Maximum, $(if ($minAlive -ne $k) { '  INVALID' } else { '' })
    }
    if ($RawCsv) { $raw | Export-Csv -NoTypeInformation -Path $RawCsv }
} finally {
    Stop-Workers
    $mutex.ReleaseMutex()
    $mutex.Dispose()
}
