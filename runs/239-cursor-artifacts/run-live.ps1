# 239: run one live (visible window, real SDL mouse) game with the scripted
# input and capture the window via PrintWindow, bound to this process only.
param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [Parameter(Mandatory = $true)][string]$OutDir,
    [string]$Map = 'kladwtwn',
    [int]$Seconds = 45,
    [int]$IntervalMs = 120,
    # Offscreen window that never stalls on lost focus (other agents' windows
    # take the foreground and a focused-only run freezes in _GNW95_lost_focus).
    [switch]$Background,
    [string[]]$Extra = @(),
    [string]$InputFile = '',
    [switch]$NoDevOut
)
$ErrorActionPreference = 'Stop'
$repo = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$runDir = Join-Path $repo 'build\test-run'
Copy-Item -LiteralPath $Exe -Destination (Join-Path $runDir 'fallout2-ce-test.exe') -Force
Copy-Item -LiteralPath (Join-Path $repo 'tools\test-config\fallout2.cfg') -Destination (Join-Path $runDir 'fallout2.cfg') -Force

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class W239 {
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    public delegate bool EnumProc(IntPtr h, IntPtr l);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc f, IntPtr l);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
    // The background run's window skips the taskbar, so .NET sees no main
    // window: take the process's largest visible top-level window instead.
    public static IntPtr FindFor(uint pid) {
        IntPtr best = IntPtr.Zero; long bestArea = 0;
        EnumWindows((h, l) => {
            uint owner; GetWindowThreadProcessId(h, out owner);
            if (owner != pid || !IsWindowVisible(h)) return true;
            RECT r; GetWindowRect(h, out r);
            long area = (long)(r.R - r.L) * (r.B - r.T);
            if (area > bestArea) { bestArea = area; best = h; }
            return true;
        }, IntPtr.Zero);
        return best;
    }
    public struct RECT { public int L; public int T; public int R; public int B; }
}
"@
# Without this GetWindowRect reports DPI-virtualized sizes and the capture
# keeps only the top-left part of the window.
[W239]::SetProcessDPIAware() | Out-Null
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$env:F2CE_SCENE_CACHE = (Join-Path $repo 'build\scene-cache')
$env:F2CE_DEV_OUT = $OutDir
if ($Background) { $env:F2CE_DEV_BACKGROUND = '1' } else { Remove-Item Env:F2CE_DEV_BACKGROUND -ErrorAction SilentlyContinue }
if ($NoDevOut) { Remove-Item Env:F2CE_DEV_OUT -ErrorAction SilentlyContinue }
$inputFile = if ($InputFile) { $InputFile } else { Join-Path $PSScriptRoot 'live\input.txt' }
$args2 = @('--dev-tools', '--dev-new-game', "--dev-map=$Map", '--dev-elevation=0')
if ($InputFile -ne 'none') { $args2 += "--dev-scene-input=$inputFile" }
if ($Background) { $args2 += @('--dev-screen=1920x1080', '--dev-scale=2') }
$args2 += $Extra
$p = Start-Process -FilePath (Join-Path $runDir 'fallout2-ce-test.exe') -ArgumentList $args2 `
    -WorkingDirectory (Join-Path $repo 'game') -PassThru `
    -RedirectStandardOutput (Join-Path $OutDir 'stdout.txt') -RedirectStandardError (Join-Path $OutDir 'stderr.txt')
$deadline = (Get-Date).AddSeconds($Seconds)
$i = 0
try {
    while ((Get-Date) -lt $deadline -and -not $p.HasExited) {
        $h = [W239]::FindFor([uint32]$p.Id)
        if ($h -ne 0) {
            $r = New-Object W239+RECT
            [W239]::GetWindowRect($h, [ref]$r) | Out-Null
            $w = $r.R - $r.L; $hh = $r.B - $r.T
            if ($w -gt 0 -and $hh -gt 0) {
                $bmp = New-Object System.Drawing.Bitmap($w, $hh)
                $g = [System.Drawing.Graphics]::FromImage($bmp)
                $dc = $g.GetHdc()
                [W239]::PrintWindow($h, $dc, 2) | Out-Null
                $g.ReleaseHdc($dc); $g.Dispose()
                $bmp.Save((Join-Path $OutDir ("frame-{0:d4}.png" -f $i)), [System.Drawing.Imaging.ImageFormat]::Png)
                $bmp.Dispose()
                $i++
            }
        }
        Start-Sleep -Milliseconds $IntervalMs
    }
} finally {
    if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force }
}
Write-Output "pid $($p.Id) saved $i frames"
