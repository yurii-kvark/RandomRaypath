#pragma once
#include <optional>
#include <string>

namespace ray {
        using ray_error = std::optional<std::string>; // if value exist, then error exist in string
}