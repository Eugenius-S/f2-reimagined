#ifndef FALLOUT_MARK_REPEAT_GUARD_H
#define FALLOUT_MARK_REPEAT_GUARD_H

#include <array>
#include <cstddef>

namespace fallout::owner_shot {
template <std::size_t N> class MarkRepeatGuard {
public:
    void begin(const unsigned char* keys) { for (std::size_t i = 0; i < N; ++i) held_[i] = keys[i] != 0; }
    void press(int key) { if (key >= 0 && static_cast<std::size_t>(key) < N) held_[key] = true; }
    void release(int key) { if (key >= 0 && static_cast<std::size_t>(key) < N) held_[key] = false; }
    bool blocks(int key, bool active) const {
        return active || (key >= 0 && static_cast<std::size_t>(key) < N && held_[key]);
    }
private:
    std::array<bool, N> held_ {};
};
} // namespace fallout::owner_shot
#endif
