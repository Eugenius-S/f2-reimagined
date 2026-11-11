# Task 229 landmark candidates

This is a source-backed candidate inventory from the main game's map records, the active RPU art override, and the in-repository area sweeps. It is not a claim that an unshown side can be recovered from a sprite. A row may name a multi-sprite set where the landmark has no single FRM.

- Arroyo stone head plaza | `arvillag`, tile 23704 | `head1.frm` | Unique named plaza set piece. The sprite supplies the dome and rear rock spur; its face is painted on floor tiles, so this is not a complete isolated object (`tools/scene_export/vendor/fp/sets.py`).
- Temple of Trials | `artemple`, tile 16491 | `temple01.frm` through `temple08.frm` | Unique entrance landmark assembled from the cliff, gate and tower sprites; the ground/steps are map tiles (`tools/scene_export/vendor/fp/sets.py`).
- Vault City gate with towers and wall gate | `vctyctyd` | Map landmark confirmed by task 160's 66-point city sweep; `gate.frm` is present as a single scenery record, but the exact multi-piece tower/arch donor set is not yet separated from surrounding walls and tiles. Do not generate from `gate.frm` alone.
- New Reno truck | `newr1`, tile 11893 | `truck3.frm` | The unique truck called out in task 163. The active RPU archive overrides the master copy with six direction slots, four of which contain art. It supplies several oblique facings, but not an orthographic rear view.
- New Reno neon signs | `newr1` through `newr4` | Sign FRMs are in the game's scenery art list; the exact sign-to-map group for the neon landmark is not separated yet. The city sweep in task 163 identifies the neon corner as a landmark; avoid assigning a single sign FRM without that map-level match.
- San Francisco tanker | `sftanker` (docks in `sfdock`) | `shipsc01.frm` through `shipsc10.frm`, with further tanker pieces `shipsc20.frm` through `shipsc24.frm` | Unique ship landmark spanning multiple scenery sprites and map tiles. The hull is not represented by one complete FRM.
- Vertibird wrecks | `klacanyn`, `navarro`, `newrvb` | Art family includes `vertibrd.frm` and `verti01.frm` through `verti18.frm`; map groups are listed in task 174/192 | Distinctive crashed vehicle landmarks, each assembled from multiple pieces; no single FRM is a whole reference object.

## Generation gate

Two controller-generated front-view attempts for the New Reno truck were rejected: the first enlarged the tank body and invented details; the second added an extra exhaust and changed the body shape. No generated files are accepted or included. The exact source remains in the ignored local scratch area `build/task229/source-candidates/truck3-rpu-six-directions.png`; the bytes came from `game/mods/rpu.dat:art/scenery/truck3.frm` and use `master.dat:color.pal`.

At present there are 7 candidate landmark groups with enough map/art evidence to name, but 0 accepted four-view reference sets. The complex stone head, temple, gate, tanker and vertibird groups lack one complete FRM; New Reno signage lacks an exact map-to-FRM match. Generation should resume only with a source set that can support the requested shape, and hidden sides must stay marked unverified.
