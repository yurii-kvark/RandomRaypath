#pragma once
#include "glm/fwd.hpp"
#include <chrono>

namespace ray::graphics {
static glm::u64 now_ticks_ns() {
        return (glm::u64)std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count();
}
};