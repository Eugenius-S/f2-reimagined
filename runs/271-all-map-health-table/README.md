# All-map health table (271)

One command (tools/map-health.ps1) walked every playable map and elevation.
Build: exe sha256 `de20f410dc47a6a9`, main commit `de6d5267`.

184 of 223 rows are valid results (one warmup and one measured run each); 106 of them breach nothing. 39 rows are not valid results and never count in the numbers above a table row.

Sorted worst first: the weakest 1% low at the top; TIMEOUT, FAILED and NOISY rows first of all, because they say nothing about the game.

| map | elev | avg fps | 1% low | worst ms | load ms | black % | fallback | flags | cpu | status |
|---|---|---|---|---|---|---|---|---|---|---|
| cave0 | 0 | 139.7 | 128.8 | 39 | 319 | 0.0 |  |  | 44 | noisy |
| cave1 | 0 | 139.8 | 125.4 | 39 | 595.8 | 0.0 |  |  | 34 | noisy |
| cave1 | 1 | 139.9 | 124.9 | 39.1 | 546 | 25.9 |  | black 25.9% over 20% | 38 | noisy |
| cave4 | 1 | 140.9 | 116.7 | 16.9 | 678.8 | 25.8 |  | black 25.8% over 20% | 38 | noisy |
| cave5 | 0 | 139.7 | 126.6 | 39.5 | 578.5 | 0.0 |  |  | 32 | noisy |
| cave5 | 1 | 140 | 124.9 | 39.1 | 555 | 0.0 |  |  | 34 | noisy |
| cave6 | 0 | 139.2 | 126.2 | 57.7 | 374.6 | 35.1 |  | black 35.1% over 20% | 33 | noisy |
| city7 | 0 | 139.8 | 128.6 | 39 | 570 | 0.0 |  |  | 32 | noisy |
| city8 | 0 | 139.9 | 127.9 | 39.1 | 603.9 | 0.0 |  |  | 33 | noisy |
| coast1 | 0 | 138.5 | 128.1 | 80.4 | 227.9 | 0.0 |  |  | 34 | noisy |
| coast11 | 0 | None | None | None | None | n/a |  | warmup dev-snapshot exited 1 | ? | failed |
| coast12 | 0 | None | None | None | None | n/a |  | warmup dev-snapshot exited 1 | ? | failed |
| coast2 | 1 | 140.1 | 121.7 | 39 | 254.8 | 99.8 |  | black 99.8% over 20% | 36 | noisy |
| coast3 | 0 | None | None | None | None | n/a |  | warmup dev-snapshot exited 1 | ? | failed |
| coast5 | 0 | None | None | None | None | n/a |  | warmup dev-snapshot exited 1 | ? | failed |
| coast5 | 1 | None | None | None | None | n/a |  | warmup dev-snapshot exited 1 | ? | failed |
| coast6 | 0 | None | None | None | None | n/a |  | warmup dev-snapshot exited 1 | ? | failed |
| coast7 | 0 | None | None | None | None | n/a |  | warmup dev-snapshot exited 1 | ? | failed |
| coast7 | 1 | None | None | None | None | n/a |  | warmup dev-snapshot exited 1 | ? | failed |
| coast8 | 0 | None | None | None | None | n/a |  | warmup dev-snapshot exited 1 | ? | failed |
| coast9 | 0 | None | None | None | None | n/a |  | warmup dev-snapshot exited 1 | ? | failed |
| desert2 | 0 | 140.1 | 128.2 | 39.1 | 192.2 | 0.0 |  |  | 42 | noisy |
| desert3 | 0 | 140 | 128.1 | 65 | 187.1 | 0.0 |  |  | 38 | noisy |
| desert4 | 0 | 140.7 | 127.7 | 38.7 | 446.9 | 0.0 |  |  | 44 | noisy |
| desert6 | 0 | None | None | None | None | n/a |  | warmup dev-snapshot exited 1 | ? | failed |
| desert6 | 1 | None | None | None | None | n/a |  | warmup dev-snapshot exited 1 | ? | failed |
| desert8 | 0 | None | None | None | None | n/a |  | warmup dev-snapshot exited 1 | ? | failed |
| desert9 | 0 | None | None | None | None | n/a |  | warmup dev-snapshot exited 1 | ? | failed |
| desrt11 | 0 | None | None | None | None | n/a |  | warmup dev-snapshot exited 1 | ? | failed |
| desrt12 | 0 | None | None | None | None | n/a |  | warmup dev-snapshot exited 1 | ? | failed |
| desrt13 | 2 | 140.2 | 122.9 | 39.2 | 423.9 | 0.0 |  |  | 32 | noisy |
| mbase12 | 0 | 139.7 | 100 | 39.5 | 697.9 | 20.4 |  | black 20.4% over 20% | 33 | noisy |
| mountn2 | 0 | 139.3 | 130.7 | 76.1 | 333.5 | 0.0 |  |  | 38 | noisy |
| mountn3 | 0 | 140.1 | 128.3 | 39 | 331 | 0.0 |  |  | 35 | noisy |
| newrcs | 0 | 140.7 | 121 | 38 | 857.1 | 0.7 |  |  | 60 | noisy |
| newrgo | 0 | 139.6 | 124.9 | 39.3 | 619.7 | 0.0 |  |  | 44 | noisy |
| newrgo | 1 | 140.3 | 125.2 | 38.9 | 680.4 | 51.4 |  | black 51.4% over 20% | 36 | noisy |
| rndholy1 | 0 | 138.6 | 114.6 | 70.5 | 527.6 | 0.7 |  |  | 32 | noisy |
| rndholy2 | 0 | 141 | 119.4 | 16.2 | 647 | 1.5 |  |  | 36 | noisy |
| dnslvrun | 2 | 53.8 | 12.8 | 110.3 | 985.2 | 0.7 |  | 1% low 12.8 fps under 80 | 8 | ok |
| cave2 | 1 | 130.6 | 20.9 | 74 | 587.9 | 21.3 |  | 1% low 20.9 fps under 80; black 21.3% over 20% | 27 | ok |
| modbrah | 0 | 136.1 | 26.8 | 40.3 | 357.5 | 5.6 |  | 1% low 26.8 fps under 80 | 10 | ok |
| sfchina2 | 0 | 135.6 | 28.6 | 72.4 | 831.4 | 0.0 |  | 1% low 28.6 fps under 80 | 7 | ok |
| redwan1 | 1 | 136.1 | 28.7 | 59.8 | 593.5 | 48.0 |  | 1% low 28.7 fps under 80; black 48.0% over 20% | 11 | ok |
| rndexcow | 0 | 84.1 | 29.2 | 516 | 235.3 | 0.0 |  | 1% low 29.2 fps under 80 | 17 | ok |
| ardead | 0 | 60.7 | 56.6 | 18.3 | 584.4 | 15.5 |  | 1% low 56.6 fps under 80 | 11 | ok |
| ncr4 | 0 | 117.4 | 57 | 52 | 542 | 0.0 |  | 1% low 57 fps under 80 | 18 | ok |
| artemple | 0 | 129.9 | 58.7 | 302.4 | 2190.7 | 0.0 |  | load 2190.7 ms over 2000; 1% low 58.7 fps under 80 | 10 | ok |
| arvill2 | 0 | 132.3 | 60.4 | 297.1 | 2107.4 | 0.0 |  | load 2107.4 ms over 2000; 1% low 60.4 fps under 80 | 8 | ok |
| newr1a | 1 | 132.4 | 60.8 | 300.2 | 2118 | 0.0 |  | load 2118 ms over 2000; 1% low 60.8 fps under 80 | 21 | ok |
| newr2a | 2 | 131.9 | 61.9 | 305.5 | 2128 | 0.0 |  | load 2128 ms over 2000; 1% low 61.9 fps under 80 | 20 | ok |
| newr1a | 2 | 132.1 | 62 | 293.2 | 2128 | 0.0 |  | load 2128 ms over 2000; 1% low 62 fps under 80 | 28 | ok |
| desert1 | 0 | 131 | 62.1 | 376.1 | 2141.7 | 0.0 |  | load 2141.7 ms over 2000; 1% low 62.1 fps under 80 | 22 | ok |
| newr2a | 1 | 132.1 | 62.2 | 313.3 | 2111.6 | 0.0 |  | load 2111.6 ms over 2000; 1% low 62.2 fps under 80 | 21 | ok |
| bhrndmtn | 0 | 131.9 | 62.3 | 326.4 | 2117.1 | 0.0 |  | load 2117.1 ms over 2000; 1% low 62.3 fps under 80 | 20 | ok |
| cowbomb | 0 | 131.7 | 62.5 | 316.5 | 2147.7 | 0.0 |  | load 2147.7 ms over 2000; 1% low 62.5 fps under 80 | 16 | ok |
| gstfarm | 0 | 90 | 64.8 | 76.6 | 652.9 | 1.0 |  | 1% low 64.8 fps under 80 | 16 | ok |
| sfchina | 0 | 134.8 | 65 | 77.2 | 1421.2 | 0.0 |  | 1% low 65 fps under 80 | 11 | ok |
| encpres | 0 | 136.9 | 72.1 | 45.1 | 821.6 | 1.6 |  | 1% low 72.1 fps under 80 | 6 | ok |
| sfelronb | 0 | 136.4 | 74 | 63.9 | 754.6 | 0.1 |  | 1% low 74 fps under 80 | 17 | ok |
| sftanker | 1 | 133.3 | 79.7 | 80.7 | 1137.4 | 95.9 |  | 1% low 79.7 fps under 80; black 95.9% over 20% | 15 | ok |
| modmain | 0 | 137.5 | 83.8 | 39.8 | 1051.2 | 0.0 |  |  | 8 | ok |
| cave5 | 2 | 137.8 | 86.6 | 36.5 | 735.7 | 25.8 |  | black 25.8% over 20% | 22 | ok |
| broken1 | 0 | 138.8 | 88.3 | 40.1 | 1330.8 | 0.0 |  |  | 15 | ok |
| modinn | 0 | 139.2 | 88.4 | 40.2 | 880.3 | 0.0 |  |  | 14 | ok |
| redment | 0 | 138.5 | 88.7 | 39.4 | 1460.2 | 0.0 |  |  | 10 | ok |
| ncr2 | 0 | 138.4 | 88.8 | 80.6 | 829.4 | 0.0 |  |  | 14 | ok |
| dnslvrun | 1 | 137.2 | 89.2 | 46.7 | 983.8 | 0.7 |  |  | 19 | ok |
| arvillag | 0 | 138.6 | 89.6 | 69.8 | 2317.2 | 0.0 |  | load 2317.2 ms over 2000 | 12 | ok |
| klagraz | 0 | 137.8 | 89.9 | 66.4 | 552.4 | 0.0 |  |  | 14 | ok |
| navarro | 2 | 138.8 | 91.4 | 68.5 | 1607.6 | 0.0 |  |  | 15 | ok |
| geckjunk | 0 | 139.4 | 92.6 | 39.4 | 877.4 | 0.0 |  |  | 14 | ok |
| ncr1 | 0 | 135.3 | 92.8 | 45.4 | 1374.3 | 0.0 |  |  | 26 | ok |
| ncrent | 0 | 138.6 | 93.4 | 58 | 1256.2 | 0.7 |  |  | 15 | ok |
| argarden | 0 | 138.2 | 93.8 | 71.7 | 436.6 | 0.4 |  |  | 12 | ok |
| reddown | 0 | 138.5 | 94.2 | 73.7 | 1316.4 | 0.7 |  |  | 10 | ok |
| denbus1 | 0 | 138.5 | 94.4 | 73.9 | 1167.2 | 0.0 |  |  | 10 | ok |
| newr2 | 0 | 139 | 95 | 39.7 | 1831 | 0.0 |  |  | 27 | ok |
| denbus2 | 0 | 139 | 96.3 | 41.6 | 987.4 | 0.3 |  |  | 10 | ok |
| klamall | 0 | 138.6 | 96.4 | 51.2 | 1132.8 | 0.6 |  |  | 15 | ok |
| modgard | 0 | 138.6 | 97.2 | 46.7 | 337.2 | 0.7 |  |  | 8 | ok |
| ncr3 | 0 | 141 | 97.4 | 11.6 | 980.6 | 0.0 |  |  | 20 | ok |
| gecksetl | 0 | 138.7 | 99.2 | 40.4 | 765.7 | 0.0 |  |  | 24 | ok |
| newrst | 1 | 137.5 | 99.3 | 51.5 | 1047.1 | 49.2 |  | black 49.2% over 20% | 24 | ok |
| kladwtwn | 0 | 138.6 | 99.9 | 39.6 | 1290.3 | 0.7 |  |  | 8 | ok |
| broken2 | 1 | 137.1 | 100.4 | 77.1 | 1051.5 | 26.1 |  | black 26.1% over 20% | 16 | ok |
| newr1 | 0 | 139.1 | 101.9 | 39.8 | 1578.1 | 0.0 |  |  | 24 | ok |
| broken2 | 2 | 139.6 | 102.3 | 51.6 | 1070.7 | 22.9 |  | black 22.9% over 20% | 17 | ok |
| modinn | 1 | 138.1 | 102.4 | 72.5 | 846.1 | 0.0 |  |  | 11 | ok |
| encdet | 0 | 136.7 | 103.1 | 70.7 | 724.6 | 2.2 |  |  | 12 | ok |
| sfdock | 0 | 138.8 | 104 | 39 | 1230.3 | 0.4 |  |  | 19 | ok |
| vctydwtn | 0 | 139.4 | 104.5 | 39.5 | 1050.6 | 0.0 |  |  | 11 | ok |
| dnslvrun | 0 | 139 | 104.9 | 43.1 | 932.9 | 1.0 |  |  | 10 | ok |
| denres1 | 1 | 139.8 | 105.3 | 39.1 | 862 | 2.0 |  |  | 14 | ok |
| redwame | 0 | 141 | 105.4 | 10.1 | 985.8 | 0.0 |  |  | 17 | ok |
| klatrap | 0 | 139.6 | 105.7 | 39.1 | 435 | 2.4 |  |  | 28 | ok |
| encrctr | 0 | 137.8 | 106.5 | 40.7 | 615.5 | 24.3 |  | black 24.3% over 20% | 13 | ok |
| depolva | 2 | 136.3 | 106.6 | 76.7 | 783.6 | 48.9 |  | black 48.9% over 20% | 14 | ok |
| newr3 | 0 | 139.4 | 106.6 | 39.4 | 1249 | 0.5 |  |  | 8 | ok |
| newrst | 0 | 141 | 107.8 | 11.1 | 1043.1 | 5.6 |  |  | 17 | ok |
| klatoxcv | 1 | 141 | 108 | 11.5 | 540.9 | 27.7 |  | black 27.7% over 20% | 4 | ok |
| newr4 | 0 | 139.5 | 108 | 38.9 | 1118.3 | 0.0 |  |  | 16 | ok |
| klaratcv | 0 | 138.4 | 108.9 | 61.4 | 538 | 25.2 |  | black 25.2% over 20% | 11 | ok |
| geckpwpl | 0 | 138.6 | 108.9 | 40.6 | 637.2 | 0.0 |  |  | 22 | ok |
| arcaves | 2 | 138.3 | 109.1 | 70.6 | 567.3 | 46.2 |  | black 46.2% over 20% | 10 | ok |
| vctycocl | 0 | 139.1 | 109.1 | 62.4 | 1055.9 | 0.0 |  |  | 13 | ok |
| arcaves | 0 | 138.4 | 109.4 | 76.2 | 576.9 | 25.9 |  | black 25.9% over 20% | 12 | ok |
| modshit | 0 | 139.3 | 109.5 | 39.1 | 518.8 | 33.2 |  | black 33.2% over 20% | 10 | ok |
| sftanker | 0 | 138.5 | 109.8 | 43.9 | 1111.1 | 61.7 |  | black 61.7% over 20% | 18 | ok |
| reddtun | 0 | 139.2 | 110 | 39.1 | 598.7 | 31.2 |  | black 31.2% over 20% | 13 | ok |
| mbclose | 0 | 139.2 | 110.6 | 54 | 786 | 11.7 |  |  | 8 | ok |
| encgd | 0 | 139.2 | 110.8 | 41 | 715.9 | 0.9 |  |  | 15 | ok |
| broken2 | 0 | 139.3 | 110.8 | 82.4 | 1160.5 | 0.0 |  |  | 12 | ok |
| arcaves | 1 | 138.6 | 111 | 47 | 562.9 | 43.6 |  | black 43.6% over 20% | 9 | ok |
| redwan1 | 0 | 138.8 | 111 | 72.7 | 678.1 | 35.1 |  | black 35.1% over 20% | 18 | ok |
| mbase34 | 2 | 139.3 | 111.2 | 65.8 | 749.6 | 26.7 |  | black 26.7% over 20% | 14 | ok |
| raiders2 | 1 | 138.5 | 111.3 | 40.7 | 683.4 | 19.7 |  |  | 30 | ok |
| mbase34 | 1 | 139.6 | 111.6 | 47.9 | 785 | 28.2 |  | black 28.2% over 20% | 17 | ok |
| klaratcv | 2 | 139.3 | 111.8 | 57.9 | 545 | 20.2 |  | black 20.2% over 20% | 13 | ok |
| newrvb | 0 | 139.7 | 112 | 39.2 | 477.4 | 1.0 |  |  | 13 | ok |
| mbase34 | 0 | 139.7 | 112.1 | 38.9 | 757.3 | 26.7 |  | black 26.7% over 20% | 16 | ok |
| redmtun | 0 | 138.9 | 112.2 | 41.3 | 423.9 | 21.8 |  | black 21.8% over 20% | 10 | ok |
| desert5 | 1 | 139.8 | 112.5 | 40.4 | 469.1 | 94.1 |  | black 94.1% over 20% | 14 | ok |
| raiders1 | 1 | 139.4 | 112.6 | 56.1 | 958.4 | 15.5 |  |  | 19 | ok |
| navarro | 1 | 140 | 112.6 | 39.1 | 1525.9 | 0.0 |  |  | 23 | ok |
| desrt13 | 0 | 139.1 | 112.6 | 40.3 | 352.2 | 0.0 |  |  | 10 | ok |
| v15ent | 0 | 139.4 | 112.7 | 40.6 | 869.2 | 15.5 |  |  | 14 | ok |
| rndtoxic | 0 | 140.4 | 112.7 | 35.3 | 560 | 0.0 |  |  | 16 | ok |
| gstcav1 | 0 | 140.5 | 113 | 39 | 764.1 | 33.4 |  | black 33.4% over 20% | 20 | ok |
| v15ent | 1 | 138.7 | 113.5 | 83 | 863.5 | 15.5 |  |  | 14 | ok |
| vault13 | 2 | 139.8 | 113.9 | 39.3 | 570.4 | 0.0 |  |  | 16 | ok |
| denbus2 | 1 | 139.2 | 114 | 39.1 | 785.5 | 91.1 |  | black 91.1% over 20% | 12 | ok |
| denbus1 | 2 | 139.6 | 114.1 | 39 | 1018.7 | 1.6 |  |  | 14 | ok |
| klaratcv | 1 | 139.6 | 115 | 39 | 532.4 | 25.0 |  | black 25.0% over 20% | 13 | ok |
| arbridge | 0 | 139.5 | 115.1 | 39.2 | 604.9 | 0.0 |  |  | 10 | ok |
| newrba | 0 | 137.8 | 115.8 | 69.7 | 440.6 | 10.6 |  |  | 19 | ok |
| vctyvlt | 2 | 139.1 | 115.8 | 40.3 | 493.4 | 0.0 |  |  | 13 | ok |
| rnduvilg | 0 | 141 | 116.1 | 14.3 | 517.5 | 0.7 |  |  | 13 | ok |
| newr4 | 1 | 141 | 116.4 | 13.5 | 968 | 48.4 |  | black 48.4% over 20% | 16 | ok |
| newr1 | 2 | 139.5 | 116.7 | 39.1 | 1418 | 86.1 |  | black 86.1% over 20% | 26 | ok |
| raiders2 | 2 | 140.1 | 116.8 | 41.7 | 760.7 | 13.5 |  |  | 20 | ok |
| klatoxcv | 0 | 141 | 117.1 | 12.3 | 544.4 | 30.6 |  | black 30.6% over 20% | 18 | ok |
| newr1 | 1 | 140.4 | 117.4 | 38.9 | 1395.6 | 47.1 |  | black 47.1% over 20% | 21 | ok |
| rndcafe | 0 | 139.2 | 117.5 | 39.5 | 715 | 0.0 |  |  | 16 | ok |
| newr3 | 1 | 138.9 | 117.6 | 40.1 | 1120.7 | 32.2 |  | black 32.2% over 20% | 11 | ok |
| enctrp | 0 | 137.9 | 117.9 | 71.7 | 550 | 23.6 |  | black 23.6% over 20% | 12 | ok |
| modwell | 0 | 139.5 | 118.3 | 40 | 406.9 | 29.2 |  | black 29.2% over 20% | 12 | ok |
| encfite | 0 | 139.3 | 118.5 | 46 | 449.3 | 31.5 |  | black 31.5% over 20% | 20 | ok |
| gstcav2 | 0 | 138.6 | 118.8 | 81.4 | 390 | 23.1 |  | black 23.1% over 20% | 13 | ok |
| depolva | 1 | 139.2 | 119 | 70.2 | 785.1 | 17.5 |  |  | 16 | ok |
| sfshutl1 | 0 | 139 | 119 | 73.9 | 620.7 | 15.5 |  |  | 16 | ok |
| v13ent | 0 | 139.3 | 119.6 | 56.6 | 351.4 | 35.0 |  | black 35.0% over 20% | 16 | ok |
| v15sent | 0 | 140.1 | 119.8 | 38.9 | 569.3 | 0.6 |  |  | 11 | ok |
| vctyvlt | 0 | 139.7 | 119.8 | 40.2 | 543.2 | 0.0 |  |  | 7 | ok |
| cave3 | 1 | 140.8 | 120 | 16.5 | 542.5 | 33.6 |  | black 33.6% over 20% | 25 | ok |
| mountn6 | 0 | 139.9 | 120.2 | 39.3 | 552.1 | 0.7 |  |  | 16 | ok |
| denbus1 | 1 | 139.1 | 120.3 | 64.6 | 1014.8 | 32.5 |  | black 32.5% over 20% | 18 | ok |
| rndparih | 0 | 139 | 120.5 | 46.2 | 470 | 0.8 |  |  | 14 | ok |
| sfchina2 | 1 | 139.9 | 120.6 | 48.5 | 805.1 | 16.2 |  |  | 17 | ok |
| newr2 | 2 | 139.8 | 120.8 | 39 | 1586.7 | 15.5 |  |  | 22 | ok |
| rndbridg | 0 | 139.5 | 120.9 | 39.1 | 600 | 0.3 |  |  | 11 | ok |
| cave3 | 0 | 139.3 | 121.1 | 67.3 | 620.9 | 0.0 |  |  | 21 | ok |
| raiders2 | 0 | 139.3 | 121.2 | 53 | 700.2 | 18.4 |  |  | 15 | ok |
| rndbess | 1 | 139.7 | 121.4 | 39.2 | 209.8 | 97.9 |  | black 97.9% over 20% | 12 | ok |
| ncrent | 1 | 138.7 | 121.5 | 53.9 | 1045 | 34.9 |  | black 34.9% over 20% | 8 | ok |
| vault15 | 1 | 140.1 | 121.6 | 39 | 626.2 | 29.4 |  | black 29.4% over 20% | 11 | ok |
| vault13 | 1 | 140.4 | 122.2 | 39.2 | 514.2 | 0.0 |  |  | 19 | ok |
| gecktunl | 0 | 140.1 | 122.3 | 39.2 | 548.2 | 18.9 |  |  | 18 | ok |
| bhrnddst | 0 | 139.2 | 122.3 | 79.4 | 206.6 | 0.0 |  |  | 26 | ok |
| rndwhale | 0 | 139.3 | 123 | 39 | 320.2 | 0.7 |  |  | 17 | ok |
| depolva | 0 | 138.6 | 123.3 | 38.9 | 745.5 | 49.6 |  | black 49.6% over 20% | 15 | ok |
| navarro | 0 | 140.6 | 123.4 | 38.9 | 1519.2 | 0.0 |  |  | 16 | ok |
| sfchina2 | 2 | 139.8 | 123.7 | 39.3 | 775.6 | 88.8 |  | black 88.8% over 20% | 10 | ok |
| vault15 | 2 | 139.5 | 123.7 | 39.1 | 670.3 | 50.9 |  | black 50.9% over 20% | 8 | ok |
| ncr1 | 1 | 140.4 | 123.8 | 39.2 | 1171.9 | 3.3 |  |  | 12 | ok |
| desert7 | 0 | 139.5 | 124 | 39.2 | 429.4 | 0.0 |  |  | 13 | ok |
| rndholy2 | 1 | 140.1 | 124.3 | 39.4 | 687.9 | 35.7 |  | black 35.7% over 20% | 22 | ok |
| newr2 | 1 | 140.3 | 124.6 | 39.1 | 1616.7 | 77.1 |  | black 77.1% over 20% | 22 | ok |
| encfite | 1 | 138.9 | 124.6 | 69.9 | 434.8 | 31.5 |  | black 31.5% over 20% | 11 | ok |
| city5 | 0 | 138.9 | 124.7 | 40.3 | 573.6 | 0.7 |  |  | 21 | ok |
| klatoxcv | 2 | 139.2 | 125 | 39.4 | 553 | 53.7 |  | black 53.7% over 20% | 12 | ok |
| gstcav2 | 1 | 140.1 | 125.6 | 39.1 | 344.3 | 97.6 |  | black 97.6% over 20% | 12 | ok |
| sftanker | 2 | 140.1 | 125.6 | 40.1 | 1031 | 89.2 |  | black 89.2% over 20% | 19 | ok |
| vctyvlt | 1 | 139.5 | 125.8 | 40.4 | 434.8 | 0.0 |  |  | 16 | ok |
| rndtinwd | 0 | 139.5 | 125.9 | 39.6 | 439.8 | 0.7 |  |  | 10 | ok |
| coast4 | 0 | 139.5 | 125.9 | 39.3 | 340.9 | 0.0 |  |  | 11 | ok |
| depolv1 | 1 | 139.5 | 126 | 39 | 669.6 | 53.4 |  | black 53.4% over 20% | 24 | ok |
| depolv1 | 0 | 139.5 | 126 | 39 | 750.1 | 0.0 |  |  | 7 | ok |
| vault13 | 0 | 139.3 | 126 | 39.1 | 536 | 0.0 |  |  | 14 | ok |
| desrt10 | 0 | 140.1 | 126.1 | 39.4 | 487.7 | 0.0 |  |  | 10 | ok |
| rndbhead | 0 | 139.4 | 126.3 | 43.1 | 277.6 | 0.0 |  |  | 20 | ok |
| cardesrt | 0 | 139.2 | 126.5 | 39.1 | 203.1 | 0.0 |  |  | 23 | ok |
| rndforvr | 0 | 139.6 | 126.6 | 39.2 | 474.9 | 0.0 |  |  | 13 | ok |
| cave4 | 0 | 139.2 | 126.7 | 40.9 | 490.7 | 0.3 |  |  | 30 | ok |
| cave2 | 0 | 139.2 | 126.8 | 77.1 | 531.6 | 0.0 |  |  | 30 | ok |
| klamall | 1 | 140.4 | 126.9 | 39.1 | 895.7 | 97.8 |  | black 97.8% over 20% | 10 | ok |
| coast10 | 0 | 138.8 | 126.9 | 50.2 | 509.2 | 0.0 |  |  | 12 | ok |
| city4 | 0 | 139.7 | 127 | 39.1 | 492.5 | 0.7 |  |  | 28 | ok |
| encfite | 2 | 140 | 127.1 | 40.4 | 417.3 | 31.5 |  | black 31.5% over 20% | 9 | ok |
| desrt13 | 1 | 139.5 | 127.2 | 60.8 | 442.6 | 0.0 |  |  | 26 | ok |
| mountn4 | 0 | 140.1 | 127.5 | 38.8 | 581.8 | 0.0 |  |  | 18 | ok |
| klacanyn | 0 | 139.7 | 127.6 | 39.3 | 606.4 | 16.2 |  |  | 8 | ok |
| city6 | 0 | 140.1 | 127.6 | 38.9 | 601.4 | 0.0 |  |  | 30 | ok |
| broken1 | 1 | 140.1 | 127.8 | 43.7 | 1065.6 | 50.4 |  | black 50.4% over 20% | 17 | ok |
| rndshutl | 0 | 138.9 | 127.9 | 78.2 | 262.4 | 0.7 |  |  | 27 | ok |
| mountn1 | 0 | 139.1 | 128 | 59.5 | 340.8 | 0.0 |  |  | 28 | ok |
| sfshutl2 | 0 | 141 | 128.3 | 12 | 114.4 | 18.8 |  |  | 10 | ok |
| encdock | 0 | 139.5 | 128.3 | 39.3 | 375.8 | 18.3 |  |  | 14 | ok |
| city3 | 0 | 139.4 | 128.3 | 66.8 | 561.6 | 0.0 |  |  | 13 | ok |
| rndforvr | 1 | 139.1 | 128.6 | 67.4 | 561.8 | 0.0 |  |  | 14 | ok |
| denres1 | 0 | 139.4 | 128.7 | 40 | 809 | 2.0 |  |  | 16 | ok |
| city1 | 0 | 139.3 | 128.7 | 39 | 552.2 | 0.0 |  |  | 10 | ok |
| city2 | 0 | 140.2 | 128.9 | 38.9 | 574.6 | 0.0 |  |  | 17 | ok |
| mbase12 | 1 | 139.9 | 129 | 39.1 | 662 | 21.0 |  | black 21.0% over 20% | 13 | ok |
| rndbess | 0 | 139.8 | 129 | 39.3 | 245.1 | 15.5 |  |  | 10 | ok |
| mountn5 | 0 | 140.1 | 129 | 39 | 545.7 | 0.7 |  |  | 12 | ok |
| raiders1 | 0 | 140.4 | 129.1 | 39 | 850.9 | 0.3 |  |  | 9 | ok |
| vctyctyd | 0 | 140.1 | 130.2 | 40.2 | 997.4 | 10.9 |  |  | 11 | ok |
| desert5 | 0 | 139.8 | 130.3 | 58.5 | 427.9 | 0.0 |  |  | 9 | ok |
| depolvb | 0 | 141 | 130.4 | 11.6 | 588.6 | 14.9 |  |  | 9 | ok |
| coast2 | 0 | 139.3 | 130.5 | 55.8 | 266 | 0.0 |  |  | 15 | ok |
| vault15 | 0 | 140.9 | 131.1 | 20.2 | 653 | 35.1 |  | black 35.1% over 20% | 12 | ok |
| cave7 | 0 | 140.8 | 131.2 | 38.9 | 432.3 | 35.1 |  | black 35.1% over 20% | 23 | ok |

## Flagged valid rows

- load over 2000 ms: artemple:e0, arvill2:e0, newr1a:e1, newr2a:e2, newr1a:e2, desert1:e0, newr2a:e1, bhrndmtn:e0, cowbomb:e0, arvillag:e0
- 1% low under 80 fps: dnslvrun:e2, cave2:e1, modbrah:e0, sfchina2:e0, redwan1:e1, rndexcow:e0, ardead:e0, ncr4:e0, artemple:e0, arvill2:e0, newr1a:e1, newr2a:e2, newr1a:e2, desert1:e0, newr2a:e1, bhrndmtn:e0, cowbomb:e0, gstfarm:e0, sfchina:e0, encpres:e0, sfelronb:e0, sftanker:e1
- with a 2D fallback line: none
- black over 20%: cave2:e1, redwan1:e1, sftanker:e1, cave5:e2, newrst:e1, broken2:e1, broken2:e2, encrctr:e0, depolva:e2, klatoxcv:e1, klaratcv:e0, arcaves:e2, arcaves:e0, modshit:e0, sftanker:e0, reddtun:e0, arcaves:e1, redwan1:e0, mbase34:e2, mbase34:e1, klaratcv:e2, mbase34:e0, redmtun:e0, desert5:e1, gstcav1:e0, denbus2:e1, klaratcv:e1, newr4:e1, newr1:e2, klatoxcv:e0, newr1:e1, newr3:e1, enctrp:e0, modwell:e0, encfite:e0, gstcav2:e0, v13ent:e0, cave3:e1, denbus1:e1, rndbess:e1, ncrent:e1, vault15:e1, depolva:e0, sfchina2:e2, vault15:e2, rndholy2:e1, newr2:e1, encfite:e1, klatoxcv:e2, gstcav2:e1, sftanker:e2, depolv1:e1, klamall:e1, encfite:e2, broken1:e1, mbase12:e1, vault15:e0, cave7:e0

## Not valid results

- cave0:e0 ok after 1 attempts: measured under CPU load over the noisy line
- cave1:e0 ok after 1 attempts: measured under CPU load over the noisy line
- cave1:e1 ok after 1 attempts: measured under CPU load over the noisy line
- cave4:e1 ok after 1 attempts: measured under CPU load over the noisy line
- cave5:e0 ok after 1 attempts: measured under CPU load over the noisy line
- cave5:e1 ok after 1 attempts: measured under CPU load over the noisy line
- cave6:e0 ok after 1 attempts: measured under CPU load over the noisy line
- city7:e0 ok after 1 attempts: measured under CPU load over the noisy line
- city8:e0 ok after 1 attempts: measured under CPU load over the noisy line
- coast1:e0 ok after 1 attempts: measured under CPU load over the noisy line
- coast11:e0 failed after 1 attempts: warmup dev-snapshot exited 1
- coast12:e0 failed after 1 attempts: warmup dev-snapshot exited 1
- coast2:e1 ok after 1 attempts: measured under CPU load over the noisy line
- coast3:e0 failed after 1 attempts: warmup dev-snapshot exited 1
- coast5:e0 failed after 1 attempts: warmup dev-snapshot exited 1
- coast5:e1 failed after 1 attempts: warmup dev-snapshot exited 1
- coast6:e0 failed after 1 attempts: warmup dev-snapshot exited 1
- coast7:e0 failed after 1 attempts: warmup dev-snapshot exited 1
- coast7:e1 failed after 1 attempts: warmup dev-snapshot exited 1
- coast8:e0 failed after 1 attempts: warmup dev-snapshot exited 1
- coast9:e0 failed after 1 attempts: warmup dev-snapshot exited 1
- desert2:e0 ok after 2 attempts: measured under CPU load over the noisy line
- desert3:e0 ok after 2 attempts: measured under CPU load over the noisy line
- desert4:e0 ok after 2 attempts: measured under CPU load over the noisy line
- desert6:e0 failed after 1 attempts: warmup dev-snapshot exited 1
- desert6:e1 failed after 1 attempts: warmup dev-snapshot exited 1
- desert8:e0 failed after 1 attempts: warmup dev-snapshot exited 1
- desert9:e0 failed after 1 attempts: warmup dev-snapshot exited 1
- desrt11:e0 failed after 1 attempts: warmup dev-snapshot exited 1
- desrt12:e0 failed after 1 attempts: warmup dev-snapshot exited 1
- desrt13:e2 ok after 1 attempts: measured under CPU load over the noisy line
- mbase12:e0 ok after 2 attempts: measured under CPU load over the noisy line
- mountn2:e0 ok after 1 attempts: measured under CPU load over the noisy line
- mountn3:e0 ok after 1 attempts: measured under CPU load over the noisy line
- newrcs:e0 ok after 2 attempts: measured under CPU load over the noisy line
- newrgo:e0 ok after 2 attempts: measured under CPU load over the noisy line
- newrgo:e1 ok after 2 attempts: measured under CPU load over the noisy line
- rndholy1:e0 ok after 1 attempts: measured under CPU load over the noisy line
- rndholy2:e0 ok after 1 attempts: measured under CPU load over the noisy line

## Method

- The map list is the exporter's map groups (`tools/map_health_maps.py`): 152 maps with squares in the game data, 223 map elevations. The map file decides the elevations; no map is missing.
- Every row is one warmup run (the scene cache and art fills land on disk) and one measured run (F2CE_PERF=1, the frame log on, a held run folder), exactly perf-check's launch and its perf lines. Load ms is the last `perf load first_frame` mark of the run's load chain; fps numbers come from the play-only frame rows after the last load row's first second.
- Each row holds the machine-wide measurement lock (290) across its warmup and measured run. The cpu column is the CPU load sampled at the start of the measurement; over 30 percent the row is noisy and not a result.
- black % is the share of pixels whose brightest channel is under 8 of 255 in the row's snapshot PNG (the full frame, interface included). A night cave is allowed to be dark; a 90 plus percent row is a mostly black frame.
- The fallback column marks a run that printed a 2D fallback line ("stays 2D" or "uses 3D ground plane"). No measured row printed one.
- All rows were measured on one build: exe sha256 de20f410dc47a6a9 from main commit de6d5267. The table builder refuses rows that mix builds.
- A background run is paced differently from a real window (157): the numbers guard against a regression, they do not prove the owner's real-window bar.

## Honest limits

- 16 rows were never measured: their warmup run hit the 30 s step watchdog because the map's scene cache was still missing (entering an uncached map stalls past the watchdog; that stall itself is data for 272 and 273). desert2, desert3 and desert4 recovered on their second attempt once the caches existed. The remaining 16 (coast3, coast5, coast6, coast7, coast8, coast9, coast11, coast12, desert6, desert8, desert9, desrt11, desrt12 and their elevations) have their caches pre-exported now; the rows wait for a free measurement slot.
- 23 rows are noisy: the machine was above the CPU line when they were measured, so they say nothing and need one rerun on a free machine. Their frame numbers still hint where to look (coast2:e1 black 99.8 percent, newrgo:e1 51.4 percent, cave6:e0 35.1 percent), but only a clean rerun turns a hint into a row.
- This is a truthful incomplete report: 184 valid rows of 223. No invalid row is counted anywhere above; the next sweep resumes and only touches these 39 rows.
