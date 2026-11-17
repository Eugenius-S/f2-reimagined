# 310: which CPU reading follows a known load? Bounded and repeatable.
#
# Holds perf-check's machine-wide lock (Global\fallout2-ce-perf-check) so it
# never overlaps a measurement. For k = 0, 2, 4, 8 busy processes (never more
# than half the logical cores) it samples once a second:
#   load     Win32_Processor.LoadPercentage (perf-check's current input)
#   time     Win32_PerfFormattedData_PerfOS_Processor _Total PercentProcessorTime
#   util     ..._Counters_ProcessorInformation _Total PercentProcessorUtility
#   perf     ..._Counters_ProcessorInformation _Total PercentProcessorPerformance
#   procs    the sum of every readable process's CPU time over the second,
#            as a share of all logical cores (protected processes are missed,
#            so this is a lower bound)
# and prints one line per phase with the means and the load the phase added
# (k / logical cores). Writes nothing but its own output.
param(
    [int[]]$Busy = @(0, 2, 4, 8),
    [int]$Samples = 6,
    [int]$WaitMinutes = 10
)
$ErrorActionPreference = 'Stop'
$cores = [Environment]::ProcessorCount
if (($Busy | Measure-Object -Maximum).Maximum -gt [Math]::Floor($cores / 2)) { throw "at most half of $cores cores" }

$mutex = New-Object System.Threading.Mutex($false, 'Global\fallout2-ce-perf-check')
try { $owned = $mutex.WaitOne([TimeSpan]::FromMinutes($WaitMinutes)) } catch [System.Threading.AbandonedMutexException] { $owned = $true }
if (-not $owned) { throw 'perf lock busy' }

function Read-Wmi($class, $property) {
    try { return [double](Get-CimInstance $class -Filter "Name='_Total'" -ErrorAction Stop).$property } catch { return [double]::NaN }
}
function Cpu-Seconds { (Get-Process | Where-Object { $null -ne $_.CPU } | Measure-Object CPU -Sum).Sum }

try {
    "cores=$cores sockets=$(@(Get-CimInstance Win32_Processor).Count) cpu=$((Get-CimInstance Win32_Processor | Select-Object -First 1).Name.Trim())"
    'k  added  load  time  util  perf  procs'
    foreach ($k in $Busy) {
        $seconds = 4 * $Samples + 10  # outlives the sampling; stopped right after it
        $workers = @(1..$k | Where-Object { $_ -gt 0 } | ForEach-Object {
            Start-Process powershell -WindowStyle Hidden -PassThru -ArgumentList '-NoProfile', '-Command',
                "`$e=[DateTime]::UtcNow.AddSeconds($seconds); while([DateTime]::UtcNow -lt `$e){}"
        })
        Start-Sleep -Seconds 2
        $rows = foreach ($i in 1..$Samples) {
            $c0 = Cpu-Seconds; $t0 = [DateTime]::UtcNow
            $load = (Get-CimInstance Win32_Processor | Measure-Object LoadPercentage -Average).Average
            $time = Read-Wmi Win32_PerfFormattedData_PerfOS_Processor PercentProcessorTime
            $util = Read-Wmi Win32_PerfFormattedData_Counters_ProcessorInformation PercentProcessorUtility
            $perf = Read-Wmi Win32_PerfFormattedData_Counters_ProcessorInformation PercentProcessorPerformance
            Start-Sleep -Milliseconds 1000
            $procs = 100 * ((Cpu-Seconds) - $c0) / (([DateTime]::UtcNow - $t0).TotalSeconds * $cores)
            [pscustomobject]@{ load = $load; time = $time; util = $util; perf = $perf; procs = $procs }
        }
        $workers | ForEach-Object { Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue }
        $m = { param($n) ($rows | Measure-Object $n -Average).Average }
        '{0,-2} {1,5:N0} {2,5:N0} {3,5:N0} {4,5:N0} {5,5:N0} {6,6:N1}' -f $k, (100 * $k / $cores),
            (& $m load), (& $m time), (& $m util), (& $m perf), (& $m procs)
    }
} finally {
    $mutex.ReleaseMutex()
    $mutex.Dispose()
}
