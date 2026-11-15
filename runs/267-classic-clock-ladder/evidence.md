# Task 267 clock ladder evidence

The harness verified all four sidecars in each run against the requested map and elevation. The 22:00 and 09:00 images differ by the listed number of pixels with channel difference above 8.

| Map | Elevation | Hour | Committed frame | Local sidecar | 22:00 vs 09:00 changed pixels |
| --- | ---: | ---: | --- | --- | ---: |
| CITY1.MAP | 0 | 22:00 | `city1-2200.png` | `build/classic-clock-267-city1-final/devshot-20261113-180429.json` | 1,627,640 |
| CITY1.MAP | 0 | 02:00 | `city1-0200.png` | `build/classic-clock-267-city1-final/devshot-20261113-180429-001.json` | |
| CITY1.MAP | 0 | 06:00 | `city1-0600.png` | `build/classic-clock-267-city1-final/devshot-20261113-180429-002.json` | |
| CITY1.MAP | 0 | 09:00 | `city1-0900.png` | `build/classic-clock-267-city1-final/devshot-20261113-180429-003.json` | |
| RNDWHALE.MAP | 0 | 22:00 | `rndwhale-2200.png` | `build/classic-clock-267-encspecial/devshot-20261113-175952.json` | 1,664,908 |
| RNDWHALE.MAP | 0 | 02:00 | `rndwhale-0200.png` | `build/classic-clock-267-encspecial/devshot-20261113-175952-001.json` | |
| RNDWHALE.MAP | 0 | 06:00 | `rndwhale-0600.png` | `build/classic-clock-267-encspecial/devshot-20261113-175952-002.json` | |
| RNDWHALE.MAP | 0 | 09:00 | `rndwhale-0900.png` | `build/classic-clock-267-encspecial/devshot-20261113-175952-003.json` | |
