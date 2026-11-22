# 325 follow-up: retain local 3D while entering barter

Windows Klamath, actual Sulik dialogue, real B and Escape input routes in an isolated game. The fixture verified both trade totals in the live window and a return to the conversation.

The old renderer blocked local 3D while the ordinary trade animation used the legacy special mode. This patch recognizes that one local dialogue transition, as it already does for closing the dialogue panel. It does not change mode flags, audio, input or the animation. World map, movies, saves, loads and map transitions remain guarded.

| Captured native frames | Before | After |
| --- | ---: | ---: |
| Entering trade, all sampled | 42 | 41 |
| Entering trade, legacy special mode | 39 | 39 |
| Entering trade, local 3D off | 39 | 0 |
| Return to the same conversation, local 3D off | 0 | 0 |

A temporary sampler also found no frame where all three sampled points were black on either pass. This is not a claim about every screen pixel or physical keyboard input. Focused preview policy, sprite card, game view and dialogue layout tests passed. The sampler is not in the patch.
