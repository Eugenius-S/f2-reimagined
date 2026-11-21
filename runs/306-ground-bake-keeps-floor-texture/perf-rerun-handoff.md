# 306: повторный замер скорости и остаток приёмки (handoff)

Для: Main или тот, кто проведёт замер после ревью 310 (PR #521, SHA 5aa7712b). Ничего здесь не запускалось. PR #517 остаётся draft, пока не выполнены шаги 1-4.

## 0. Условия входа

Все команды для PowerShell, из папки `../fallout2-ce-3d-claude-306`, если не сказано иное.

- Независимое ревью #521 дало PASS, #521 влит в main. Если ревью потребовало правок, ниже везде берётся принятый SHA, а не 5aa7712b.
- Тяжёлый слот свободен. Проверка: ни `fallout2-ce.exe`, ни `scene-exporter.exe`, ни чужого perf-check; мьютекс `Global\fallout2-ce-perf-check` свободен. perf-check сам ждёт мьютекс (`-WaitMinutes 30`), но installer-проверки GLM его не берут, поэтому окно согласуется вручную.

## 1. Две сборки с одним и тем же принятым инструментом

perf-check меряет exe своего же worktree (`$Root\build\fallout2-ce.exe`), поэтому инструмент и сборка должны быть в одном дереве. Оба worktree принадлежат claude, они рядом с основным checkout. Основной checkout не трогать.

До (main с 310, без 306):

```
git -C ../fallout2-ce-3d-claude-306-main fetch origin
git -C ../fallout2-ce-3d-claude-306-main checkout --detach origin/main
cd ../fallout2-ce-3d-claude-306-main; powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release
```

После (306 поверх того же main):

```
git -C ../fallout2-ce-3d-claude-306 fetch origin
git -C ../fallout2-ce-3d-claude-306 merge --no-edit origin/main
git -C ../fallout2-ce-3d-claude-306 push origin HEAD:task/306-ground-bake-keeps-floor-texture
cd ../fallout2-ce-3d-claude-306; powershell -NoProfile -ExecutionPolicy Bypass -File tools/configure-build.ps1 Release
```

Проверка, что разница только в рендерере 306:

```
git -C ../fallout2-ce-3d-claude-306 diff --stat origin/main HEAD -- ce tools
```

Ожидается одна строка: `ce/src/scene_renderer/scene_draw.cc`. В `tools/perf-check.ps1` обоих деревьев должно быть `PercentProcessorTime` (`git grep -c PercentProcessorTime -- tools/perf-check.ps1`). Сцены одни и те же: `build/scene-cache` в main-дереве является junction на кеш 306.

Запасной путь, если #521 одобрен, но ещё не влит: в обоих деревьях `git checkout <принятый SHA> -- tools/perf-check.ps1` (без коммита), после замера `git checkout HEAD -- tools/perf-check.ps1`. Сборку при этом не пересобирать.

## 2. Замер (последовательно, ничего параллельно)

```
cd ../fallout2-ce-3d-claude-306-main; powershell -NoProfile -ExecutionPolicy Bypass -File tools/perf-check.ps1; echo "exit $LASTEXITCODE"
cd ../fallout2-ce-3d-claude-306;      powershell -NoProfile -ExecutionPolicy Bypass -File tools/perf-check.ps1; echo "exit $LASTEXITCODE"
```

Гейт (пороги не менять, флагов не добавлять):
- оба прогона: exit 0, строка `perf-check: PASS`, ни одной строки с `noisy` и ни одного `?` в колонке cpu;
- exit 2 (NOISY): это не результат. Повторить в другом свободном окне, а не переписывать вывод;
- exit 1 (FAIL) у 306 при PASS у main: регрессия, мерж запрещён, разбор в Log 306;
- в Log 306 записать обе таблицы целиком, cpu каждой строки и exit-коды.

perf-check идёт без теней, поэтому путь запекания он не проходит. Его роль: доказать, что 306 не задел обычный путь.

## 3. Цена запекания (с тенями, информативно, не гейт)

Под тем же мьютексом, чтобы не наложиться на чужой замер. По 2 повтора, main и 306 вперемешку:

```
$m = New-Object Threading.Mutex($false, 'Global\fallout2-ce-perf-check'); [void]$m.WaitOne(1800000)
try {
  foreach ($r in 1, 2) { foreach ($w in '../fallout2-ce-3d-claude-306-main', '../fallout2-ce-3d-claude-306') {
    foreach ($map in @('-Map', 'klamall', '-ViewTile', '28096'), @('-Map', 'arvillag')) {
      $cpu = (Get-CimInstance Win32_PerfFormattedData_PerfOS_Processor -Filter "Name='_Total'").PercentProcessorTime
      $o = & powershell -NoProfile -ExecutionPolicy Bypass -File "$w/tools/dev-snapshot.ps1" @map -Elevation 0 -Hour 8 -Zoom 1 `
        -Shadows -Set3d 'frame_log=1' -HoldSeconds 10 -KeepRunFolder -OutputDirectory "$w/build/t306-bake-cost"
      "$r $w $($map[1]) cpu=$cpu " + (($o | Select-String 'RUN FOLDER: ') -replace '.*RUN FOLDER: ', '')
  } } }
} finally { $m.ReleaseMutex(); $m.Dispose() }
```

Каждую напечатанную папку разобрать через `py -3 ../claude306-perf.py <папка>/frame_log.csv`. Это то же правило, что `Read-FrameLogSummary` в perf-check. Строки с cpu выше 30 считать noisy. Записать в Log 306 средний FPS и 1% low до и после. Ожидание: исправленное запекание делает реальную работу (свет и тень на четверти пикселей), поэтому небольшая цена против сломанного режима 0 нормальна. Она не должна опускать 1% low ниже 80 (принятая планка владельца).

## 4. Приёмка 306: что доказано и чего не хватает

| Требование | Доказательство | Статус |
|---|---|---|
| Точная причина и общее правило | Log: два правила, вариант-тест (только FBO: белое 62.4%) | есть |
| Исходная команда KLAMALL даёт текстуру | `before-after.jpg` ряд 1, near-white 62.4% -> 0.0% | есть |
| Замеры области пола | near-white и mean по всему кадру и нижней полосе, \|d\| к no-bake | есть |
| Вторая карта | ARVILLAG 73.7% -> 0.0%, \|d\| 1.53 | есть |
| Смена размера | 1366x767: 51.4% -> 0.0% | есть |
| Свечение (FBO ≠ 0) | чёрный кадр -> mean 53.7 | есть |
| Тени и свет сохранены | днём: тени на листе, \|d\| к no-bake 1.17/1.53 | **частично**: точечный свет ночью и в пещерах не снят |
| Сфокусированная проверка | не добавлена, причина в Log | **пробел, обоснован** |
| CTest, регрессия, облёт | 29/29, PASS, 0 из 40 | есть, но регрессия и облёт идут без теней и путь 306 не проходят |
| Лист изменённых видов облёта | `tour-changed.jpg`: изменений нет | формально есть; намеренная разница видна только на `before-after.jpg` |
| perf до и после | шаги 2-3 | **ждёт** |
| Ссылка в 270 | строка в Log 270 | есть |

Лёгкая добивка без тяжёлого слота, два снимка примерно по 15 с: ночь на KLAMALL с запеканием и без, чтобы закрыть «точечный свет»:

```
cd ../fallout2-ce-3d-claude-306
powershell -NoProfile -ExecutionPolicy Bypass -File tools/dev-snapshot.ps1 -Map klamall -Elevation 0 -ViewTile 28096 -Hour 22 -Zoom 1 -Still -Shadows -OutputDirectory build/t306-night-bake
$env:F2CE_NO_BAKE='1'; powershell -NoProfile -ExecutionPolicy Bypass -File tools/dev-snapshot.ps1 -Map klamall -Elevation 0 -ViewTile 28096 -Hour 22 -Zoom 1 -Still -Shadows -OutputDirectory build/t306-night-nobake; Remove-Item Env:F2CE_NO_BAKE
py -3 ../claude306-cmp.py build/t306-night-bake/*.png build/t306-night-nobake/*.png
```

Ожидание: mean |d| около 1-2, пикселей с d > 16 меньше 0.1%. Больше: свет в запекании расходится с полом без запекания, это разобрать до мержа.

## 5. Остаточные риски (после мержа их не закрывает никто)

- Ни регрессия, ни облёт, ни perf-check не включают тени. Поломка запекания снова пройдёт незамеченной (так случилось после 157). Нужна отдельная задача: одна точка регрессии с `-Shadows`.
- Непроверенное наблюдение: viewport запекания берёт x, y полного viewport и делит пополам только размер. При ненулевом начале запекание съедет. Сейчас начало нулевое.
- `glGetIntegerv` в `ensureGroundBake` выполняется только при пересоздании буфера (смена размера), не каждый кадр.
- `map-health.ps1` (GLM, #516) судит о шуме своим правилом; на тот же счётчик 310 не переведён.

## 6. После PASS шагов 2-4

`gh pr ready 517`, затем прочитать все проверки CI, squash merge с удалением ветки, затем `update_board.py`. Дальше правило 1 по точному коммиту: Release из свежего origin/main, один запуск dev-snapshot, `tools/perf-check.ps1`, exe в f2r-verified со строкой в version.txt, `gh workflow run verify.yml -f commit=<commit>` и `gh run watch <id> --exit-status`. До SUCCESS у владельца остаётся прежний проверенный exe (c1b9afe).
