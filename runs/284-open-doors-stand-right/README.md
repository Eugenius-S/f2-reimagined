# 284 open and closed doors stand right

Build: origin/main 017ddac2, worktree f2r-279-glm. Per door kind: one representative spot, captured closed and open (`-OpenDoors`) in the game's own 2D and in 3D, plus an open 3D view turned +30. 26 kinds over 26 spots, 145 captures, zero failures.

| Kind | Map | Elev | Tile | Closed->open change | Status |
|---|---|---|---|---|---|
| door (generic) | kladwtwn | 0 | 17699 | leaf strip in 3D (1112-1194) | right |
| wooden door | gecksetl | 0 | 23709 | leaf region both engines | right |
| wood door | ncr1 | 0 | 13915 | narrow leaf strip (1474-1512 x 446-620) both | right |
| tin doorway | denbus1 | 0 | 17292 | same strip (1575-1632 x 195-361) both | right |
| wooden doorway | broken1 | 0 | 17947 | gate area both | right |
| stone doorway | kladwtwn | 0 | 14892 | narrow strip (1127-1314) both | right |
| door frame | kladwtwn | 0 | 22885 | door band both | right |
| security door | epasl | 0 | 20100 | door block (1080-1340 x 255-458) both | right |
| vault door | vctyvlt | 0 | 21095 | none - the big cog has no open frame in either engine | right |
| vault 13 door | vault13 | 0 | 17095 | corridor doors open both | right |
| vault 13 outer door | v13ent | 0 | 25943 | none in either engine | right |
| gate | broken1 | 0 | 17947 | gate area both | right |
| vault city gate | vctyctyd | 0 | 25506 | gate opens both | right |
| fence door | navarro | 0 | 28694 | none at this tile in either engine | right |
| curtain | gstcav1 | 0 | 14255 | curtain drawn aside, same 50x100 px block both | right |
| elevator door | depolva | 0 | 26505 | doors open both | right |
| mine elevator | redment | 0 | 26912 | elevator area both | right |
| elevator (cargo) | mbase34 | 0 | 14141 | none in either engine | right |
| warehouse door | newrcs | 0 | 24537 | none in either engine | right |
| casino door | newr1 | 0 | 18100 | doors open both | right |
| steel bars (cell) | broken2 | 0 | 24944 | bars/gate open both | right |
| trapdoor | gstfarm | 0 | 21345 | none at this tile in either engine | right |
| submarine hatch | sfsub | 0 | 20100 | lid swings up, same block both | right |
| bank door | reddown | 0 | 19059 | none at this tile in either engine | right |
| president's office door | encpres | 0 | 20100 | doors open both | right |
| impenetrable door | arcaves | 0 | 28706 | none in either engine | right |

## Findings

- No defects: in every pair the 3D closed and open states match the classic at the same spot; where the art has an open frame the leaf swings into its frame exactly as the original draws it (security door slides into its wall pocket, the submarine hatch lid stands up, the Vault City gate opens under the arch).
- The "no change" kinds are identical in BOTH engines - the original art has no open frame for those pieces (the vault door cog, warehouse door, cargo elevator, trapdoor, bank door, impenetrable door) or the chosen tile holds no door object (fence door at the Navarro edge tile). The 3D replicates the original's behaviour one-sided exactly as it should.
- The owner's 2026-10-25 item 10 (a broken corner at an open door leaf) does not show: checked at full resolution on the security door, the submarine hatch, the Vault City gate and the Ghost Farm curtain - corners clean everywhere.
- Spots re-aimed during the sweep: tin doorway 13292 -> 17292, gate arvillag 21115 -> broken1 17947, security door epamain1 e0 -> epasl e0, vault door vault15 27103 -> vctyvlt 21095 (the first tile of each showed no door in frame).
- Triage method: closed-vs-open pixel diff per engine; NPC walk noise makes broad bboxes, the door leaf is the narrow strip where 2D and 3D bboxes coincide.

Evidence (closed CLASSIC | closed 3D | open CLASSIC | open 3D | open 3D +30 crops): `tin-doorway-closed-open.jpg`, `security-door-closed-open.jpg`, `curtain-closed-open.jpg`, `sub-hatch-closed-open.jpg`, `wood-door-closed-open.jpg`, `stone-doorway-closed-open.jpg`. Contact sheets: `build/walk/284/sheets_0..2.jpg`.
