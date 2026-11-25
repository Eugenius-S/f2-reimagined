# 283 roofs open over the hero

Build: origin/main 9cfae5d1, worktree f2r-279-glm. Per building: the hero placed inside the room and on open ground outside, each state captured in the game's own 2D (classic) and in the 3D view, still, shadows/glow off. Views centered on the hero tile.

| Building | Map | Elev | Hero in / out | Kind | Status |
|---|---|---|---|---|---|
| Arroyo hut | arvillag | 0 | 19115 / 21115 | hut | right |
| Temple of Trials hall | artemple | 0 | 17288 / 21288 | temple | right |
| Arroyo caves | arcaves | 1 | 22706 / 28706 (e0) | cave | right |
| Buckner's clinic | kladwtwn | 0 | 22678 / 24678 | shop | right |
| Vic's shack | kladwtwn | 0 | 17467 / 17699 | shack | right |
| Klamath western rooms | kladwtwn | 0 | 22885 / 24678 | house | right |
| Klamath graze dwelling | klagraz | 0 | 24727 / 16333 | house | right |
| Toxic caves | klatoxcv | 0 | 23879 / 15919 | cave | right |
| Rat caves | klaratcv | 1 | 22661 / 14701 | cave | right |
| Trapper's building | klatrap | 0 | 20733 / 13913 | house | right |
| Den top-floor shack | denbus1 | 2 | 23487 / 19487 | shack-multi | right |
| Den upstairs apartment | denbus1 | 1 | 19070 / 15070 | apartment-multi | right |
| Cat's Paw second storey | denbus2 | 1 | 3110 / 1110 | brothel-multi | right (both engines show the same bare floor wedge in void; no roof art to open there) |
| Modoc barn | modmain | 0 | 19333 / 23333 | barn | right |
| Modoc B&B upper floor | modinn | 1 | 25321 / 21321 (e0) | inn-multi | right |
| Modoc B&B ground floor | modinn | 0 | 25321 / 21321 | inn | right |
| Modoc outhouse | modshit | 0 | 20699 / 18699 | shed | right |
| Vault City vault door | vctyvlt | 0 | 21095 / 17095 | vault-door | right |
| Vault City anomaly room | vctyvlt | 2 | 20694 / 16694 | vault-room | right |
| Gecko reactor hall | geckpwpl | 0 | 16533 / 20533 | industrial-hall | right |
| Broken Hills jail | broken1 | 0 | 19947 / 23947 | jail | right |
| Broken Hills east building | broken2 | 1 | 21340 / 18944 (e0) | multi-storey | right |
| New Reno first street upper | newr1 | 1 | 15087 / 24105 (e0) | multi-storey | right |
| New Reno second street top | newr2 | 2 | 18695 / 26105 (e0) | multi-storey | right |
| NCR entrance warehouse | ncrent | 1 | 19733 / 20274 (e0) | warehouse | right |
| NCR council top floor | ncr1 | 1 | 16104 / 13915 (e0) | civic | right |
| Navarro bunker | navarro | 1 | 28694 / 28694 (e0) | bunker | right |
| Redding mine tunnels | redmtun | 0 | 20100 | mine | right |
| Wanamingo mine level | redwan1 | 0 | 26893 | mine | right |
| New Reno VIP basement | newrvb | 0 | 21732 | basement | right |
| New Reno chop shop | newrcs | 0 | 24537 | garage | right |
| Shuttle interior | sfshutl2 | 0 | 20695 | vehicle-interior | right (the floor's UNUSED ART placeholder tiles draw in the classic and in 3D alike) |
| Vault 13 entrance cave | v13ent | 0 | 25943 | cave-mouth | right |
| Vault 13 dining hall | vault13 | 2 | 14694 | vault-room | right |
| Vault 15 door cave | vault15 | 0 | 27103 | vault-door-cave | right |
| Mariposa vat room | mbase34 | 1 | 15131 | military | right |
| Mariposa corridor fork | mbase34 | 0 | 14141 | military | right |
| Enclave presidential hall | encpres | 0 | 20100 | hall | right |
| Ghost Farm cavern | gstcav1 | 0 | 14255 | cave | right |

38 buildings, 128 captures (26 buildings x in/out x classic/3D + 12 whole-interior maps x in classic/3D), zero failures.

## Findings

- No roof-rule defects: in every pair the 3D roof state matches the classic 2D at the same hero tile - open over the hero inside, closed when the hero is away.
- The classic hides a roof when the hero's screen point falls on it, so a hero standing right against a wall keeps that roof open in BOTH engines (seen on the clinic first shot at tile 23481, re-shot at 24678 for a clean closed pair). The 3D replicates the original's radius faithfully.
- Placeholder split confirmed on SFSHUTL2: floor tiles whose art is the red UNUSED ART placeholder draw in the classic itself, and the 3D draws them too (match, see `sfshutl2-unused-art-both-engines.jpg`). This is distinct from the 281 find at tile 20985 (CLOSED ART placeholder OBJECTS where the classic draws nothing) - task 329 keeps that rule one-sided.
- NAVARRO e0 at tile 28694 is the gas station at the map's south edge (the classic draws it, the 3D matches it, see `navarro-edge-gas-station.jpg`).
- 3D stills render dark interiors slightly brighter and colder than the classic at the same spot - global tint of still captures, present in every prior walk, not a roof-rule issue.

Evidence pairs (classic left | 3D right): `kladwtwn-clinic-open.jpg`, `kladwtwn-clinic-closed.jpg`, `vctyvlt-vault-door.jpg`, `sfshutl2-unused-art-both-engines.jpg`, `navarro-edge-gas-station.jpg`. Raw captures: `build/walk/283/` (contact sheets `sheets_0..3.png`).
