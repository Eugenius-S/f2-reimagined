#ifndef FALLOUT_FPS_COUNTER_H
#define FALLOUT_FPS_COUNTER_H

#include <algorithm>
#include <cstddef>
#include <vector>

namespace fallout {

// One logged present: the time since the previous present, and whether the
// file read progress handler drove it (a loading screen, 250).
struct FrameLogSample {
    double milliseconds = 0.0;
    bool load = false;
};

struct FrameLogSummary {
    unsigned int playFrames = 0;
    unsigned int loadFrames = 0;
    double averageFps = 0.0;
    double onePercentLowFps = 0.0;
    double onePercentLowMs = 0.0;
    double worstMs = 0.0;
};

// 250: the summary math is pure, so a test can pin it. Play frames only:
// loading-screen presents never enter the average, the 1% low or the worst;
// they are counted separately.
inline FrameLogSummary frameLogSummary(const std::vector<FrameLogSample>& frames)
{
    FrameLogSummary summary;
    std::vector<double> play;
    play.reserve(frames.size());
    for (const FrameLogSample& sample : frames) {
        if (sample.load) {
            summary.loadFrames++;
        } else {
            play.push_back(sample.milliseconds);
        }
    }
    summary.playFrames = static_cast<unsigned int>(play.size());
    if (play.empty()) {
        return summary;
    }

    std::sort(play.begin(), play.end());
    double total = 0.0;
    for (double ms : play) {
        total += ms;
    }
    summary.averageFps = static_cast<double>(play.size()) * 1000.0 / total;
    // The 1% low: the frame time the slowest 1% of the frames goes past.
    const std::size_t index = play.size() > 1 ? play.size() * 99 / 100 : 0;
    summary.onePercentLowMs = play[index];
    summary.onePercentLowFps = 1000.0 / summary.onePercentLowMs;
    summary.worstMs = play.back();
    return summary;
}

// 127: the frame-time log ([3d] frame_log, --dev-frame-log): every frame's
// time to a CSV, a summary (average FPS, the 1% low, the worst frame) when
// the log closes. The readout itself lives in renderPresent (svga.cc).
// 250: the argument marks a present driven by the file read progress
// handler (a loading screen); its row carries load=1 and stays out of the
// summary. The clock is SDL_GetPerformanceCounter, and opening a log
// starts a fresh interval, so a pause with the log off never lands in the
// first row of the new log.
void fpsCounterOnFrame(bool loadingFrame = false);
void fpsCounterFlushLog();

} // namespace fallout

#endif /* FALLOUT_FPS_COUNTER_H */
