#ifndef FALLOUT_MARK_REPEAT_GUARD_H
#define FALLOUT_MARK_REPEAT_GUARD_H

#include <array>
#include <cstddef>

namespace fallout::owner_shot {
template <std::size_t N> class MarkRepeatGuard {
public:
    static constexpr int Vetoed = -2;
    MarkRepeatGuard() { mapped_.fill(-1); }
    void begin(const unsigned char* keys) {
        latched_.fill(false);
        for (std::size_t i = 0; i < N; ++i) held_[i] = keys[i] != 0;
        for (std::size_t i = 0; i < N; ++i) {
            if (mapped_[i] == Vetoed || (valid(mapped_[i]) && keys[mapped_[i]] != 0)) latch(i, mapped_[i]);
        }
    }
    void press(int raw, int effective, bool active) {
        if (!valid(raw) || (!valid(effective) && effective != Vetoed)) return;
        mapped_[raw] = effective;
        if (active) latch(raw, effective);
    }
    int release(int raw) {
        if (!valid(raw)) return -1;
        const int effective = latched_[raw] ? mapped_[raw] : -1;
        held_[raw] = false;
        if (valid(effective)) held_[effective] = false;
        mapped_[raw] = -1;
        latched_[raw] = false;
        for (std::size_t i = 0; i < N; ++i) if (latched_[i] && valid(mapped_[i])) {
            held_[i] = true;
            held_[mapped_[i]] = true;
        }
        return valid(effective) ? effective : -1;
    }
    bool blocksNativeRepeat(int raw) const {
        return valid(raw) && mapped_[raw] != -1 && mapped_[raw] != raw;
    }
    bool blocks(int key, bool active) const {
        return active || (key >= 0 && static_cast<std::size_t>(key) < N && held_[key]);
    }
private:
    bool valid(int key) const { return key >= 0 && static_cast<std::size_t>(key) < N; }
    void latch(int raw, int effective) { latched_[raw] = true; held_[raw] = true; if (valid(effective)) held_[effective] = true; }
    std::array<bool, N> held_ {};
    std::array<bool, N> latched_ {};
    std::array<int, N> mapped_ {};
};
} // namespace fallout::owner_shot
#endif
