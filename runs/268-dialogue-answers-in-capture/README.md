# Dialogue fixture evidence

Windows Release `0f1570f37c1db2ad0922c33c7bc35fb721b4b619` (`32660ab18aee1e0c7b3a6185d55da11b44216b90516eac9694d56449501caec6` executable SHA-256). Captured on an isolated Restoration Project copy at Sulik's first dialogue page in `KLADWTWN.MAP` elevation 0 (hero 25075, speaker 23478). The fixture lengthens answer 797 in its private run data. Both languages draw the full NPC reply and all seven options in the Windows framebuffer.

| Language and scale | Full frame | Sidecar | Wrapped options (1-based) |
| --- | --- | --- | --- |
| English ×1 | [960×540](english-x1.webp) | [JSON](english-x1.json) | 1 |
| English ×2 | [1920×1080](english-x2.webp) | [JSON](english-x2.json) | 1 |
| Russian ×1 | [960×540](russian-x1.webp) | [JSON](russian-x1.json) | 1, 4 |
| Russian ×2 | [1920×1080](russian-x2.webp) | [JSON](russian-x2.json) | 1, 4 |

Each sidecar records seven actual button rectangles in logical 960×540 coordinates via `gameDialogGetOptionLayout`. For the physical coordinates, multiply by `scale_factor`. `text_top` and `text_bottom` are positions within the options window. The full-frame evidence is encoded as lossless WebP and decodes pixel for pixel against the original PNG. The sidecars record the original PNG filename and SHA-256.

To repeat, pass the isolated game copy, its matching Klamath cache, and the Release executable to the fixture:

```powershell
foreach ($language in 'english','russian') {
    foreach ($scale in 1,2) {
        pwsh -NoProfile -File tools/dev-snapshot.ps1 -DialogFixture `
            -GameRoot $qa -SceneCache $cache -BuildExecutable $exe `
            -Map kladwtwn -Elevation 0 -HeroTile 25075 -Language $language `
            -Scale $scale -ScreenWidth (960*$scale) -ScreenHeight (540*$scale) `
            -CaptureConsole -OutputDirectory "build/268-$language-x$scale"
    }
}
```

CTest passed 35/35. Controls used the Windows artifact from main `f0474d2aefb5f1df931d0a463b888539d2faec97` (executable SHA-256 `487fddc43cba53da4c27acf0ff517126ea19ff57cf2495289d2d375aae017797`).

| Control | Result on main and candidate |
| --- | --- |
| Ordinary live Klamath view (`-LiveFrame -NoPreview`, hero and camera 25075, noon, 1920×1080 at ×2) | 0 differing RGB pixels |
| Ordinary dialogue cursor probe (`-DialogCursorCheck -SharpText`, English ×2) | Both pass with matching cursor proofs and 0 differing RGB pixels in the named hover capture |
| Stock `entrance` regression spot (Klamath e0, camera 25075, 1920×1080 at ×2) | 0 differing RGB pixels between the two captures. The tracked reference differs by 517,359 pixels on both builds |
| Old page inspector on the isolated RP copy (English ×2) | Both time out waiting for its specific page-inspection PASS marker |
| Old layout diagnostic (`-DialogLayoutCheck -SharpText`, English ×2) | Both report 7 buttons and 14 text rows against the expected 8 |

The stored regression reference and the two legacy diagnostic expectations were kept as-is. The fixture matrix above runs separately and passes on the candidate.
