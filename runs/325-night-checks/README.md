# 325: Remaining acceptance checks, Windows

The integration PR #562 was verified and published as 8d9e4de. Releasing the integration does not close all the follow-up acceptance below. Pot-rim follow-up #563 was published as fd16a60. Outline follow-up #564 merged as b6bf9e2 and passed native CTest 34/34, four combat phases and the four-map performance gate.

| Check | Result and limit |
| --- | --- |
| Random encounter and Arroyo exit markers | [F12 frames](exit-markers-f12.jpg) show the orange random bridge and green Arroyo bridge markers after one real key press each. Both before and after metadata remained scene-gl on the same tiles. This is marker and screenshot proof, not walking proof. |
| Original randomized map edge | [Same camera, before and after](random-exit-and-edge.jpg) on the first retained release and fd16a60. The first nonblack pixel on the left matches on all 860 sampled terrain rows. The jagged left edge predates the exit marker fix. |
| Temple edge movement | At 3440x1440 and 2x, camera tile 22307 moved to 22108 going left, to 21708 diagonally along the boundary, and to 21907 on reversing. The authored clip planes stayed identical and all four views remained scene-gl. This does not cover all maps or the owner save's specific door click. |
| Temple navigation and combat | ARCAVES e0 to ARTEMPLE walked in a normal engine fixture. Actual targeting, player attack, enemy turn and combat end on ARCAVES kept local 3D. The reverse probe's automatic entry proof rendered in 3D, but a separate normal-click fixture was inconclusive. |
| Screenshot actions | Plain F12 in Options and on two bridge maps was verified. Region PNGs and JSON were verified outside combat. The real Ctrl+F12 confirmation during combat was not certified by an offscreen fixture. |
| Night floor and fixed sources | Same native camera in Arroyo, sun shadows active: floor crop averages 24.9 at 22:00 and 02:00, 87.6 at 06:00 and 60.8 at 09:00. The no-bake control measured 26.2, 87.6, 60.8. Klamath direct lighting also darkened overnight. The torch and interface remained visible. |
| Temple ensemble | [Matched native camera](temple-sun-hours.jpg) at 09:00, 12:00 and 16:00. The marked sections of the ensemble were checked visually, not given an additional geometry change or a per-pixel grouping claim. |
| Character creation | The English fixture checked vertical centering and fitting for all trait labels, then pressed and released one trait button with SharpText on and off. When on, all 96 captured text runs stayed in the SharpText overlay after selection. No claim for the unavailable external Russian text and fonts. |
| Display | Button selection persisted 1600x1200 and 2x in the isolated configuration fixture. Physical resizing, synchronization, and all fullscreen modes remain unverified. |
