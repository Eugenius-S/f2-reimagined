# Original and CE HUD on Windows

**Preferences → General → HUD Style** controls the HUD side panels. Original uses the metallic winged emblems from the installed high-resolution art. CE is the default and uses the profile's existing HUD layout and decor. With either of the two original frames missing, the menu reports that Original is unavailable and the game displays CE.

## Paired game frames

| Game view | 1920×1080 at ×2 | 3440×1440 at ×2 |
| --- | --- | --- |
| Classic | [Original and CE](classic-ordinary.webp) | [Original and CE](classic-wide.webp) |
| 3D | [Original and CE](3d-ordinary.webp) | [Original and CE](3d-wide.webp) |

[HUD close-up](hud-detail.webp) · [English and Russian menu](menu-choices.png) · [Raw capture metadata and SHA-256](captures.json)

All captures use the same Klamath entrance (`KLADWTWN.MAP` e0, hero and camera at tile 25075, noon) and the 800-pixel center panel. The 3D ultrawide sheet is reduced for the repository. The RGB comparisons use the raw framebuffer captures. In each of the four cases, the CE HUD matches main pixel for pixel, and Original matches every pixel of the CE center strip. Both Original emblems and both seams are visible. At the 640-pixel logical minimum, the 640-pixel center occupies the full width.

## Interaction on the isolated Windows copy

- In Russian 3D and English Classic, opened the dropdown with scripted mouse input in the actual Preferences window, selected Original, saved, then quit and relaunched with the same profile. Repeated for CE. The menu save set `ui.hud_style` to `1` and `0` respectively, and the next process showed the saved HUD.
- Staged CE from Original and pressed Escape. The original art and saved value persisted in both views. Clicking Defaults and saving restored CE in both views.
- Clicked a decorative side panel and compared the center before and during the press. All center pixels were unchanged. Pressing the inventory button at the same layout coordinate drew its pressed state in Classic and 3D.
- The isolated stock game copy without `f2_res.dat` displayed the CE HUD and [the unavailable message](original-unavailable.png) with Original saved. No game artwork is copied or bundled by this change.

Source candidate `962f829ef3877942c5629281320546ff334fb0e9` (Windows executable SHA-256 `d682e8b4a469e826becff3b648d52c8bfc871d85310399a769dd629614bc218f`). The control uses the CI Windows artifact from main `611c00c39385afdc1dc3b117d581015f18711c71`. Release build and CTest passed, 35/35. Independent review checked the settings and art selection and rechecked the narrow and wide drawing after the fix.
