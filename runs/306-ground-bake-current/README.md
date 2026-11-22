# 306: ground bake preserves the current draw state

The prior [draft #517](https://github.com/Eugenius-S/f2-reimagined/pull/517) isolated a lost floor texture and glow framebuffer during ground-bake allocation. The current renderer already has the corrected floor light mode and its later receiver/unit-7 code. This patch ports only the missing binding restoration. The bake creates and binds only the draw framebuffer, and then restores the caller's draw framebuffer and current 2D texture. The read framebuffer does not change.

[Matched native Windows images](before-after.jpg), exact main 45de6c363 versus candidate. No source assets or cache data were changed.

| Setup | Prior | Candidate | Candidate bake vs direct |
| --- | --- | --- | --- |
| KLAMALL e0 tile 28096, 1920x1080, 2x, same still camera and shadows | 59.980% near-white sample points | 0% near-white | mean absolute RGB-channel delta 1.283 |
| ARVILLAG e0 tile 25115, 1366x768, 1x, same still camera, shadows and glow | 100% black sample points | 2.003% black, similar to the floor's normal direct frame | mean absolute RGB-channel delta 1.362 |

Both sides kept the same clock request and scene-gl. The KLAMALL no-bake control remained unchanged across the sampled pixels (delta 0). Direct lighting was a control, not the fix. The samples are from the interior at a stride of four physical pixels. Four focused native tests (ground pick, game view, map clip, sun) passed. The alternate viewport is a new isolated process; the physical display was not changed.
