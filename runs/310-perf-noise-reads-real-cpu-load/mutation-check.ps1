# 310: perf-check -SelfTest must fail when the fail-closed noise rule breaks.
# Each mutation is one exact text change in a temporary copy of
# tools/perf-check.ps1; the copy's -SelfTest has to exit non-zero. The
# unchanged script has to pass. Light: no game, no load, no real perf lock.
# Continue: a mutant's stderr must not stop this script (Windows PowerShell 5.1).
$ErrorActionPreference = 'Continue'
$source = Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'tools\perf-check.ps1'
$text = [IO.File]::ReadAllText($source)
$mutations = [ordered]@{
    'unknown load counts as clean' = @(
        'return ($null -eq $value) -or ($value -gt $noisyAbove)',
        'return ($null -ne $value) -and ($value -gt $noisyAbove)')
    'a noisy row lets the run pass' = @(
        "if (`$noisyRows -gt 0) { return 'NOISY' }",
        '')
    'raw sample cast to double (null becomes 0)' = @(
        '$value = ConvertTo-CpuPercent $property.Value',
        '$value = [double]$property.Value')
    'no range or NaN check' = @(
        ' -or $value -lt 0 -or $value -gt 100) { return $null }',
        ') { return $null }')
    'one missing sample is skipped' = @(
        '        if ($null -eq $value) { return $null }',
        '        if ($null -eq $value) { continue }')
}
$temp = Join-Path ([IO.Path]::GetTempPath()) ('f2r-310-mutation-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp | Out-Null
$bad = 0
try {
    $out = & powershell -NoProfile -ExecutionPolicy Bypass -File $source -SelfTest 2>&1
    $ok = $LASTEXITCODE -eq 0
    if (-not $ok) { $bad++ }
    '{0} unchanged script: exit {1}' -f $(if ($ok) { 'ok  ' } else { 'BAD ' }), $LASTEXITCODE
    foreach ($name in $mutations.Keys) {
        $from, $to = $mutations[$name]
        $count = ([regex]::Matches($text, [regex]::Escape($from))).Count
        if ($count -ne 1) { 'BAD  {0}: the text to mutate is found {1} times' -f $name, $count; $bad++; continue }
        $copy = Join-Path $temp 'perf-check.ps1'
        [IO.File]::WriteAllText($copy, $text.Replace($from, $to))
        $out = & powershell -NoProfile -ExecutionPolicy Bypass -File $copy -SelfTest 2>&1
        $caught = $LASTEXITCODE -ne 0
        if (-not $caught) { $bad++ }
        $first = @($out | Where-Object { "$_" -like 'selftest: FAIL*' } | Select-Object -First 1)
        '{0} {1}: exit {2} {3}' -f $(if ($caught) { 'ok  ' } else { 'BAD ' }), $name, $LASTEXITCODE, ($first -join '')
    }
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
}
if ($bad -gt 0) { "mutation-check: FAIL $bad"; exit 1 }
'mutation-check: ok'
