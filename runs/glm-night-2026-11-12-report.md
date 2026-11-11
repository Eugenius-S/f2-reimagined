# glm's night, 2026-11-12: the queue report

Per the queue in `runs/glm-night-2026-11-12.md`, top to bottom.

- **146 (the combat fixture): PR #408, merged.** The aim pick now fights on
  the map's drawn ground: the fixture moves the hero out of the
  clean-edges void (klatoxcv e2's landing), aims with the engine's own
  sprite-rect arithmetic plus a small scan, spawns the target one hex away
  (bare hands reach one hex) and lights it (dark maps read the light at
  the defender). The fixture also leaves cleanly after PASS and FAIL, and
  the runner counts CAPTURE lines as watchdog progress and matches the
  game's `.MAP` suffix in its PASS check. Verified: all four acceptance
  combos (klatoxcv e2 3D, kladwtwn 3D, kladwtwn classic, klatoxcv e2
  classic) each give START, four CAPTUREs, PASS; strips in
  `runs/146-combat-fixture/`; ctest 27/27; the tools' unit tests;
  check_public_content and check_runs_size clean; the pixel regression 150
  points 0 changed. Not verified: a map-hostile target far from the hero
  (both acceptance maps use test critters); the captured attacks missed
  (the swing shows, no damage text).

- **193 part A (the message window's clipped text): PR #410, merged.** The
  log's sharp rows share one straight left edge: the monitor's per-row
  one-pixel stair now applies only with the sharp pass off (the pixel view
  keeps it), the sharp pass drops the per-row counter-correction, and a
  wrapped continuation starts at the block's text column. Verified: the
  log's leftmost ink at 206/208/210/219 px before vs 206/206/206/206 after,
  in English and in Russian; the sheets in
  `runs/193-text-indent-and-line-height/`; ctest 27/27 with the updated
  embedded_fonts case; the pixel regression 150 points 0 changed
  (the regression runs sharp-off); perf-check red as on main, within the
  reference band. Not verified: the dialogue answers' hanging indent
  (part A continues) and the line-pitch check against the dialogue.

- **135 (3D not disabled in combat): PR #442, merged.** Evidence only: the
  combat acceptance closes on 146's fixture (the four phases on both maps,
  3D and classic; the aim gate proves the pick names the target with a
  to-hit over zero on all four view/map combinations). The strips copied
  into `runs/135-3d-always-on/`. Not verified: a human playing the fight
  (the owner's fly-through); the unscaled outlines stay task 149.

- **140 (dialogue letters cut): skipped** - gpt's PR #424 is open.

- **149 (combat outlines follow turn and zoom): PR #443, draft, blocked on
  the turn half.** The zoom half is proven: the outline hugs the shown
  sprite at zoom 0.5/1/2 (the fixture's targeting frames in
  `runs/149-combat-overlays-follow-view/`). The turn half: two attempts -
  the projected path already engages at a turn (the renderer's gFlying is
  true once the turn's pose lands), and the outlines still miss: the
  placed ink sits about 120 logical px off the drawn critters, and the
  target's own outline draws in the cursor pass, which does not follow the
  turn at all. The diagnosis and the suspect pair (the overlay's
  staticScreenFromWorld fly path versus the mvp the cards draw with) are
  in the task Log. Not verified: turn +/-30 outline on the sprite.

- **147 (map exits and floors): PR #444, draft, the matrix walk started.**
  The KLAGRAZ blocker (2026-11-04) is dead: the probe now clicks the hex
  the ground pick names (the bare anchor round-tripped to a neighbour) and
  the ground pick accepts any hex the map draws ground on (the exits live
  in the clipped edge zones). Proven: KLAGRAZ->KLADWTWN passed (30
  frames), KLADWTWN->KLAMALL passed (29 frames), the -Tour
  kladwtwn->klamall->klagraz->kladwtwn (the last stop through a save)
  shows every arrival in 3D with the camera on the hero. Recorded for the
  next steps: the mall's script removes the kladwtwn-facing exit grids
  after the probe's teleport (the probe must walk onto the tile), the
  KLARATCV stairs have no exit grids (an elevation driver is needed), the
  klacanyn walk stage timed out. ctest 27/27; perf-check within the
  reference band. Not verified: the full matrix (the mall walk-based
  probe, the stairs driver, the world map out/in, save on another map).

- **173 (3D under windows): PR #445, draft, evidence only.** The owner's
  2026-11-10 rule holds on main: the inventory and the Pip-Boy draw over
  the live 3D (frames in `runs/173-3d-never-off/`), the full-screen
  options menu sits on the black backdrop per 193 part B's spec, the
  dialogue fixture keeps the 3D view on, no run's log has a fallback
  line. No code changed; the perf reference stands (f2r-verified
  8c310a9be). Not verified: the rest of the acceptance (the area walks,
  the billboard list).

- **156 (resolution, window mode, fullscreen): skipped** - gpt's PR #425
  is open.

- **189 (walls on all maps): not taken.** The painted-post rule's
  diagnose (face selection across maps, the 2026-11-12 audit) is a deep
  renderer research task; the night went to the higher-tier items first
  and then to the second pass (148 done, 143 attempted).

- **224 (geometry second pass): PR #446, draft, the finds worked off.**
  F1 (kladwtwn's black edge wedge) verified FIXED on main by 202's
  quadrant fill (the fresh frame at the same view tile). F2 (denbus2 e1)
  re-diagnosed: not a 3D defect - the map's own e1 data is 9917/10000
  void floor squares, 0 objects, and the fresh classic 2D frame renders
  the same content the 3D does; the real residual (the vanilla 2D draws
  the lower elevation's tall walls into the upper view, the 3D upper
  scene does not) is recorded as the next exporter rule. Evidence pairs
  in `runs/224-geometry-second-pass/`. F3 (the Shark Club marquee) stays
  open. Not verified: F3's diagnosis, the full 154-spot re-walk.

- **143 (cloud shadow edges): attempted, still open.** Three hour frames
  at the owner's spot did not reproduce the hard curved edge (the clouds
  drift), and a sharp-step measurement is dominated by the road's own art
  edges. The attempt is recorded in the task Log (PR #450); the 5-bit
  quantization suspicion stays the first suspect once a controlled
  clouds-off/on pair exists.

- **148 (README turn and combat): not taken** (the budget went to the
  higher-tier items above).

Rule 1 after the last code merge: performed in full, twice. After 193A
(main 8c310a9be) and again after the night's PR wave merged (443, 444,
445, 446, 447; main b3820c487): Release rebuilt from fresh origin/main,
ctest 27/27, the off-screen dev-snapshot launch check OK, perf-check red
as known on main (the final pass: avg 59.2/54.8/60.4/59.1 fps, load
1787/2303/1597/833 ms for kladwtwn, newr1, vctyctyd, klatoxcv e2; the 1%
lows under 60 in background runs as always),
`f2r-verified/fallout2-ce.exe` carries b3820c487 with its line in
`version.txt`, and `gh workflow run verify.yml -f commit=b3820c487` was
dispatched (run 43103224810).
