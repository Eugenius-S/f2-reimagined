# 325 follow-up: continuous outlines at zoom

The previous transformed 3D outline projected the center of each painted donor pixel. At high zoom this left holes between the dots. In a reproduction using the old loop, a three-pixel stroke at 3x painted three isolated pixels instead of a 9 by 3 strip.

The new path inverse-samples the existing indexed mask inside its projected quad. The palette, unobscured interiors and native untransformed path stay the same. The projection respects the destination clip and discards degenerate faces. The existing sprite-card suite now includes magnified, perspective, transparent, clipped and edge-on masks.

[Windows presented frames](outline-zoom.jpg) show the same third-person view in ARCAVES at tile 19864 (actor 20467, zoom 1.8). A QA-only hook forced a known red outline on the visible actor; it is not part of the patch. The presented pixel changes fit in a 114 by 238 rectangle around the actor. The separate static world captures are pixel-identical.

This is an isolated reproduction of the broken outline at scale, not an attribution of every outline in the owner's recording.
