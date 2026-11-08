# 172: the map-change phase tables

All values in ms, from the chain start (the engine's mapLoadByName entry)
to the mark. `first_frame` = the first full 3D frame. Measured with
F2CE_PERF=1 in dev runs (dev-snapshot, view tile 25075, elevation 0),
the scene cache warm on disk. The stage scopes (floor/walls/... sub-phases)
live in each run's logs/devperf.log.

## before kladwtwn (run 994e62e8a897)

```
perf load chain map_change begin
perf load map_tiles ms=44.66 since chain start
perf load map_scripts ms=45.37 since chain start
perf load map_objects ms=1366.66 since chain start
perf load map_elevation ms=1367.46 since chain start
perf load map_load ms=1821.68 since chain start
perf load floor ms=96.65
perf load walls ms=227.74
perf load projection ms=0.09
perf load scene_loaded ms=2193.36 since chain start
perf load scene_total ms=333.47
perf load first_frame ms=2486.92 since chain start
```

## before newr1 (run c6c5e8e67461)

```
perf load chain map_change begin
perf load map_tiles ms=70.03 since chain start
perf load map_scripts ms=70.74 since chain start
perf load map_objects ms=1972.24 since chain start
perf load map_elevation ms=1973.09 since chain start
perf load map_load ms=2545.56 since chain start
perf load floor ms=96.38
perf load walls ms=245.32
perf load projection ms=0.10
perf load scene_loaded ms=2901.26 since chain start
perf load scene_total ms=350.26
perf load first_frame ms=3376.74 since chain start
```

## before vctyctyd (run bb71451cafca)

```
perf load chain map_change begin
perf load map_tiles ms=51.38 since chain start
perf load map_scripts ms=52.15 since chain start
perf load map_objects ms=1342.18 since chain start
perf load map_elevation ms=1342.98 since chain start
perf load map_load ms=1640.86 since chain start
perf load floor ms=54.00
perf load walls ms=183.27
perf load projection ms=0.07
perf load scene_loaded ms=1891.43 since chain start
perf load scene_total ms=244.34
perf load first_frame ms=2298.74 since chain start
```

## before cave0 (run 32b6f8ace55d)

```
perf load chain map_change begin
perf load map_tiles ms=47.10 since chain start
perf load map_scripts ms=47.65 since chain start
perf load map_objects ms=114.85 since chain start
perf load map_elevation ms=115.39 since chain start
perf load map_load ms=243.84 since chain start
perf load floor ms=36.00
perf load walls ms=12.96
perf load projection ms=0.08
perf load scene_loaded ms=305.15 since chain start
perf load scene_total ms=52.45
perf load first_frame ms=484.39 since chain start
```

## after (run 4f452ae4bbb9)

```
perf load objects_load ms=427.26
perf load objects_load ms=0.00
perf load objects_load ms=0.00
perf load objects_light ms=237.61
perf load map_objects ms=727.38 since chain start
perf load map_elevation ms=734.85 since chain start
perf load art_preload ms=446.50
perf load map_load ms=1227.32 since chain start
perf load floor ms=93.00
perf load walls ms=230.96
perf load projection ms=0.23
perf load scene_loaded ms=1598.46 since chain start
perf load scene_total ms=334.07
perf load first_frame ms=1945.83 since chain start
```

## after (run c9ea9e8e90f9)

```
perf load objects_load ms=488.53
perf load objects_load ms=44.16
perf load objects_load ms=25.19
perf load objects_light ms=297.37
perf load map_objects ms=932.33 since chain start
perf load map_elevation ms=1039.45 since chain start
perf load art_preload ms=525.26
perf load map_load ms=1628.18 since chain start
perf load floor ms=98.08
perf load walls ms=299.95
perf load projection ms=0.08
perf load scene_loaded ms=2072.92 since chain start
perf load scene_total ms=405.48
perf load first_frame ms=2519.49 since chain start
```

## after (run 5d86ef7e7aa9)

```
perf load objects_load ms=334.47
perf load objects_load ms=0.00
perf load objects_load ms=0.00
perf load objects_light ms=259.65
perf load map_objects ms=650.93 since chain start
perf load map_elevation ms=694.85 since chain start
perf load art_preload ms=351.31
perf load map_load ms=1086.73 since chain start
perf load floor ms=53.30
perf load walls ms=189.54
perf load projection ms=0.08
perf load scene_loaded ms=1352.43 since chain start
perf load scene_total ms=249.35
perf load first_frame ms=1756.95 since chain start
```

## after (run e4a245b5ef68)

```
perf load objects_load ms=58.10
perf load objects_load ms=0.00
perf load objects_load ms=0.00
perf load objects_light ms=0.11
perf load map_objects ms=119.67 since chain start
perf load map_elevation ms=120.37 since chain start
perf load art_preload ms=130.73
perf load map_load ms=255.69 since chain start
perf load floor ms=35.30
perf load walls ms=11.19
perf load projection ms=0.11
perf load scene_loaded ms=311.36 since chain start
perf load scene_total ms=49.66
perf load first_frame ms=462.01 since chain start
```


## the parallel proto warm (2026-11-08, glm)

The planned lever (the light spread on worker threads) turned out to be
aimed at the wrong cost: with the protos warm the whole spread walk is
~2 ms. The rebuild's real cost was the lazy proto loads - the walk reads
wall extendedFlags per visited tile and every miss pays a .lst and a .pro
file read (~2 ms each, 219 unique wall protos on newr1). So this round
parallelizes the proto warm instead: the unique wall pids parse over up to
4 threads (pure file reads, no list touches), the inserts stay on the main
thread, the spread keeps the vanilla shape. The spread result is
pixel-identical (0 changed px against the before runs' snapshots on all
four maps).

Second chain, warm OS cache, view tile 25075, elevation 0
(kladwtwn 5d5d0b2cd2e4 / newr1 5a825a4592af + 21a04d543180 /
vctyctyd 13e788383eff / cave0 71f851ba8218; before rows from
5fe157771fa3 / 6ebfe956afba / 4daf112f8c99 / df764d284470):

| map | objects_light | first_frame before | first_frame after |
| --- | --- | --- | --- |
| kladwtwn | 238 -> 48 | 1933 | 1793 |
| newr1 | 295/297 -> 46/48 | 2609/2524 | 2397/2319 |
| vctyctyd | 260 -> 45 | 1827 | 1508 |
| cave0 | 0.1 -> 0.8 | 638 | 619 |
