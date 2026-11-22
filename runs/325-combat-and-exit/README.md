# 325 follow-up: combat capture and exit probes

The developer Ctrl+F12 flight binding previously took the key before the combat region capture could see it. In combat it now falls through to the normal owner shot handler. Outside combat the developer shortcut and the normal plain F12 path are unchanged.

The test injected the already decoded Ctrl+F12 twice into the Windows engine input queue, in actual combat with the developer tools enabled. [The marked screenshot](combat-mark.jpg), clean PNG and JSON were produced. The first and second shortcuts and the frame after save all reported combat still active. The fixture then attacked, saw the enemy turn and ended the battle through its normal route. The decisive QA injection code is not part of the patch and does not certify a physical keyboard.

The normal-click exit probe no longer starts on a neighboring exit tile or takes over an unsettled entry animation. The walking handler uses the current mouse position, so the probe sets the chosen coordinate immediately before dispatch, not just on the previous frame. Product walk and clip behavior are unchanged.

- ARTEMPLE e0 exit tile 15887 to ARCAVES e0 tile 28707: 42 frames after the normal click; actual first local frame stayed in scene-gl.
- ARCAVES e0 exit tile 31307 to ARTEMPLE e0 tile 16486: 33 frames after the normal click; actual first local frame stayed in scene-gl.
- These are disposable native scene probes, not a reproduction of the owner's save, UI cursor focus, or other random encounter routes.
