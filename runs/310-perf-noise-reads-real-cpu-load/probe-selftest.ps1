# 310: offline checks of probe.ps1 with fake workers, counters and lock.
# No process is started, no CPU is loaded, the real perf lock is not touched.
# Checks: exactly k workers per phase (k=0 starts none); every started
# worker is stopped before the lock is released when sampling throws and
# when a worker fails to start. Then two mutants (the old enumeration, no
# cleanup in finally) must each fail a check. Exit 0 only if all hold.
$ErrorActionPreference = 'Stop'
$probeText = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'probe.ps1'))

function New-Object {
    param($TypeName, $ArgumentList)
    if ($TypeName -ne 'System.Threading.Mutex') { throw "unexpected New-Object $TypeName" }
    $m = [pscustomobject]@{}
    $m | Add-Member ScriptMethod WaitOne { param($t) $true }
    $m | Add-Member ScriptMethod ReleaseMutex { $global:fake.events.Add('release') }
    $m | Add-Member ScriptMethod Dispose { }
    $m
}
function Start-Process {
    param($FilePath, [switch]$PassThru, $WindowStyle, $ArgumentList)
    if ($global:fake.startFailAt -and $global:fake.started + 1 -eq $global:fake.startFailAt) { throw 'injected start failure' }
    $global:fake.started++
    $id = $global:fake.started
    $global:fake.events.Add("start:$id")
    $w = [pscustomobject]@{ Id = $id; HasExited = $false }
    $w | Add-Member ScriptMethod WaitForExit { param($ms) $true }
    $global:fake.live[$id] = $w
    $w
}
function Stop-Process {
    param($Id, [switch]$Force, $ErrorAction)
    $global:fake.events.Add("stop:$Id")
    if ($global:fake.live.ContainsKey($Id)) { $global:fake.live[$Id].HasExited = $true }
}
function Start-Sleep { param($Seconds, $Milliseconds) }
function Get-Process { [pscustomobject]@{ CPU = 1.0 } }
function Get-CimInstance {
    param($ClassName, $Filter, $ErrorAction)
    if ($ClassName -eq 'Win32_Processor') {
        $global:fake.cpuCalls++
        if ($global:fake.throwAt -and $global:fake.cpuCalls -eq $global:fake.throwAt) { throw 'injected sampling failure' }
    }
    [pscustomobject]@{ Name = 'fake'; LoadPercentage = 10; PercentProcessorTime = 10
        PercentProcessorUtility = 10; PercentProcessorPerformance = 100 }
}
function Export-Csv { param([Parameter(ValueFromPipeline = $true)]$InputObject, [switch]$NoTypeInformation, $Path) }

function Invoke-Probe([string]$text, [hashtable]$probeArgs, [int]$throwAt = 0, [int]$startFailAt = 0) {
    $global:fake = @{ events = [System.Collections.Generic.List[string]]::new(); live = @{}; started = 0
        cpuCalls = 0; throwAt = $throwAt; startFailAt = $startFailAt }
    $out = @()
    $err = $null
    try { $out = & ([scriptblock]::Create($text)) @probeArgs -Cores 16 } catch { $err = $_.Exception.Message }
    $stops = @($global:fake.events | Where-Object { $_ -like 'stop:*' } | ForEach-Object { [int]($_ -replace 'stop:', '') } | Sort-Object -Unique)
    $releaseAt = $global:fake.events.IndexOf('release')
    $lastStop = -1
    for ($i = 0; $i -lt $global:fake.events.Count; $i++) { if ($global:fake.events[$i] -like 'stop:*') { $lastStop = $i } }
    [pscustomobject]@{ out = $out; err = $err; started = $global:fake.started; stopped = $stops.Count
        releases = @($global:fake.events | Where-Object { $_ -eq 'release' }).Count
        stopsBeforeRelease = ($global:fake.started -eq 0) -or ($lastStop -ge 0 -and $lastStop -lt $releaseAt) }
}

function Test-Probe([string]$text) {
    $fails = @()
    # Exactly k workers: 0 then 2.
    $r = Invoke-Probe $text @{ Busy = @(0, 2); Samples = 1 }
    if ($r.err) { $fails += "normal run threw: $($r.err)" }
    if ($r.started -ne 2) { $fails += "k=0 then k=2 started $($r.started) workers, want 2" }
    $k0 = @($r.out | Where-Object { "$_" -like '0 *' })
    if ($k0.Count -ne 1 -or "$k0" -notlike '* 0/0 *' -or "$k0" -like '*INVALID*') { $fails += "k=0 line is not '0/0' and valid: $k0" }
    if ($r.stopped -ne 2 -or -not $r.stopsBeforeRelease -or $r.releases -ne 1) { $fails += 'normal run: workers not all stopped before the one release' }
    # Sampling throws in the k=2 phase (call 1 and 2 are the header reads, 3 is the first sample).
    $r = Invoke-Probe $text @{ Busy = @(2); Samples = 2 } -throwAt 4
    if (-not $r.err) { $fails += 'injected sampling failure did not surface' }
    if ($r.started -ne 2 -or $r.stopped -ne 2 -or -not $r.stopsBeforeRelease -or $r.releases -ne 1) {
        $fails += "sampling failure: started $($r.started) stopped $($r.stopped), stops before release $($r.stopsBeforeRelease)" }
    # The third worker fails to start: the two started ones are still stopped first.
    $r = Invoke-Probe $text @{ Busy = @(4); Samples = 1 } -startFailAt 3
    if ($r.started -ne 2 -or $r.stopped -ne 2 -or -not $r.stopsBeforeRelease -or $r.releases -ne 1) {
        $fails += "start failure: started $($r.started) stopped $($r.stopped), stops before release $($r.stopsBeforeRelease)" }
    # A worker that dies during a phase makes it INVALID.
    $r = Invoke-Probe ($text.Replace('$aliveStart = Alive', '$aliveStart = Alive; if ($i -eq 1 -and $workers.Count) { $workers[0].HasExited = $true }')) @{ Busy = @(2); Samples = 2 }
    if (-not (@($r.out | Where-Object { "$_" -like '*INVALID*' }).Count)) { $fails += 'a dead worker did not mark the phase INVALID' }
    $fails
}

$bad = 0
$fails = Test-Probe $probeText
if ($fails) { $bad++; $fails | ForEach-Object { "BAD  probe.ps1: $_" } } else { 'ok   probe.ps1: exact k, cleanup before release, liveness' }
$mutants = [ordered]@{
    'old enumeration 1..k' = @('for ($j = 0; $j -lt $k; $j++) {', 'foreach ($j in (1..$k | Where-Object { $_ -gt 0 })) {')
    'no cleanup in finally' = @("} finally {`n    Stop-Workers`n", "} finally {`n")
}
foreach ($name in $mutants.Keys) {
    $from, $to = $mutants[$name]
    $text = $probeText.Replace("`r`n", "`n")
    if (([regex]::Matches($text, [regex]::Escape($from))).Count -ne 1) { "BAD  mutant '$name': text not found once"; $bad++; continue }
    $fails = Test-Probe ($text.Replace($from, $to))
    if ($fails) { "ok   mutant '$name' caught: $($fails[0])" } else { "BAD  mutant '$name' was not caught"; $bad++ }
}
if ($bad) { "probe-selftest: FAIL $bad"; exit 1 }
'probe-selftest: ok'
