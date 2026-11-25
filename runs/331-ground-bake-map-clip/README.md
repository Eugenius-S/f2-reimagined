# Ground bake clip check

Baseline: `896dbb1eed18d43d9debd13ad742c76863348408`. Candidate code: `dd2bbe43f5e910d0664097d96febf85a2a964390`. Windows Release, isolated test game and copied scene cache.

`static-ab.jpg`: KLADWTWN e0, view tile 20100, 1920x1080, zoom 1, sun shadows on, 10:00, ambient 20000. The full-size matched stills differ in 0 of 2,073,600 pixels. Baseline executable SHA256 `57c0523c6b5b6173248e97b26cb45300da497e8e9feb236478d0e526a71afcd7`; candidate `f334930d724a1e54422d969550a014ab89909819f28a06e3e18d683c1d39df92`. Temporary local full images and metadata are in `build/baseline-clean/` and `build/candidate-clean/`.

A temporary scoped probe changed plane 0 offset by -50 at a stationary camera on the same map and settings, after 154 frames. This extreme plane is a targeted cache test, not a real map boundary. Baseline frame 155 reused the old bake with no shadow or light pulse; frame 156 forced a new bake. All 518,400 bake pixels differed: SHA256 stale `d367a2443f7e174ae7ffaa39ab126c0332b6f4b53182b75cfe1ac8361131ceee`, fresh `c216e1ed5c0b833b02a2e42bc95e2ccd6afe291b9e71b9ad69bb9bd8a3228d1f`. Candidate frame 155 baked with no shadow or light pulse and matched that fresh hash; frame 157 reused the stable bake. Local raw buffers: `build/baseline-plane-155.raw`, `build/baseline-plane-156.raw`, `build/candidate-probe-155.raw`, `build/candidate-probe-156.raw`; console logs: `build/baseline-probe-plane/`, `build/candidate-probe-klamath/`. The diagnostic code is not committed.

The candidate also rebaked when the effective clip was disabled, matched a forced fresh bake, and reused it on the next stable frame. New Reno `newr1` e0, view tile 14999, zoom 1.28, 10:00, sun shadows: the baseline reused after the clip was disabled, but its bake was completely white and matched the forced result. This does not establish that the selected view showed floor beyond the clip. The actual Clean map edges control and a visible gameplay defect remain unverified.

Windows Release build passed. CTest: 34/34 passed. The final commit adds only this evidence and the task log to the candidate code.
