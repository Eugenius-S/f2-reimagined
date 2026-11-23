# 325 follow-up: apply display-only preferences live

The modern options model writes the selected resolution when Done is pressed. The physical window update was tied to the legacy preference dirty flag, so a display-only edit saved to the configuration but did not resize the window. Apply the desired mode after successful dialog teardown and do nothing if the live mode and dimensions already match. Cancel and VSync's restart policy are unchanged.

The mouse had previously stretched input over the whole window even when the active image was letterboxed. Absolute input and cursor warps now share the renderer's content rectangle. Normal and HiDPI points map consistently, and clicks in the bars are ignored. The relative input path is unchanged. The scene pick adapter uses the already mapped logical point rather than compensating twice.

Windows QA red: choosing 1600x1200 from 1920x1080 saved the value, but the old application hook did not run. Green, using the normal Preferences commit path with a bounded hidden-window fixture:

    requested=1600x1200 window=1600x1200 drawable=1600x1200 logical=960x540 scene_gl=1 hidden=1
    mouse logical=480/216 inside=1 tile=28906 oldTile=28906 barInside=0

The fixture kept its window off screen and hidden during the resize, never changed the monitor's display mode, and is not part of the shipped change. The game-view, ground-pick and options model suites passed. The test does not certify focus, fullscreen, physical display synchronization or every real mouse device.

[Display-only Cancel evidence](display-cancel-stage.jpg): exact Windows build 94f5b988d stages 1600x1200, then Escape leaves the original 1920x1080 at 2x and the whole isolated configuration unchanged.
