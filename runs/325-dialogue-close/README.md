# 325 follow-up: keep 3D during dialogue close

Sulik's actual Goodbye choice in the Windows Klamath fixture reproduced a local 2D fallback on all 19 rolling-close frames. The engine retained a modal window but had already left the dialogue mode, marked the teardown as special, and left isometric updates paused.

The fix labels just that teardown as a local visible modal for the scene policy. It does not change the game's mode flags, panel animation, input, world map, movies, or save/load guards.

| Same fixture, first frame back on the local map | Previous | Candidate |
| --- | ---: | ---: |
| Native close animation frames with the modal visible | 19 | 19 |
| Those frames with the local scene off | 19 | 0 |
| All seven sampled points black | 0 | 0 |
| Four exterior pixel RGB sums, first close frame | 208, 0, 84, 0 | 120, 64, 120, 208 |
| Four exterior pixel RGB sums, first local frame after close | 120, 64, 120, 208 | 120, 64, 120, 208 |

Synthetic logical key 7 used Sulik's real Goodbye response and its ordinary script teardown. A temporary test-only sampler read seven representative points of each presented OpenGL frame; no claim for unsampled pixels or physical input. It did not change production transitions. The candidate focused native tests passed 4/4: preview policy, game view, text layer and dialogue layout. Pip-Boy close was separately exercised at baseline and did not drop local 3D.
