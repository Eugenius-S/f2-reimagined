# Prints the task board from the headers of tasks/*.md.
#
# -Next <agent> prints the one task the agent should work on (its own doing
# task first, otherwise the free task by the AGENTS.md rule); -Review lists
# the done tasks that still need a review; -SelfTest feeds made-up rows to
# the taken/stale decision and fails on a wrong answer. All read the task
# files from origin/main after a git fetch; without a network they fall back
# to the working tree and say so. A task is taken when an open PR's title
# starts with its number, or when any origin/task/NNN-* branch (any name
# tail) has a commit in the last three days. A doing task without a live
# lock is stale: any agent may continue it. The plain board prints the
# blocked tasks in one block at the end, each with the first Log line that
# names the reason.
param(
    [string]$Next,
    [switch]$Review,
    [switch]$SelfTest
)
$ErrorActionPreference = 'Stop'

# The one taken/stale decision for the whole script (also fed by -SelfTest).
# A task is locked ("live") while an open PR carries its number or any
# task/NNN-* branch committed within the last three days. Without a live
# lock a doing task (or a todo task that still has a branch) is "stale":
# any agent may continue it from its branch and Log. Everything else keeps
# its header status ("plain").
function Resolve-LockState {
    param(
        [string]$Id,
        [string]$Status,
        $BranchTime,
        $PrNumber,
        [long]$Now,
        [int]$StaleDays
    )
    if ($null -ne $PrNumber) { return 'live' }
    $fresh = ($null -ne $BranchTime) -and (($Now - [long]$BranchTime) -le $StaleDays * 86400)
    if ($fresh) { return 'live' }
    if ($Status -in 'todo', 'doing') {
        if ($null -ne $BranchTime -or $Status -eq 'doing') { return 'stale' }
    }
    return 'plain'
}

if ($SelfTest) {
    $now = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds()
    $cases = @(
        # a live branch whose slug differs from the task file's name
        @{ Id = '111'; Status = 'doing'; BranchTime = $now - 100; PrNumber = $null; Want = 'live' },
        # an open PR titled "NNN: ..." locks the task even without a branch
        @{ Id = '186'; Status = 'doing'; BranchTime = $null; PrNumber = 401; Want = 'live' },
        # a branch silent for longer than the stale window does not lock
        @{ Id = '145'; Status = 'doing'; BranchTime = $now - 4 * 86400; PrNumber = $null; Want = 'stale' },
        # a doing task with no branch and no PR is stale too
        @{ Id = '207'; Status = 'doing'; BranchTime = $null; PrNumber = $null; Want = 'stale' }
    )
    $failed = $false
    foreach ($c in $cases) {
        $got = Resolve-LockState -Id $c.Id -Status $c.Status -BranchTime $c.BranchTime `
            -PrNumber $c.PrNumber -Now $now -StaleDays 3
        if ($got -ne $c.Want) {
            Write-Output ("selftest: FAIL id={0} want={1} got={2}" -f $c.Id, $c.Want, $got)
            $failed = $true
        }
    }
    if ($failed) { exit 1 }
    Write-Output 'selftest: ok (4 cases)'
    exit 0
}

$Root = Split-Path -Parent $PSScriptRoot

function Read-Header([string[]]$lines) {
    $header = @{}
    if ($lines.Count -eq 0 -or $lines[0] -ne '---') { return $null }
    for ($i = 1; $i -lt $lines.Count -and $lines[$i] -ne '---'; $i++) {
        if ($lines[$i] -match '^([a-z-]+):\s*(.*?)\s*(#.*)?$') { $header[$Matches[1]] = $Matches[2] }
    }
    return $header
}

# Prefer origin/main (another agent may work on a branch in another
# worktree); fall back to the working tree without a network.
$source = 'origin/main'
# git's warnings on stderr must not stop Windows PowerShell 5 here.
$ErrorActionPreference = 'Continue'
git fetch origin 2>$null
$ErrorActionPreference = 'Stop'
$files = @()
git ls-tree --name-only origin/main tasks/ 2>$null | ForEach-Object { $files += $_ }
if ($files.Count -eq 0) {
    $source = 'the working tree (git fetch failed)'
    $files = Get-ChildItem (Join-Path $Root 'tasks') -Filter '*.md' |
        ForEach-Object { "tasks/" + $_.Name } | Sort-Object
}

$StaleDays = 3
$branchTimes = @{}     # 'task/NNN-slug' -> last commit (unix seconds)
$newestBranchTime = @{} # 'NNN' -> newest commit across every task/NNN-* branch
git for-each-ref --format='%(refname:short) %(committerdate:unix)' refs/remotes/origin/task/ 2>$null |
    ForEach-Object {
        $parts = $_ -split ' '
        $branch = $parts[0] -replace '^origin/', ''
        $branchTimes[$branch] = [long]$parts[1]
        if ($branch -match '^task/(\d+)-') {
            $num = $Matches[1]
            if (-not $newestBranchTime.ContainsKey($num) -or [long]$parts[1] -gt $newestBranchTime[$num]) {
                $newestBranchTime[$num] = [long]$parts[1]
            }
        }
    }
$now = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds()

# One call for the whole board: open PRs whose title starts with "NNN:".
$prLocks = @{}
$ghListed = $false
$ErrorActionPreference = 'Continue'
$prJson = $null
try { $prJson = gh pr list --state open --limit 300 --json number,title 2>$null } catch { $prJson = $null }
$ErrorActionPreference = 'Stop'
if ($LASTEXITCODE -eq 0 -and $prJson) {
    try {
        foreach ($pr in ($prJson | ConvertFrom-Json)) {
            if ($pr.title -match '^(\d+):') { $prLocks[[string]$Matches[1]] = [int]$pr.number }
        }
        $ghListed = $true
    } catch { $ghListed = $false }
}
if (-not $ghListed) {
    Write-Output 'gh is unavailable: open-PR locks are skipped, branches only.'
}

$script:resolvedLines = @{}
$rows = foreach ($file in $files) {
    $branch = 'task/' + (Split-Path -Leaf $file).Replace('.md', '')
    $lines = git show "$source`:$file" 2>$null
    $onMain = Read-Header @($lines)
    # The branch copy tells a continuing agent where the work stopped; read
    # the freshest branch of this task's number (the exact-slug one first).
    $branchFile = $null
    if ($branchTimes.ContainsKey($branch)) { $branchFile = $branch }
    elseif ($onMain -and $newestBranchTime.ContainsKey([string]$onMain['id'])) {
        $num = [string]$onMain['id']
        $newest = $null
        foreach ($b in $branchTimes.Keys) {
            if ($b -match ('^task/' + [regex]::Escape($num) + '-')) {
                if ($null -eq $newest -or $branchTimes[$b] -gt $branchTimes[$newest]) { $newest = $b }
            }
        }
        if ($newest) { $branchFile = $newest }
    }
    if ($branchFile -and $onMain -and $onMain['status'] -ne 'done') {
        $fromBranch = git show "origin/$branchFile`:$file" 2>$null
        if ($fromBranch) { $lines = $fromBranch }
    }
    if ($null -eq $lines -or @($lines).Count -eq 0) {
        $lines = Get-Content (Join-Path $Root ($file -replace '/', '\'))
    }
    $header = Read-Header @($lines)
    if ($null -eq $header) { continue }
    $status = $header['status']
    if ($status -notin 'todo', 'doing', 'done', 'blocked') {
        Write-Output ("warning: {0} has status '{1}' outside todo/doing/done/blocked" -f $file, $status)
    }
    $lock = Resolve-LockState -Id ([string]$header['id']) -Status $status `
        -BranchTime ($newestBranchTime[[string]$header['id']]) -PrNumber ($prLocks[[string]$header['id']]) `
        -Now $now -StaleDays $StaleDays
    $status = switch ($lock) {
        'live' { if ($status -eq 'done') { 'done' } else { 'doing' } }
        'stale' { 'stale' }
        default { $status }
    }
    [pscustomobject]@{
        id = $header['id']
        status = $status
        owner = $header['owner']
        suggested = $header['suggested']
        priority = $header['priority']
        # the board order lives on main, whatever the branch's copy says.
        tier = if ($onMain -and $onMain['tier']) { [int]$onMain['tier'] } elseif ($header['tier']) { [int]$header['tier'] } else { 3 }
        claude = $header['claude-only']
        deps = $header['depends-on']
        reviewed = $header['reviewed']
        title = $header['title']
        file = $file
        branch = $branch
    }
    # Keep the resolved body for the blocked block below.
    $script:resolvedLines[$file] = @($lines)
}

if ($Review) {
    $rows | Where-Object { $_.status -eq 'done' -and $_.reviewed -eq 'no' } |
        Format-Table -AutoSize id, title, file
    exit 0
}

if ($Next) {
    $agent = $Next
    $mine = @($rows | Where-Object { $_.status -in 'doing', 'stale' -and $_.owner -eq $agent } | Sort-Object { $_.tier }, { [int]$_.id })
    $stale = @($rows | Where-Object { $_.status -eq 'stale' -and ($_.claude -ne 'yes' -or $agent -eq 'claude') } |
        Sort-Object { $_.tier }, { [int]$_.id })
    $done = @{}
    foreach ($r in $rows) { if ($r.status -eq 'done') { $done[$r.id] = $true } }

    $task = $null
    if ($mine.Count -gt 0) {
        $task = $mine[0]
        Write-Output "${agent}: continuing its own doing task"
    } else {
        $free = @($rows | Where-Object {
            $_.status -eq 'todo' `
                -and ($_.owner -in 'none', 'any', '' -or $_.owner -eq $agent) `
                -and ($_.claude -ne 'yes' -or $agent -eq 'claude') `
                -and (($_.deps -replace '[\[\]]', '').Split(',') | Where-Object { $_.Trim() } |
                    ForEach-Object { [bool]$done[$_.Trim()] }) -notcontains $false
        })
        # the board order: the lower tier first, then high priority, then the lower id;
        # a stale task competes with the free ones by the same order.
        $order = @({ $_.tier }, { switch ($_.priority) { 'high' { 0 } 'low' { 2 } default { 1 } } }, { [int]$_.id })
        $task = @(@($stale) + @($free) | Sort-Object $order)[0]
        if ($task -and $task.status -eq 'stale') {
            Write-Output "${agent}: continuing a stale task of $($task.owner) from its branch and Log"
        } elseif ($task) {
            Write-Output "${agent}: next free task"
        }
    }

    if ($task) {
        Write-Output "  file:   $($task.file)"
        Write-Output "  branch: $($task.branch)"
        Write-Output "  title:  $($task.title)"
    } else {
        Write-Output "${agent}: nothing is free."
        $waiting = @($rows | Where-Object {
            $_.status -eq 'todo' -and ($_.claude -ne 'yes' -or $agent -eq 'claude')
        } | Sort-Object { [int]$_.id })
        foreach ($t in $waiting) {
            $missing = @(($t.deps -replace '[\[\]]', '').Split(',') |
                Where-Object { $_.Trim() -and !$done[$_.Trim()] } |
                ForEach-Object { $_.Trim() })
            if ($missing.Count -gt 0) {
                Write-Output "  $($t.id) '$($t.title)' waits for: $($missing -join ', ')"
            }
        }
    }
    exit 0
}

$rows | Format-Table -AutoSize id, status, owner, suggested, priority, tier, claude, reviewed, title, file

# The blocked tasks stop being invisible: one block, each with the first
# Log line that names the reason.
$blocked = @($rows | Where-Object { $_.status -eq 'blocked' } | Sort-Object { [int]$_.id })
if ($blocked.Count -gt 0) {
    Write-Output ''
    Write-Output ('Blocked tasks: {0}' -f $blocked.Count)
    foreach ($t in $blocked) {
        $reason = ''
        $inLog = $false
        foreach ($l in $script:resolvedLines[$t.file]) {
            $trim = ('' + $l).Trim()
            if ($trim -match '^##\s+Log') { $inLog = $true; continue }
            if (-not $inLog) { continue }
            $line = $trim -replace '^- ', ''
            if ($line -match 'blocked') {
                $reason = if ($line.Length -gt 140) { $line.Substring(0, 140) + '...' } else { $line }
                break
            }
        }
        Write-Output ("  {0}  {1}" -f $t.id, $reason)
    }
}
