#pragma once
#include "glm/vec4.hpp"

namespace ray {
#if RAY_GRAPHICS_ENABLE

class ray_colors final {
public:
        static constexpr float alpha_value = 0.5f;

        static constexpr glm::vec4 transparent {0.0f, 0.0f, 0.0f, 0.0f};

        static constexpr glm::vec4 black {0.0f, 0.0f, 0.0f, alpha_value};
        static constexpr glm::vec4 white {1.0f, 1.0f, 1.0f, alpha_value};
        static constexpr glm::vec4 gray {0.5f, 0.5f, 0.5f, alpha_value};

        static constexpr glm::vec4 red {1.0f, 0.0f, 0.0f, alpha_value};
        static constexpr glm::vec4 green {0.0f, 1.0f, 0.0f, alpha_value};
        static constexpr glm::vec4 blue {0.0f, 0.0f, 1.0f, alpha_value};

        static constexpr glm::vec4 yellow {1.0f, 1.0f, 0.0f, alpha_value};
        static constexpr glm::vec4 cyan {0.0f, 1.0f, 1.0f, alpha_value};
        static constexpr glm::vec4 magenta {1.0f, 0.0f, 1.0f, alpha_value};
        static constexpr glm::vec4 orange {1.0f, 0.5f, 0.0f, alpha_value};
        static constexpr glm::vec4 purple {0.5f, 0.0f, 1.0f, alpha_value};
        static constexpr glm::vec4 pink {1.0f, 0.25f, 0.5f, alpha_value};
        static constexpr glm::vec4 brown {0.55f, 0.27f, 0.07f, alpha_value};
        static constexpr glm::vec4 lime {0.50f, 1.00f, 0.00f, alpha_value};
        static constexpr glm::vec4 teal {0.00f, 0.50f, 0.50f, alpha_value};
        static constexpr glm::vec4 navy {0.00f, 0.00f, 0.50f, alpha_value};
        static constexpr glm::vec4 maroon {0.50f, 0.00f, 0.00f, alpha_value};
        static constexpr glm::vec4 olive {0.50f, 0.50f, 0.00f, alpha_value};

        inline static glm::vec4 solid(glm::vec4 in) {
                return alpha(in, 1.);
        }

        inline static glm::vec4 alpha(glm::vec4 in, float alpha) {
                return {in.x, in.y, in.z, alpha};
        }
};

class ray_pipeline_order final {
public:
        static constexpr glm::u32 world_obj = 1000;
        static constexpr glm::u32 grid = 100;
        static constexpr glm::u32 hud_info = 100500;
};

#endif
};