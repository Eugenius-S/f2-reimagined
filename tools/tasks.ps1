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
    [switch]$SelfTest,
    # 300: validate the headers and dependency graph of the checked-out
    # tasks\*.md (or -TasksDir for the fixtures); exit nonzero on any hard
    # metadata error. Legacy gaps (an old task without priority or tier) are
    # named but do not fail the check.
    [string]$TasksDir,
    [switch]$CheckMetadata
)
$ErrorActionPreference = 'Stop'

function Read-Header([string[]]$lines) {
    $header = @{}
    if ($lines.Count -eq 0 -or $lines[0] -ne '---') { return $null }
    for ($i = 1; $i -lt $lines.Count -and $lines[$i] -ne '---'; $i++) {
        if ($lines[$i] -match '^([a-z-]+):\s*(.*?)\s*(#.*)?$') { $header[$Matches[1]] = $Matches[2] }
    }
    return $header
}

# A terminal main card owns its entire body, including its Log.
function Resolve-TaskLines([string[]]$MainLines, [string]$Branch, [string]$File, [scriptblock]$ReadBranch) {
    $mainHeader = Read-Header $MainLines
    if ($Branch -and $mainHeader -and $mainHeader['status'] -notin 'done', 'blocked') {
        $fromBranch = @(& $ReadBranch $Branch $File)
        if ($fromBranch.Count -gt 0) { return $fromBranch }
    }
    return $MainLines
}

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
    # A draft PR is a lock for a live task, but it does not revive a task
    # that has been explicitly stopped as blocked.
    if ($Status -eq 'blocked') { return 'blocked' }
    if ($null -ne $PrNumber) { return 'live' }
    $fresh = ($null -ne $BranchTime) -and (($Now - [long]$BranchTime) -le $StaleDays * 86400)
    if ($fresh) { return 'live' }
    if ($Status -in 'todo', 'doing') {
        if ($null -ne $BranchTime -or $Status -eq 'doing') { return 'stale' }
    }
    return 'plain'
}

# 300: the task metadata validator. Hard errors fail the check (a duplicate
# numeric id, an id that does not match its file name, an unknown or
# self dependency, a dependency cycle, a status or field value outside the
# board's own vocabulary). Legacy gaps - an old task without a priority or
# tier, or the historical 'medium' priority - are named but never fail:
# the owner's priorities and assignments are not rewritten here.
function Test-TaskMetadata {
    param([string]$TasksDir)
    $errors = [System.Collections.Generic.List[string]]::new()
    $legacy = [System.Collections.Generic.List[string]]::new()
    $statuses = @('todo', 'doing', 'blocked', 'done')
    $owners = @('claude', 'gpt', 'glm', 'any', 'none', 'owner')
    # 'medium' is a historical value the board carries; legal, but named.
    $priorities = @('high', 'normal', 'low', 'medium')

    $tasks = [ordered]@{}
    $seen = @{}
    Get-ChildItem (Join-Path $TasksDir '*.md') | Sort-Object Name | ForEach-Object {
        $name = $_.Name
        if ($name -eq 'inbox.md') { return }  # the owner's finds inbox, not a task
        if ($name -notmatch '^(\d+)-') {
            $errors.Add("$name : the file name does not start with a numeric task id")
            return
        }
        $id = $Matches[1]
        # Two files claiming one numeric id: the evidence must survive the
        # parse (a later file would otherwise silently overwrite the key).
        if ($seen.ContainsKey($id)) {
            $errors.Add("$name : duplicate numeric id $id (also $($seen[$id]))")
        } else { $seen[$id] = $name }
        $lines = Get-Content -LiteralPath $_.FullName
        if ($lines.Count -eq 0 -or $lines[0] -ne '---') {
            $errors.Add("$name : no frontmatter header")
            return
        }
        $header = @{}
        for ($i = 1; $i -lt $lines.Count -and $lines[$i] -ne '---'; $i++) {
            if ($lines[$i] -match '^([a-z-]+):\s*(.*?)\s*(#.*)?$') { $header[$Matches[1]] = $Matches[2] }
        }
        foreach ($field in @('id', 'title', 'status', 'owner', 'depends-on')) {
            if (-not $header.ContainsKey($field)) { $errors.Add("$name : missing required field $field") }
        }
        if ($header.ContainsKey('id') -and $header['id'] -ne $id) {
            $errors.Add("$name : header id '$($header['id'])' does not match the file name's id $id")
        }
        if ($header.ContainsKey('status') -and $statuses -notcontains $header['status']) {
            $errors.Add("$name : status '$($header['status'])' is not one of $($statuses -join ', ')")
        }
        if ($header.ContainsKey('owner') -and $owners -notcontains $header['owner']) {
            $errors.Add("$name : owner '$($header['owner'])' is not one of $($owners -join ', ')")
        }
        if ($header.ContainsKey('priority')) {
            if ($priorities -notcontains $header['priority']) {
                $errors.Add("$name : priority '$($header['priority'])' is not one of $($priorities -join ', ')")
            }
            if ($header['priority'] -eq 'medium') { $legacy.Add("$name : historical 'medium' priority") }
        } else {
            $legacy.Add("$name : no priority (a task from before the field existed)")
        }
        if ($header.ContainsKey('tier')) {
            if ($header['tier'] -notmatch '^\d+$' -or [int]$header['tier'] -gt 4) {
                $errors.Add("$name : tier '$($header['tier'])' is not 0-4")
            }
        } else {
            $legacy.Add("$name : no tier (a task from before the field existed)")
        }
        foreach ($field in @('claude-only', 'user-check', 'reviewed')) {
            if ($header.ContainsKey($field) -and $header[$field] -notin 'yes', 'no') {
                $errors.Add("$name : $field '$($header[$field])' is not yes or no")
            }
        }
        $tasks[$id] = @{ name = $name; deps = @() ; header = $header }
        $raw = if ($header.ContainsKey('depends-on')) { $header['depends-on'] } else { '' }
        foreach ($dep in ($raw -split '[,\s]+' | Where-Object { $_ } | ForEach-Object { $_.Trim('[]') } | Where-Object { $_ })) {
            $tasks[$id].deps += $dep
        }
    }
    # Dependencies must exist, not self-reference, and not cycle.
    foreach ($id in $tasks.Keys) {
        foreach ($dep in $tasks[$id].deps) {
            if (-not $tasks.Contains($dep)) {
                $errors.Add("$($tasks[$id].name) : depends-on unknown task $dep")
            } elseif ($dep -eq $id) {
                $errors.Add("$($tasks[$id].name) : depends on itself")
            }
        }
    }
    $color = @{}
    foreach ($start in $tasks.Keys) {
        $stack = [System.Collections.Generic.List[string]]::new()
        function Walk([string]$node) {
            if ($color.ContainsKey($node)) { return $color[$node] -eq 1 }
            $color[$node] = 1
            [void]$stack.Add($node)
            foreach ($dep in $tasks[$node].deps) {
                if ($tasks.Contains($dep) -and (Walk $dep)) {
                    $errors.Add("$($tasks[$node].name) : dependency cycle through $dep")
                }
            }
            # List.Remove returns a bool that must not join the function's
            # pipeline: it would make every call return a truthy array.
            [void]$stack.Remove($node)
            $color[$node] = 2
            return $false
        }
        [void](Walk $start)
    }
    return @{ Errors = $errors; Legacy = $legacy; Tasks = $tasks.Count }
}

if ($CheckMetadata) {
    $dir = if ($TasksDir) { $TasksDir } else { Join-Path (Split-Path -Parent $PSScriptRoot) 'tasks' }
    $result = Test-TaskMetadata -TasksDir $dir
    foreach ($gap in $result.Legacy) { Write-Output "TASKS legacy: $gap" }
    foreach ($err in $result.Errors) { Write-Output "TASKS ERROR $err" }
    if ($result.Errors.Count -gt 0) {
        Write-Output ("TASKS FAIL {0} metadata errors" -f $result.Errors.Count)
        exit 1
    }
    Write-Output ("TASKS ok ({0} tasks, {1} legacy gaps named)" -f $result.Tasks, $result.Legacy.Count)
    exit 0
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
        @{ Id = '207'; Status = 'doing'; BranchTime = $null; PrNumber = $null; Want = 'stale' },
        # A blocked task stays unavailable even when its draft PR is open.
        @{ Id = '248'; Status = 'blocked'; BranchTime = $null; PrNumber = 414; Want = 'blocked' }
    )
    $failed = $false
    foreach ($c in $cases) {
        $got = Resolve-LockState -Id $c.Id -Status $c.Status -BranchTime $c.BranchTime `
            -PrNumber $c.PrNumber -Now $now -StaleDays 3
        if ($got -ne $c.Want) {
            Write-Output ("selftest: FAIL id={0} want={1} got={2}" -f $c.Id, $c.Want, $got)
            $failed = $true
        }
        if ($c.Id -eq '248' -and $c.PrNumber -eq 414 -and $got -in 'todo', 'doing', 'stale') {
            Write-Output 'selftest: FAIL blocked task with an open draft PR became selectable'
            $failed = $true
        }
    }
    # Exercise the board's card resolver, including its branch read and fallback.
    foreach ($case in @(
        @{ Main = 'blocked'; Branch = 'doing'; Want = 'blocked'; Log = 'main log'; Reads = 0 },
        @{ Main = 'done'; Branch = 'doing'; Want = 'done'; Log = 'main log'; Reads = 0 },
        @{ Main = 'todo'; Branch = 'blocked'; Want = 'blocked'; Log = 'branch log'; Reads = 1 },
        @{ Main = 'todo'; Branch = 'doing'; Want = 'doing'; Log = 'branch log'; Reads = 1 },
        @{ Main = 'todo'; Branch = ''; Want = 'todo'; Log = 'main log'; Reads = 1 }
    )) {
        $mainLines = @('---', "status: $($case.Main)", '---', 'main log')
        $script:branchReads = 0
        $reader = {
            param($branch, $file)
            $script:branchReads++
            if ($case.Branch) { @('---', "status: $($case.Branch)", '---', 'branch log') }
        }
        $resolved = @(Resolve-TaskLines $mainLines 'task/fixture' 'tasks/fixture.md' $reader)
        $resolvedHeader = Read-Header $resolved
        if ($resolvedHeader['status'] -ne $case.Want -or $resolved[-1] -ne $case.Log -or $script:branchReads -ne $case.Reads) {
            Write-Output ("selftest: FAIL main={0} branch={1} precedence" -f $case.Main, $case.Branch)
            $failed = $true
        }
    }
    # 300: the metadata validator against synthetic fixtures.
    function Assert-True([string]$what, [bool]$got) {
        if (-not $got) {
            Write-Output ("selftest: FAIL {0}" -f $what)
            $script:failed = $true
        }
    }
    $fixtureRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("f2r-tasks-selftest-" + [guid]::NewGuid().ToString('N'))
    $debugFixture = $true
    New-Item -ItemType Directory -Path $fixtureRoot -Force | Out-Null
    function Write-Task([string]$name, [string]$body) {
        Set-Content -LiteralPath (Join-Path $fixtureRoot $name) -Value $body -Encoding ascii
    }
    Write-Task '001-valid.md' @'
---
id: 001
title: A valid task
status: todo
owner: any
priority: high
tier: 1
depends-on: [002]
---
body
'@
    Write-Task '002-valid-blocked.md' @'
---
id: 002
title: A legal blocked task
status: blocked
owner: glm
priority: normal
tier: 0
depends-on: []
---
body
'@
    $valid = Test-TaskMetadata -TasksDir $fixtureRoot
    Assert-True 'a valid pair with a legal blocked status has no errors' ($valid.Errors.Count -eq 0)
    Assert-True 'the valid pair still names nothing' ($valid.Legacy.Count -eq 0)

    # A true duplicate: two files claiming the numeric id 002.
    Write-Task '002-dup.md' @'
---
id: 002
title: A duplicate numeric id
status: todo
owner: any
priority: low
tier: 2
depends-on: []
---
body
'@
    $dup = Test-TaskMetadata -TasksDir $fixtureRoot
    Assert-True 'a duplicate numeric id fails' (($dup.Errors | Where-Object { $_ -like '*duplicate numeric id*' }).Count -gt 0)

    Remove-Item -LiteralPath (Join-Path $fixtureRoot '002-dup.md') -Force
    Write-Task '003-unknown-dep.md' @'
---
id: 003
title: An unknown dependency
status: todo
owner: any
priority: low
tier: 2
depends-on: [999]
---
body
'@
    $unknown = Test-TaskMetadata -TasksDir $fixtureRoot
    Assert-True 'an unknown dependency fails' (($unknown.Errors | Where-Object { $_ -like '*unknown task 999*' }).Count -gt 0)

    Set-Content -LiteralPath (Join-Path $fixtureRoot '003-unknown-dep.md') -Value @'
---
id: 003
title: A self dependency
status: doing
owner: any
priority: low
tier: 2
depends-on: [003]
---
body
'@ -Encoding ascii
    $self = Test-TaskMetadata -TasksDir $fixtureRoot
    Assert-True 'a self dependency fails' (($self.Errors | Where-Object { $_ -like '*depends on itself*' }).Count -gt 0)

    Set-Content -LiteralPath (Join-Path $fixtureRoot '003-cycle.md') -Value @'
---
id: 003
title: One half of a cycle
status: todo
owner: any
priority: low
tier: 2
depends-on: [001]
---
body
'@ -Encoding ascii
    $cycle = Test-TaskMetadata -TasksDir $fixtureRoot
    Assert-True 'a dependency cycle fails' (($cycle.Errors | Where-Object { $_ -like '*dependency cycle*' }).Count -gt 0)

    # The legacy check runs against the clean pair again: the earlier
    # deliberate errors must not leak into it.
    Remove-Item -LiteralPath (Join-Path $fixtureRoot '003-unknown-dep.md') -Force
    Remove-Item -LiteralPath (Join-Path $fixtureRoot '003-cycle.md') -Force
    Write-Task '004-legacy.md' @'
---
id: 004
title: An old task without priority or tier
status: todo
owner: any
depends-on: []
---
body
'@
    $legacy = Test-TaskMetadata -TasksDir $fixtureRoot
    Assert-True 'a legacy task without priority or tier is not a hard error' ($legacy.Errors.Count -eq 0)
    Assert-True 'the legacy gap is named' (($legacy.Legacy | Where-Object { $_ -like '*004-legacy.md*' }).Count -gt 0)

    Remove-Item -LiteralPath $fixtureRoot -Recurse -Force
    if ($failed) { exit 1 }
    Write-Output ("selftest: ok ({0} lock cases and 5 card precedence cases)" -f $cases.Count)
    exit 0
}

$Root = Split-Path -Parent $PSScriptRoot

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
    $lines = @(Resolve-TaskLines @($lines) $branchFile $file {
        param($branchName, $taskFile)
        git show "origin/$branchName`:$taskFile" 2>$null
    })
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
