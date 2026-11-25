# 326: the ground bake repeats only when its picture changes

Repro spot: KLADWTWN e0, hero 18479, camera 18271, yaw -123.69, zoom 1.024,
scene-gl, sun shadows, glow 2.0, 1720x720 logical at scale 2.0 (the owner's
shot `20261124-234309-kladwtwn-e0` on build 1c893f5af). The owner's live
window reads 47.8 FPS / 20.92 ms: VSync on a 144 Hz panel presents any frame
cost over 13.89 ms as 3 vblank intervals (20.83 ms). The background harness
measured the frame at 14.2-15.6 ms, just over that budget.

Cause: the 157 ground bake (half-resolution floor render with the light loop
and the 25-tap shadow disk) re-rendered every frame. With the bake off the
floor pass costs 14.8 ms instead of 6.0, so the bake pass was about 5 ms of
every frame while its inputs stood still.

Fix: the bake keeps a key over what its picture reads (camera matrix, bake
size, floor texture, receiver group, shadow-map redraw, light inputs, sun
direction and strength) and re-renders only when the key changes; cloud
shadows keep the old behavior (their offset moves every frame).

## Numbers (same harness, same settings, one run at a time)

Before (main 1c893f5af): 70.5 / 68.1 / 64.0 FPS, frame average
14.19 / 14.69 / 15.61 ms, 1% low 37.7 / 47.0 / 47.6 ms.

Previous release (pre-586, 039d25da3): 68.0 / 68.6 / 66.9 FPS, frame average
14.71 / 14.58 / 14.95 ms: a standing cost, not a regression.

After: 83.7 / 78.5 / 85.5 FPS, frame average 11.95 / 12.74 / 11.69 ms, 1% low
36.0 / 35.2 / 53.3 ms. GPU frame 12.0-12.8 ms (floor 3.4-3.7), under the
13.89 ms two-vblank budget: the owner's window presents in 2 intervals, about
72 FPS sustained on the same spot.

F2CE_NO_BAKE=1 still disables the bake entirely (floor 14.8 ms).

## Picture

Still A/B devshots (same args, ambient pinned), clean main exe vs this
branch, byte-identical PNGs (sha256):

- KLADWTWN e0 hero 18479 view 18271 zoom 1.024:
  `a455a17eeb5fc9a68126d61a1a838b55d0afe2c4814a7221017fbc8665ddd0fa`
- KLAMALL e0 hero 15450 view 15450:
  `1fc3bf5a4b7bcb458053ea2ad3b00bf466d5ba503a9bd440afd8ca2373b98d86`

The vanilla fly tour stills run with shadows off (devshot default), so the
bake never renders there and the tour cannot gate this change; the A/B pairs
above carry the shadow-on pixels instead.

## Repro

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/dev-snapshot.ps1 `
  -GameRoot build/test-install -BuildExecutable build/fallout2-ce.exe `
  -Map kladwtwn -Elevation 0 -HeroTile 18479 -ViewTile 18271 -Zoom 1.024 `
  -SunShadows -Glow -ScreenWidth 3440 -ScreenHeight 1440 -Scale 2.0 `
  -OwnerShot -OwnerShotDelay 10 -HoldSeconds 16
```

(the owner's logical 1720x720 at scale 2.0 renders the same 3440x1440
backing; dev-snapshot's minimum logical framebuffer is 640x480, so the
equivalent screen form is used)
