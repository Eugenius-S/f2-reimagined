# Exercise the real publisher against fake remote services and disposable payloads.
$ErrorActionPreference = 'Stop'
$publisher = Join-Path $PSScriptRoot 'publish-latest.ps1'
$originalLocation = Get-Location
$tempParent = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$testRoot = [IO.Path]::GetFullPath((Join-Path $tempParent ('f2r-publish-test-' + [guid]::NewGuid())))
if (-not $testRoot.StartsWith($tempParent, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe test root' }
New-Item -ItemType Directory $testRoot | Out-Null
$env:GITHUB_REPOSITORY = 'fixture/repository'
$env:GITHUB_ACTOR_ID = '1'
$env:GITHUB_ACTOR = 'fixture'
$env:GH_TOKEN = 'fixture'
$global:publishFixtureCheckCount = 0

function Check([bool]$ok, [string]$message) {
    if (-not $ok) { throw $message }
    $global:publishFixtureCheckCount++
}
function Reset-Remote {
    $global:publishFixture = @{
        marker = $null; latest = 'legacy'; dist = 'legacy'; release = $true
        fail = ''; malformedTree = $false; malformedTreeEntry = $false; malformedBlob = $false
        legacyAsset = $true; writes = 0; calls = @(); rpuFallback = $false
        stagedDist = $null; stagedMarker = $null; stagedLauncherHash = $null
        committedDist = $null; committedMarker = $null; committedLauncherHash = $null
        latestLauncherHash = $null; distLauncherHash = $null
    }
}
function gh {
    $global:LASTEXITCODE = 0
    $global:publishFixture.calls += ($args -join ' ')
    if ($args[0] -eq 'api') {
        switch ($args[1]) {
            'repos/fixture/repository/git/trees/dist' {
                if ($global:publishFixture.fail -eq 'tree-read') { $global:LASTEXITCODE = 1; return }
                if ($global:publishFixture.malformedTree) { return '{"tree":"damaged","truncated":false}' }
                if ($global:publishFixture.malformedTreeEntry) {
                    return (@{tree=@(@{});truncated=$false;sha=('b' * 40)} | ConvertTo-Json -Depth 4)
                }
                $entries = @()
                if ($null -ne $global:publishFixture.marker) {
                    $entries = @(@{path='publish-run-number.txt';type='blob';mode='100644';sha=('a' * 40)})
                }
                return (@{tree=$entries;truncated=$false;sha=('b' * 40)} | ConvertTo-Json -Depth 4)
            }
            ('repos/fixture/repository/git/blobs/' + ('a' * 40)) {
                if ($global:publishFixture.fail -eq 'blob-read') { $global:LASTEXITCODE = 1; return }
                if ($global:publishFixture.malformedBlob) { return '{"encoding":"base64","content":"!"}' }
                return (@{encoding='base64';content=[Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes($global:publishFixture.marker))} | ConvertTo-Json)
            }
            'repos/fixture/repository/releases' {
                if ($global:publishFixture.fail -eq 'release-list') { $global:LASTEXITCODE = 1; return }
                if ($global:publishFixture.release) { return 'latest' }
                return
            }
            default { throw "Unexpected fake gh API: $args" }
        }
    }
    if ($args[0] -ne 'release') { throw "Unexpected fake gh command: $args" }
    $action = $args[1]
    if ($global:publishFixture.fail -eq "release-$action") {
        if ($action -eq 'upload') { $global:publishFixture.latest = 'partial-upload'; $global:publishFixture.writes++ }
        $global:LASTEXITCODE = 1
        return
    }
    switch ($action) {
        'view' {
            if ($args[2] -eq 'latest') {
                $assets = @()
                if ($global:publishFixture.legacyAsset) { $assets = @(@{name='fallout2-ce-3d-launcher.exe'}) }
                return (@{assets=$assets} | ConvertTo-Json -Depth 3)
            }
            if ($global:publishFixture.rpuFallback) { $global:LASTEXITCODE = 1; return }
            return 'rpu-test'
        }
        'download' {
            $directory = $args[[array]::IndexOf($args, '--dir') + 1]
            $source = Join-Path $directory 'rpu_russian.dat'
            [IO.File]::WriteAllBytes($source, (New-Object byte[] (1MB)))
            Compress-Archive -Path $source -DestinationPath (Join-Path $directory 'rpu_test_translations.zip')
            return
        }
        'create' { $global:publishFixture.release = $true }
        'upload' {
            # Read the actual packaged version, not the incoming environment value.
            $zip = [IO.Compression.ZipFile]::OpenRead((Join-Path (Get-Location) 'fallout2-ce-3d-win64.zip'))
            try {
                $reader = New-Object IO.StreamReader($zip.GetEntry('version.txt').Open())
                try { $global:publishFixture.latest = $reader.ReadToEnd() } finally { $reader.Dispose() }
                $hashEntry = $zip.GetEntry('launcher-sha256.txt')
                Check ($null -ne $hashEntry) 'ZIP does not contain the launcher identity'
                $reader = New-Object IO.StreamReader($hashEntry.Open())
                try { $zipLauncherHash = $reader.ReadToEnd() } finally { $reader.Dispose() }
            } finally { $zip.Dispose() }
            Check ($global:publishFixture.latest -eq [IO.File]::ReadAllText((Join-Path (Get-Location) 'staging/version.txt'))) 'ZIP and release version differ'
            Check ($args -contains 'staging/launcher-sha256.txt') 'Release does not upload the launcher identity'
            $expectedLauncherHash = (Get-FileHash -LiteralPath (Join-Path (Get-Location) 'staging/fallout2-ce-3d-launcher.exe') -Algorithm SHA256).Hash.ToLowerInvariant()
            Check ($zipLauncherHash -eq $expectedLauncherHash) 'ZIP launcher identity differs from its executable'
            $global:publishFixture.latestLauncherHash = [IO.File]::ReadAllText((Join-Path (Get-Location) 'staging/launcher-sha256.txt'))
        }
        'delete-asset' { $global:publishFixture.legacyAsset = $false }
        'edit' { }
        default { throw "Unexpected fake gh release action: $args" }
    }
    $global:publishFixture.writes++
}
function git {
    $global:LASTEXITCODE = 0
    $global:publishFixture.calls += ('git ' + ($args -join ' '))
    if ($global:publishFixture.fail -eq "git-$($args[0])") { $global:LASTEXITCODE = 1; return }
    if ($args[0] -notin 'init','config','add','commit','push') { throw "Unexpected fake git command: $args" }
    if ($args[0] -eq 'add') {
        $global:publishFixture.stagedDist = [IO.File]::ReadAllText((Join-Path (Get-Location) 'version.txt'))
        $global:publishFixture.stagedMarker = [IO.File]::ReadAllText((Join-Path (Get-Location) 'publish-run-number.txt'))
        $global:publishFixture.stagedLauncherHash = [IO.File]::ReadAllText((Join-Path (Get-Location) 'launcher-sha256.txt'))
    }
    if ($args[0] -eq 'commit') {
        $global:publishFixture.committedDist = $global:publishFixture.stagedDist
        $global:publishFixture.committedMarker = $global:publishFixture.stagedMarker
        $global:publishFixture.committedLauncherHash = $global:publishFixture.stagedLauncherHash
    }
    if ($args[0] -eq 'push') {
        # A successful push atomically exposes both dist payload and watermark.
        $global:publishFixture.dist = $global:publishFixture.committedDist
        $global:publishFixture.marker = $global:publishFixture.committedMarker
        $global:publishFixture.distLauncherHash = $global:publishFixture.committedLauncherHash
        $global:publishFixture.writes++
    }
}
function Invoke-WebRequest([string]$Uri, [string]$OutFile) {
    if (-not $global:publishFixture.rpuFallback) { throw "Unexpected network request: $Uri" }
    if ($Uri.EndsWith('/rpu_russian.dat')) { [IO.File]::WriteAllBytes($OutFile, (New-Object byte[] (1MB))) }
    else { [IO.File]::WriteAllText($OutFile, 'previous-rpu') }
}
function Publish([string]$number, [string]$commit, [switch]$Fails) {
    $job = Join-Path $testRoot ([guid]::NewGuid().ToString())
    New-Item -ItemType Directory (Join-Path $job 'staging'), (Join-Path $job 'temp') | Out-Null
    foreach ($name in 'fallout2-ce.exe','fallout2-ce-3d-launcher.exe','ce.dat','fallout2.cfg','version.txt','scene-exporter.exe') {
        [IO.File]::WriteAllText((Join-Path $job "staging/$name"), $commit)
    }
    $env:RUNNER_TEMP = Join-Path $job 'temp'
    $env:GITHUB_RUN_NUMBER = $number
    $env:BUILD_COMMIT = $commit
    Set-Location $job
    $failed = $false
    $failureText = ''
    try { & $publisher | Out-Null } catch { $failed = $true; $failureText = $_.ToString() }
    Check ($failed -eq [bool]$Fails) "Run $number failure=$failed, expected=$Fails, native failure=$($global:publishFixture.fail): $failureText"
}
function Check-Published([string]$number, [string]$commit) {
    Check ($global:publishFixture.marker -eq $number) 'Wrong persisted watermark'
    Check ($global:publishFixture.latest -eq $commit) 'Wrong latest version'
    Check ($global:publishFixture.dist -eq $commit) 'Wrong dist version'
    Check ($global:publishFixture.latestLauncherHash -eq $global:publishFixture.distLauncherHash) 'Release and mirror launcher identities differ'
}

try {
    Reset-Remote
    Publish 11 B
    Check-Published 11 B
    $writes = $global:publishFixture.writes
    Publish 10 A
    Check-Published 11 B
    Check ($global:publishFixture.writes -eq $writes) 'Older run made remote writes'
    Publish 11 C
    Check-Published 11 B
    Check ($global:publishFixture.writes -eq $writes) 'Published rerun made remote writes'

    Reset-Remote
    Publish 10 A
    Check-Published 10 A
    Publish 11 B
    Check-Published 11 B

    Reset-Remote
    $global:publishFixture.release = $false
    Publish 10 A
    Check-Published 10 A
    Check ($global:publishFixture.release) 'Legacy first publication did not create latest'

    foreach ($invalid in '', 'bad', "11`n12", '0', '-1', '9223372036854775808') {
        Reset-Remote
        $global:publishFixture.marker = $invalid
        Publish 12 B -Fails
        Check ($global:publishFixture.writes -eq 0) 'Malformed watermark allowed remote writes'
    }
    foreach ($failure in 'tree-read','blob-read','release-list','release-view') {
        Reset-Remote
        $global:publishFixture.marker = '9'
        $global:publishFixture.fail = $failure
        Publish 11 B -Fails
        Check ($global:publishFixture.writes -eq 0) 'Failed metadata read allowed remote writes'
        Check ($global:publishFixture.marker -eq '9') 'Failed metadata read changed watermark'
    }
    Reset-Remote
    $global:publishFixture.fail = 'tree-read'
    Publish 11 B -Fails
    Check ($global:publishFixture.writes -eq 0) 'Unreadable legacy state allowed remote writes'
    foreach ($bad in 'malformedTree','malformedTreeEntry','malformedBlob') {
        Reset-Remote
        $global:publishFixture.marker = '9'
        $global:publishFixture[$bad] = $true
        Publish 11 B -Fails
        Check ($global:publishFixture.writes -eq 0) 'Malformed metadata allowed remote writes'
    }
    foreach ($invalid in '0','bad') {
        Reset-Remote
        Publish $invalid B -Fails
        Check ($global:publishFixture.writes -eq 0) 'Invalid incoming run number allowed remote writes'
    }
    foreach ($failure in 'release-create','release-upload','release-delete-asset','release-edit','git-init','git-config','git-add','git-commit','git-push') {
        Reset-Remote
        $global:publishFixture.marker = '9'
        if ($failure -eq 'release-create') { $global:publishFixture.release = $false }
        $global:publishFixture.fail = $failure
        Publish 11 B -Fails
        Check ($global:publishFixture.marker -eq '9') "$failure advanced watermark"
        Check ($global:publishFixture.dist -eq 'legacy') "$failure advanced dist"
        $global:publishFixture.fail = ''
        Publish 10 A
        Check-Published 10 A
    }
    Reset-Remote
    $global:publishFixture.rpuFallback = $true
    Publish 10 A
    Check-Published 10 A
    Write-Host "publish-latest: $global:publishFixtureCheckCount checks passed"
    $global:LASTEXITCODE = 0
} finally {
    Set-Location $originalLocation
    if ((Resolve-Path -LiteralPath $testRoot).Path -ne $testRoot -or
        -not $testRoot.StartsWith($tempParent, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe cleanup path' }
    Remove-Item -LiteralPath $testRoot -Recurse -Force
}
