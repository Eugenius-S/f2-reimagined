---
id: 233
title: Sharp text no longer flickers, garbles and switches itself off
milestone: M1
status: done
owner: claude
suggested: claude
priority: high
tier: 0
claude-only: no
user-check: yes
depends-on: []
reviewed: yes
---

## Goal

Sharp interface text stays on once it is switched on: no flicker, no garbled
letters, no silent fall back to the pixel font. When it does fail, the failure
is written to the game's logs folder, and the snapshot JSON says whether sharp
text is really drawn.

## Context

The owner, 2026-11-08, build e73dc533 (main), 3440x1440 window, logical
1720x720, scale 2, map kladwtwn, RP 2.4.34: after he switched sharp text on in
the options, the screen flickered, then the text became garbled, then sharp
text switched itself off and the game showed the pixel font. His shots:
`game/shots/dev/2026-11-08/20261108-153737-kladwtwn-e0.{png,json}`,
`20261108-153744-kladwtwn-e0{,-marked}.png/.json` (sharp text still drawn),
`20261108-153908-kladwtwn-e0.{png,json}` (pixel font, first letters of the
message lines clipped, yet the JSON says `"sharp_text":true`).

Cause: the 219 sharp-text cache (PR 336). When the picture under the text is
unchanged but the text records changed (a new line in the message log, here
the "screenshot saved" line the owner's own shot adds), `prepareSharpTextForPresent`
takes the glyph-only rebuild branch. The top of the function had cleared
`output.active`, and that branch never set it again, so
`drawPreparedSharpTextOverlay` found the frame inactive, `drawSharpTextOverlay`
was never called, and `disableSharpText("OpenGL draw failed")` switched the
setting off for the rest of the session. The one line it printed went to
stderr only. The JSON kept saying `true` because it printed the setting, and
the options screen writes the setting back when it closes.

## Acceptance

- The glyph-only rebuild branch marks the frame active; a message appended to
  the log while the picture is unchanged keeps sharp text drawn (checked with a
  3440x1440 scale 2 run, two shots, the second after the screenshot message).
- `disableSharpText` also appends its line to `logs/scene.log`.
- The owner shot JSON has `sharp_text_drawn`, the real drawn state of the last
  presented frame, next to `sharp_text`, the setting.
- Release build and ctest pass.

## Out of scope

The slightly clipped first letter of a message line (the sharp glyph starts one
pixel left of the window's clip box); a separate fix.

## Log

### 2026-11-08 claude
Done: reproduced off screen at 3440x1440 scale 2, Russian, sharp text on, with
`tools/dev-snapshot.ps1 -OwnerShot -OwnerShotDelay 6 -OwnerMark 1 -OwnerMarkDelay 14`:
stdout shows `perf present sharp=1.06` then `sharp=0.00` right after
`owner shot: capture frame`, stderr shows `sharp text: OpenGL draw failed; using
pixel text`, and the second shot has the pixel font (`runs/233-sharp-text-breaks/before.jpg`).
With the fix the same run draws sharp text in both shots, stderr has no sharp
text line, `sharp_text_drawn` is true (`after.jpg`). ctest 20/20.
Next: none.
