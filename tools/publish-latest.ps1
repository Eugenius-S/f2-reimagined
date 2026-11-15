# Called only inside verify.yml's fixed publication concurrency group.
$ErrorActionPreference = 'Stop'
$repo = $env:GITHUB_REPOSITORY
$markerName = 'publish-run-number.txt'
$runNumber = 0L
if ($env:GITHUB_RUN_NUMBER -notmatch '\A[1-9][0-9]*\z' -or
    -not [long]::TryParse($env:GITHUB_RUN_NUMBER, [ref]$runNumber)) {
    throw 'Invalid publication run number'
}

# Concurrency serializes publishers but does not guarantee dispatch order.
$treeJson = gh api "repos/$repo/git/trees/dist"
if ($LASTEXITCODE -ne 0) { throw 'Cannot read the published dist tree' }
$tree = $treeJson | ConvertFrom-Json
if ($tree.truncated -isnot [bool] -or $tree.truncated -or $tree.tree -isnot [array] -or
    $tree.sha -notmatch '\A[0-9a-f]{40}\z') { throw 'Invalid published dist tree' }
foreach ($entry in $tree.tree) {
    if ($entry.path -isnot [string] -or [string]::IsNullOrEmpty($entry.path) -or
        $entry.sha -notmatch '\A[0-9a-f]{40}\z' -or $entry.type -notin 'blob','tree','commit') {
        throw 'Invalid published dist tree entry'
    }
}
$markers = @($tree.tree | Where-Object { $_.path -eq $markerName })
$publishedNumber = 0L
if ($markers.Count -gt 0) {
    if ($markers.Count -ne 1 -or $markers[0].type -ne 'blob' -or $markers[0].mode -ne '100644' -or
        $markers[0].sha -notmatch '\A[0-9a-f]{40}\z') { throw 'Invalid publication watermark entry' }
    $blobJson = gh api "repos/$repo/git/blobs/$($markers[0].sha)"
    if ($LASTEXITCODE -ne 0) { throw 'Cannot read the publication watermark' }
    $blob = $blobJson | ConvertFrom-Json
    if ($blob.encoding -ne 'base64' -or $blob.content -isnot [string]) { throw 'Invalid publication watermark blob' }
    $watermark = [Text.Encoding]::UTF8.GetString([Convert]::FromBase64String($blob.content))
    if ($watermark -notmatch '\A[1-9][0-9]*\z' -or
        -not [long]::TryParse($watermark, [ref]$publishedNumber)) { throw 'Invalid publication watermark' }
}
# Only a successful tree read with no marker is a legacy publication.
if ($runNumber -le $publishedNumber) {
    Write-Host "Skipping publication ${runNumber}: dist already holds publication $publishedNumber"
    return
}

Compress-Archive -Path staging/* -DestinationPath fallout2-ce-3d-win64.zip -Force
# The launcher unpacks these six by name; check before uploading.
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::OpenRead((Join-Path (Get-Location) 'fallout2-ce-3d-win64.zip'))
try {
    $names = $zip.Entries | ForEach-Object { $_.FullName } | Where-Object { $_ }
    $want = 'fallout2-ce.exe', 'fallout2-ce-3d-launcher.exe', 'ce.dat', 'fallout2.cfg', 'version.txt', 'scene-exporter.exe'
    $missing = @($want | Where-Object { $_ -notin $names })
    "zip contains: $($names -join ', ')"
    if ($missing.Count -gt 0) { throw "the release zip is missing: $($missing -join ', ')" }
} finally {
    $zip.Dispose()
}

# A failed read is not evidence that the release is absent.
$releaseTags = @(gh api "repos/$repo/releases" --paginate --jq '.[].tag_name')
if ($LASTEXITCODE -ne 0) { throw 'Cannot list releases' }
$assets = @()
if ($releaseTags -notcontains 'latest') {
    gh release create latest --repo $repo --target $env:BUILD_COMMIT `
        --title 'Latest build' `
        --notes "Rolling build of main. Run Install-F2R.exe to install it onto your Fallout 2; afterwards start the game with Fallout 2 Reimagined.exe in the game folder; the launcher makes the 3D scene data from your own game files."
    if ($LASTEXITCODE -ne 0) { throw 'Cannot create the latest release' }
} else {
    $releaseJson = gh release view latest --repo $repo --json assets
    if ($LASTEXITCODE -ne 0) { throw 'Cannot read latest release assets' }
    $release = $releaseJson | ConvertFrom-Json
    if ($release.assets -isnot [array]) { throw 'Invalid latest release assets' }
    $assets = $release.assets
}
# Keep the download name and the ZIP's in-place-update launcher name unchanged.
Copy-Item staging/fallout2-ce-3d-launcher.exe Install-F2R.exe
gh release upload latest fallout2-ce-3d-win64.zip staging/version.txt Install-F2R.exe --repo $repo --clobber
if ($LASTEXITCODE -ne 0) { throw 'Cannot upload the latest release assets' }
if ($assets.name -contains 'fallout2-ce-3d-launcher.exe') {
    gh release delete-asset latest fallout2-ce-3d-launcher.exe --repo $repo --yes
    if ($LASTEXITCODE -ne 0) { throw 'Cannot remove the obsolete launcher asset' }
}
gh release edit latest --repo $repo --title "Latest build $((Get-Date).ToUniversalTime().ToString('yyyy-MM-dd'))" `
    --notes "Rolling build of main. Run Install-F2R.exe to install it onto your Fallout 2; afterwards start the game with Fallout 2 Reimagined.exe in the game folder; the launcher makes the 3D scene data from your own game files."
if ($LASTEXITCODE -ne 0) { throw 'Cannot update the latest release notes' }

# The orphan dist branch remains the VPN fallback (235), including RPU (238).
if ((Get-Item fallout2-ce-3d-win64.zip).Length -gt 90MB) { throw 'the release zip is too big for the dist mirror' }
$dist = Join-Path $env:RUNNER_TEMP 'dist'
New-Item -ItemType Directory $dist | Out-Null
Copy-Item fallout2-ce-3d-win64.zip, staging/version.txt $dist
$rpu = Join-Path $env:RUNNER_TEMP 'rpu'
New-Item -ItemType Directory $rpu | Out-Null
$rpuRepo = 'BGforgeNet/Fallout2_Restoration_Project'
$tag = (gh release view --repo $rpuRepo --json tagName --jq .tagName)
if ($LASTEXITCODE -eq 0 -and $tag) {
    gh release download --repo $rpuRepo --pattern 'rpu_*_translations.zip' --dir $rpu --clobber
}
$rpuZip = Get-ChildItem $rpu -Filter 'rpu_*_translations.zip' | Select-Object -First 1
if ($LASTEXITCODE -eq 0 -and $rpuZip) {
    Expand-Archive $rpuZip.FullName -DestinationPath (Join-Path $rpu 'x') -Force
    $dat = Get-ChildItem (Join-Path $rpu 'x') -Recurse -Filter rpu_russian.dat | Select-Object -First 1
    if (-not $dat) { throw "rpu_russian.dat is not in $($rpuZip.Name)" }
    Copy-Item $dat.FullName (Join-Path $dist 'rpu_russian.dat')
    Set-Content -Path (Join-Path $dist 'rpu-version.txt') -Value $tag -NoNewline
} else {
    Write-Warning 'RPU release download failed; keeping the previous dist copy'
    $raw = "https://raw.githubusercontent.com/$repo/dist"
    foreach ($name in 'rpu_russian.dat', 'rpu-version.txt') {
        Invoke-WebRequest "$raw/$name" -OutFile (Join-Path $dist $name)
    }
}
if (-not (Test-Path (Join-Path $dist 'rpu_russian.dat')) -or (Get-Item (Join-Path $dist 'rpu_russian.dat')).Length -lt 1MB) {
    throw 'the dist mirror has no usable rpu_russian.dat'
}
# Publish the watermark atomically with dist only after every release output succeeded.
Set-Content -Path (Join-Path $dist $markerName) -Value $env:GITHUB_RUN_NUMBER -Encoding ascii -NoNewline
Set-Location $dist
git init -q -b dist
if ($LASTEXITCODE -ne 0) { throw 'Cannot initialize dist' }
git config user.name github-actions
if ($LASTEXITCODE -ne 0) { throw 'Cannot configure dist author' }
git config user.email "$($env:GITHUB_ACTOR_ID)+$($env:GITHUB_ACTOR)@users.noreply.github.com"
if ($LASTEXITCODE -ne 0) { throw 'Cannot configure dist author email' }
git add .
if ($LASTEXITCODE -ne 0) { throw 'Cannot stage dist' }
git commit -q -m "dist $env:BUILD_COMMIT"
if ($LASTEXITCODE -ne 0) { throw 'Cannot commit dist' }
git push -q --force "https://x-access-token:$($env:GH_TOKEN)@github.com/$repo.git" dist
if ($LASTEXITCODE -ne 0) { throw 'Cannot push dist' }
