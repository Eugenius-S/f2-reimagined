# 273: Keep local 3D when the decorative roof atlas is damaged

Matched Windows route: start in ARTEMPLE e0, click its stock exit at
21917, and arrive in ARVILLAG e0 at 11683. All controls used the same
original game assets, the same isolated cache files other than the
named damage, and the ordinary game movement and transition handlers.

| ARVILLAG destination cache | Observed result |
| --- | --- |
| Baseline cf94e05c, only roof_fringe.png absent | Arrival succeeded but stayed in the vanilla framebuffer. Renderer reported the missing roof bitmap as a floor error. |
| Candidate, only roof_fringe.png absent | First local full 3D frame, with terrain, hero and walls. One flat-roof fallback diagnostic. |
| Candidate, roof_fringe.png unreadable | Same first local full 3D and one decode diagnostic. |
| Candidate, intact cache | Same first local full 3D, without the fallback diagnostic. |

[First native views](first-normal-arrivals.jpg). The three candidate
game-camera arrival frames were pixel-identical. The decorative bands
can differ in turned or fly views when the atlas is absent; the base
roof tiles remain. The intentionally partial cache with just the floor
files reaches the existing ground-only 3D recovery mode. It is not full
playable scenery and was not accepted as a full 3D transition by the
strict native route monitor. Completely missing floor.bin still falls
back to 2D; this change neither synthesizes nor rebuilds the whole cache.

The intact-cache standard fly tour compared exact baseline and candidate
on ARTEMPLE and ARVILLAG e0, 5 stops by 3 turns on each side. All 5 stops
matched ([summary](intact-fly-tour.jpg) from the existing tour runner).
Focused CTest: 4/4. No pixel reference or exporter format changed.

Baseline EXE SHA-256
`12e279f84b8c439e7a82aa0fd0b37dea10f97aa3221fecce970fb7cd4b352a67`.
Candidate EXE SHA-256
`715cb232a3144d37a1571287522397b523f2527b13ef39fe9e337cf79bcbad7f`.
First-load control: a +30-degree ARVILLAG view with the broken fringe
atlas still presented full 3D with 160 shadow-casting base roof squares.
