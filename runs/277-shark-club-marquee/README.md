# 277: Exterior casino canopies, native Windows comparison

The stock art puts the illuminated upper fascias on live scene cards:
`CASENT06.FRM` (Desperado) and `CASENT26.FRM` (Shark Club). Their
upper texels crossed their own buildings' roof height masks. In the
ordinary 3D view this revealed the floor and disconnected objects behind
the fascias. `CASENT25.FRM` is a neighboring wall, not a second marquee.
Only the two verified canopy art files skip this roof-grid test. The
roof meshes, depth test, shadow, visibility and picking paths remain.

[Native matched views](native-matched-facades.jpg): Classic, same-camera
before/after in local 3D, and final 3D at +30 degrees. All use the stock
assets and native renderer on Windows. The still frame configuration was
1920x1080, 2x, 12:00, zoom 1, with sunlight and glow. Classic frames are
shown as reference, not as 3D regression targets.

| Still view, e0 | Changed RGB pixels vs main | Bounds (right and bottom exclusive) |
| --- | ---: | --- |
| Shark Club, NEWR2 tile 18894 | 65,930 | x 758..1173, y 106..294 |
| Desperado, NEWR1 tile 16500 | 92,790 | x 1275..1810, y 0..200 |
| New Reno arch, NEWR1 tile 11502 | 0 | Unchanged |
| Boxing ring, NEWRBA tile 20100 | 0 | Unchanged |

The standard pixel checker passed both unaffected stills against the
exact main baseline with zero channel and area tolerance. No shared
regression reference was updated. The changed stills were compared
directly against matching controls, then inspected for the location and
shape of the difference.

[Standard fly tour](changed.jpg) against 65d374e8 covered eight stops at
-45, 0 and +45 degrees across NEWR1 and NEWR2, e0 to e2, before and after.
Two stops differ; both see Desperado's restored cover at one or more turns.
[Highlighted differences](fly-delta.jpg) show even the partial distant
cover at tile 24105. No changes on the other six stops. The scripted
NEWR2 tour angles do not reproduce the ordinary camera's Shark Club
roof cut; its matched still in the first image is the regression fixture.

Baseline EXE SHA-256
`63abea5f749458924d6d1e371f5176234115a0c04d9aca82b4431014ff4dd953`.
Final uncommitted local candidate EXE SHA-256
`c69755013ed8ff21f76b635030bc28834330195c474cfffbd31b7dd899b4012a`.
Classic controls were taken with the earlier candidate; this change
does not enter the Classic rendering path. Focused CTest: 4/4.
