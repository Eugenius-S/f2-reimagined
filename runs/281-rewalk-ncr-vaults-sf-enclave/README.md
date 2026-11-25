# 281 re-walk

Build: origin/main 12992e533, worktree f2r-279-glm. Views: game camera and +45 fly turn per spot, still, shadows/glow off.

| Map | Elev | Tile | Spot | Status |
|---|---|---|---|---|
| mbase12 | 0 | 16089 | the military base corridors / mbase-corridors | clean |
| mbase12 | 1 | 16089 | the military base lower level | clean |
| mbase34 | 0 | 14141 | the base corridor fork | clean |
| mbase34 | 1 | 15131 | the vat room / mbase-vats | clean |
| mbclose | 0 | 23939 | the military base surface / mbase-surface | clean |
| navarro | 0 | 28694 | Navarro, the base floor / navarro-base | clean |
| navarro | 1 | 28694 | Navarro, the lower floor | clean |
| navarro | 2 | 28694 | Navarro, the hangar level | clean |
| ncr1 | 0 | 13915 | NCR city, elevation 0, entry / the NCR council square / ncr-council-square | clean |
| ncr1 | 0 | 20898 | NCR city, elevation 0, overview / ncr-ncr1-e0 | clean |
| ncr1 | 1 | 13915 | NCR city, elevation 1, entry | clean |
| ncr1 | 1 | 16104 | the council building top floor | clean |
| ncr1 | 1 | 20106 | NCR city, elevation 1, overview / ncr-ncr1-e1 | clean |
| ncr2 | 0 | 13895 | the statue plaza | clean |
| ncr2 | 0 | 20100 | NCR bazaar, elevation 0, entry | clean |
| ncr2 | 0 | 20902 | NCR bazaar, elevation 0, overview / ncr-ncr2-e0 | clean |
| ncr3 | 0 | 21499 | squatters camp, elevation 0, overview / ncr-ncr3-e0 | clean |
| ncr3 | 0 | 23669 | the west gate street | clean |
| ncr3 | 0 | 23869 | squatters camp, elevation 0, entry | clean |
| ncr4 | 0 | 17699 | north NCR, elevation 0, entry | clean |
| ncr4 | 0 | 18279 | the brahmin pens | clean |
| ncr4 | 0 | 20481 | north NCR, elevation 0, overview / ncr-ncr4-e0 | clean |
| ncrent | 0 | 19112 | NCR entrance, elevation 0, overview / ncr-ncrent-e0 | clean |
| ncrent | 0 | 20274 | the NCR entrance | clean |
| ncrent | 0 | 20877 | NCR entrance, elevation 0, entry | clean |
| ncrent | 1 | 19733 | the entrance warehouse | clean |
| ncrent | 1 | 22148 | NCR entrance, elevation 1, overview / ncr-ncrent-e1 | clean |
| sfchina | 0 | 26504 | Chinatown street | clean |
| sfchina2 | 0 | 26504 | Chinatown, the shaman's floor | clean |
| sfchina2 | 1 | 20133 | Chinatown, the upper walk | clean |
| sfchina2 | 2 | 24722 | Chinatown, the top floor | clean |
| sfdock | 0 | 20100 | the docks pier / sanfran-docks | clean |
| sfelronb | 0 | 23155 | the Elronologists' base | clean |
| sfshutl1 | 0 | 26311 | the shuttle exterior | clean |
| sfshutl2 | 0 | 20695 | the shuttle interior | clean |
| sftanker | 0 | 27701 | the tanker deck, bow / sanfran-tanker-deck | clean |
| sftanker | 1 | 27701 | the tanker deck, midship | clean |
| sftanker | 2 | 23490 | the tanker, the top level | clean |
| v13ent | 0 | 25943 | the Vault 13 entrance cave / v13ent-cave | clean |
| v15ent | 0 | 23907 | Vault 15 entrance, elevation 0, overview / ncr-v15ent-e0 | clean |
| v15ent | 0 | 29325 | the Vault 15 entrance desert / v15-entrance | clean |
| v15ent | 0 | 29523 | Vault 15 entrance, elevation 0, entry | clean |
| v15ent | 1 | 23907 | Vault 15 entrance, elevation 1, overview / ncr-v15ent-e1 | clean |
| v15ent | 1 | 29325 | the vault plaza | clean |
| v15ent | 1 | 29523 | Vault 15 entrance, elevation 1, entry | clean |
| v15sent | 0 | 17700 | Vault 15 south entrance, elevation 0, entry | clean |
| v15sent | 0 | 18097 | the squatters' camp | clean |
| v15sent | 0 | 19088 | Vault 15 south entrance, elevation 0, overview / ncr-v15sent-e0 | clean |
| vault13 | 0 | 17095 | the Vault 13 command centre / vault13-corridors | clean |
| vault13 | 1 | 17095 | the Vault 13 living quarters | clean |
| vault13 | 2 | 14694 | the Vault 13 dining hall | clean |
| vault13 | 2 | 16694 | the deathclaw floor / vault13-deathclaw-floor | clean |
| vault15 | 0 | 19094 | Vault 15 ruins, elevation 0, overview / ncr-vault15-e0 | clean |
| vault15 | 0 | 20100 | Vault 15 ruins, elevation 0, entry | clean |
| vault15 | 0 | 27103 | the vault door cave | clean |
| vault15 | 1 | 20100 | Vault 15 ruins, elevation 1, entry | clean |
| vault15 | 1 | 21907 | Vault 15 ruins, elevation 1, overview / ncr-vault15-e1 | clean |
| vault15 | 1 | 27103 | vault15-level | clean |
| vault15 | 2 | 20100 | Vault 15 ruins, elevation 2, entry | clean |
| vault15 | 2 | 20897 | Vault 15 ruins, elevation 2, overview / ncr-vault15-e2 | clean |

## Defect

- **sfshutl2 e0, around tile 20985** (defect): the 3D game camera draws a field of about a hundred red "CLOSED ART!" hex cards lying flat over the black floor; the classic 2D at the same tile shows an empty black picture (the original does not draw these objects at all). Evidence: `sfshutl2-closed-art.jpg` in this folder. The red diamond with the CLOSED ART text is a placeholder that lives in the art data, so the rule to fix is: where the original 2D draws nothing (missing art), the 3D scene must not draw a visible placeholder either. Cut as task 329 (the placeholder rule with its own fix PR).

## Notes

- No other geometry defects found in this walk (21 maps, 60 spots, game camera and +45 fly turn each).
- Dark cells (military base corridors, Vault 13 entrance cave, the tanker's lower decks, NCR1 fly views behind buildings) follow the maps' own lighting or the known fly roof-block limit at tour distance 8; the game-camera view of the same spots renders and was used for judging.
- Chain-link fence sections (ncrent gates) keep the known dark-mesh look of 191 find 1 / task 276 (draft).
