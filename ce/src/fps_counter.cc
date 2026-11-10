#include "fps_counter.h"

#include <cstdio>
#include <vector>

#include <SDL.h>

#include "dev_output.h"
#include "platform_compat.h"
#include "settings.h"

namespace fallout {

namespace {

struct FrameLogState {
    bool open = false; // whether the open CSV belongs to the current setting
    FILE* stream = nullptr;
    std::vector<FrameLogSample> frames;
    // 250: the performance counter of the previous present, reset when a log
    // opens, so the pause with the log off never lands in the first row.
    Uint64 lastCounter = 0;
    unsigned int loggedFrames = 0;
};

FrameLogState& state()
{
    static FrameLogState value;
    return value;
}

void closeLog()
{
    FrameLogState& log = state();
    if (!log.open) return;
    log.open = false;
    if (log.stream != nullptr) {
        fclose(log.stream);
        log.stream = nullptr;
    }

    if (log.frames.empty()) return;

    const FrameLogSummary summary = frameLogSummary(log.frames);
    printf("frame log: %u play frames, average %.1f fps, 1%% low %.1f fps (%.1f ms), worst %.1f ms\n",
        summary.playFrames, summary.averageFps, summary.onePercentLowFps,
        summary.onePercentLowMs, summary.worstMs);
    printf("frame log: %u load frames excluded\n", summary.loadFrames);
    fflush(stdout);

    const std::filesystem::path summaryPath = dev_output::outputFile("frame_log.summary.txt");
    FILE* summaryFile = compat_fopen(summaryPath.string().c_str(), "w");
    if (summaryFile != nullptr) {
        fprintf(summaryFile, "frames,%u\n", summary.playFrames);
        fprintf(summaryFile, "load_frames,%u\n", summary.loadFrames);
        fprintf(summaryFile, "average_fps,%.2f\n", summary.averageFps);
        fprintf(summaryFile, "one_percent_low_fps,%.2f\n", summary.onePercentLowFps);
        fprintf(summaryFile, "one_percent_low_ms,%.2f\n", summary.onePercentLowMs);
        fprintf(summaryFile, "worst_ms,%.2f\n", summary.worstMs);
        fclose(summaryFile);
    }

    log.frames.clear();
    log.loggedFrames = 0;
}

} // namespace

void fpsCounterOnFrame(bool loadingFrame)
{
    FrameLogState& log = state();

    if (settings.scene3d.frame_log) {
        if (!log.open) {
            const std::filesystem::path path = dev_output::outputFile("frame_log.csv");
            log.stream = compat_fopen(path.string().c_str(), "w");
            if (log.stream != nullptr) {
                fprintf(log.stream, "frame,ms,load\n");
            }
            // A fresh interval: the row before this one belongs to a pause
            // (the log was off, or the game just started), not to a frame.
            log.lastCounter = SDL_GetPerformanceCounter();
            log.open = true;
        }
        if (log.stream != nullptr) {
            const Uint64 now = SDL_GetPerformanceCounter();
            const double milliseconds = static_cast<double>(now - log.lastCounter) * 1000.0
                / static_cast<double>(SDL_GetPerformanceFrequency());
            log.lastCounter = now;
            fprintf(log.stream, "%u,%.3f,%d\n", log.loggedFrames, milliseconds, loadingFrame ? 1 : 0);
            log.frames.push_back({milliseconds, loadingFrame});
            log.loggedFrames++;
            if (log.loggedFrames % 100 == 0) {
                fflush(log.stream);
            }
        }
    } else if (log.open) {
        closeLog();
    }
}

void fpsCounterFlushLog()
{
    closeLog();
}

} // namespace fallout
