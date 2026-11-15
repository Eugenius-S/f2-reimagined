# RP installation rollback and removal

The old installer was reproduced with a local `--rp-zip-url` package and a file blocking a destination parent. The RP-only operation failed after moving originals and adding package files, leaving different file lists and hashes. The fixed operation restores the exact original file list and SHA-256 hashes.

The disposable integration suite covers 26 cases: actual locked copy/remove/cleanup failures, fresh install and full uninstall equality, package saves, manual RP, Russian/custom mod-order lines, changed player files and backups, invalid paths and manifests, junctions and hardlinks, interrupted recovery, checked vanilla scene refresh, missing/failed exporter retry, and real window checkbox/Play state. Fake-exporter control and logs stay outside the synthetic game folder. No installed game was used or modified.

Validation:

- Release build: passed.
- CTest: 29/29 passed, including all 26 RP integration cases.
- Exporter Python tests: 148 passed.
- Regression-tool Python tests: 12 passed.
- Full pixel regression: 154/154 passed, with references and thresholds unchanged. 150 views matched exactly. `newreno-first` and `ncr-ncr1-e0` each differed by two pixels; `ncr-ncr3-e0` and `ncr-ncrent-e0` each differed by one, the existing baseline.
- English and Russian real-window removal checks: passed. The checkbox is enabled and ticked before removal, clear after successful scene refresh; CE stays installed. Failed refresh or a partial restore keeps Play disabled and the checkbox available for recovery.

The official 900 MB RP download was not exercised. Unknown contents after an interrupted operation are preserved with an error and backup, rather than overwritten. Full mod removal restores RP first and stops if recovery fails; RP-only removal refreshes vanilla scenes before clearing its receipt.
