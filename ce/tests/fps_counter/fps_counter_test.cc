#include <cstdio>
#include <cstdlib>
#include <vector>

#include "fps_counter.h"

namespace {

int failures = 0;

void expectNear(const char* what, double got, double want, double epsilon)
{
    if (got < want - epsilon || got > want + epsilon) {
        printf("fps_counter_tests: FAIL %s got %.6f want %.6f\n", what, got, want);
        failures++;
    }
}

void expectEqual(const char* what, unsigned int got, unsigned int want)
{
    if (got != want) {
        printf("fps_counter_tests: FAIL %s got %u want %u\n", what, got, want);
        failures++;
    }
}

} // namespace

int main()
{
    // 250: one long first frame (the stale-interval shape the audit caught),
    // 119 ordinary 10 ms play frames, three loading-screen presents that
    // must never enter the play numbers.
    std::vector<fallout::FrameLogSample> frames;
    frames.push_back({1000.0, false});
    for (int i = 0; i < 119; i++) {
        frames.push_back({10.0, false});
    }
    frames.push_back({500.0, true});
    frames.push_back({500.0, true});
    frames.push_back({500.0, true});

    const fallout::FrameLogSummary summary = fallout::frameLogSummary(frames);
    expectEqual("playFrames", summary.playFrames, 120);
    expectEqual("loadFrames", summary.loadFrames, 3);
    expectNear("averageFps", summary.averageFps, 120.0 * 1000.0 / 2190.0, 0.0001);
    expectNear("onePercentLowMs", summary.onePercentLowMs, 10.0, 0.0001);
    expectNear("onePercentLowFps", summary.onePercentLowFps, 100.0, 0.0001);
    expectNear("worstMs", summary.worstMs, 1000.0, 0.0001);

    // Only load frames: nothing to average, nothing crashes.
    const fallout::FrameLogSummary loadOnly
        = fallout::frameLogSummary({{400.0, true}, {400.0, true}});
    expectEqual("loadOnly playFrames", loadOnly.playFrames, 0);
    expectEqual("loadOnly loadFrames", loadOnly.loadFrames, 2);
    expectNear("loadOnly averageFps", loadOnly.averageFps, 0.0, 0.0001);

    // Empty log.
    const fallout::FrameLogSummary empty = fallout::frameLogSummary({});
    expectEqual("empty playFrames", empty.playFrames, 0);
    expectEqual("empty loadFrames", empty.loadFrames, 0);
    expectNear("empty worstMs", empty.worstMs, 0.0, 0.0001);

    if (failures != 0) {
        printf("fps_counter_tests: %d failure(s)\n", failures);
        return 1;
    }
    printf("fps_counter_tests: PASS\n");
    return 0;
}
