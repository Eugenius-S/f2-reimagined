# The captures dev-snapshot.ps1 puts into an -OutputDirectory, and the state of
# the run that put them there (task 301). Dot-sourced by dev-snapshot.ps1.
#
# snapshot-output.json in that folder lists the files the last run copied in
# (its devshot JSON sidecars and their images), each with its SHA-256, and a
# state: "incomplete" from the start of a run, "complete" only once every
# check passed. A new run first deletes the listed files whose hash still
# matches; a file changed since, any other file and the folder stay. A failed
# run exits nonzero and leaves "incomplete" with only its own files listed,
# so no earlier capture can pass for its result.

$SnapshotOutputManifest = 'snapshot-output.json'
$script:snapshotOutputFiles = [ordered]@{}

# Windows PowerShell children launched by the tour may lack the Utility cmdlet.
function Get-Sha256FileHash([string]$Path) {
    $algorithm = [System.Security.Cryptography.SHA256]::Create()
    $stream = [System.IO.File]::OpenRead($Path)
    try {
        return [System.BitConverter]::ToString($algorithm.ComputeHash($stream)).Replace('-', '')
    } finally {
        $stream.Dispose()
        $algorithm.Dispose()
    }
}

function Write-SnapshotOutput([string]$Directory, [string]$State) {
    $path = Join-Path $Directory $SnapshotOutputManifest
    $text = [ordered]@{ state = $State; files = $script:snapshotOutputFiles } | ConvertTo-Json
    # No BOM: Python callers read the folder's JSON files as plain UTF-8.
    [IO.File]::WriteAllText("$path.tmp", $text)
    Move-Item -LiteralPath "$path.tmp" -Destination $path -Force
}

function Reset-SnapshotOutput([string]$Directory) {
    New-Item -ItemType Directory -Path $Directory -Force | Out-Null
    $path = Join-Path $Directory $SnapshotOutputManifest
    if (Test-Path -LiteralPath $path) {
        try {
            $previous = Get-Content -Raw -LiteralPath $path | ConvertFrom-Json
        } catch {
            throw "Unreadable $path; nothing was deleted. Move it away to start this folder afresh."
        }
        foreach ($entry in @($previous.files.PSObject.Properties)) {
            $name = [string]$entry.Name
            if ($name -ne [IO.Path]::GetFileName($name)) { continue }
            $file = Join-Path $Directory $name
            if ((Test-Path -LiteralPath $file -PathType Leaf) -and
                (Get-Sha256FileHash $file) -eq [string]$entry.Value) {
                Remove-Item -LiteralPath $file -Force
            }
        }
    }
    Write-SnapshotOutput $Directory 'incomplete'
}

function Add-SnapshotOutput([string]$Directory, [string]$Path) {
    $name = Split-Path -Leaf $Path
    $script:snapshotOutputFiles[$name] = (Get-Sha256FileHash (Join-Path $Directory $name))
    Write-SnapshotOutput $Directory 'incomplete'
}

function Complete-SnapshotOutput([string]$Directory) {
    Write-SnapshotOutput $Directory 'complete'
}

# "probe done" is a terminal diagnostic, not proof of a successful walk.
# A named-map exit needs the expected target and the actual arrival map.
function Get-SnapshotExitProbeStatus([string]$Path, [string]$Destination) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return 'pending' }
    $text = Get-Content -Raw -LiteralPath $Path
    $name = [regex]::Escape($Destination)
    if ($text -match ('(?m)^exit transition passed: [^\r\n]+ target=' + $name + ' map=' + $name + '\.(?:MAP|SAV) ')) {
        return 'passed'
    }
    if ($text -match '(?m)^(?:exit probe failed:|outside probe failed:|probe done\b)') {
        return 'failed'
    }
    return 'pending'
}

function Test-SnapshotExitArrival([string]$Path, [string]$Destination) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $false }
    return Select-String -LiteralPath $Path -Quiet -Pattern ('^first-3d-capture result=pass map=' + [regex]::Escape($Destination) + ' elevation=')
}
