# 303: fly tour coverage of the re-walk selections

`tools/tour/tour.py --coverage` shoots nothing. It prints the spots that `tools/tour/spots.json` (or `rp_spots.json`) has for the requested maps, then every requested map with no spot. The same report now also prints before every tour.

Below are the reports for the four geometry re-walk selections, with the maps taken from the exporter's region groups (`MAP_GROUPS`, `RP_MAPS` in `tools/scene_export/export.py`). The re-walk tasks belong to their own owners; these files only say what the tour can show them.

| Re-walk | Groups | Report | Requested maps with no spot |
|---|---|---|---|
| [279](../../tasks/279-rewalk-arroyo-klamath-den-modoc.md) | arroyo, klamath, den, modoc | [rewalk-279.txt](rewalk-279.txt) | arvill2 |
| [280](../../tasks/280-rewalk-vault-city-gecko-redding-reno.md) | vaultcity, gecko, redding, brokenhills, newreno | [rewalk-280.txt](rewalk-280.txt) | bhrndmtn, newr1a, newr2a |
| [281](../../tasks/281-rewalk-ncr-vaults-sf-enclave.md) | ncr, vault13depot, vault15, militarybase, sanfran | [rewalk-281.txt](rewalk-281.txt) | none at map level |
| [282](../../tasks/282-rewalk-encounters-and-rp-maps.md) | encdesertcoast, enccitycave, encspecial, plus the RP maps | [rewalk-282.txt](rewalk-282.txt) | 8 coast (coast3, 5-9, 11, 12), 9 desert (desert1-4, 6, 8, 9, desrt11, desrt12), cowbomb; RP: easter, gammovie, gcenclen, merchant, v13_orig, v15_orig, vill |

The maps in the last column still need configured spots or owner captures before a tour can count as evidence for them.

Limits:

- Elevations are not checked here. The expected elevations come from each map's header in the game data, which these reports did not read. Pass them with `--expect map:elevation,...` to list every elevation without a spot.
- A named spot proves only its own view at its turns (-45, 0, +45), not every object of its class or the rest of the map.
- Map names count as requested even if the game has no such map. An unknown name shows up as a map with no spot.
