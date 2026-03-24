#pragma once

#include <string_view>
#include <glm/vec3.hpp>



#if RAY_CPU_PROFILE == 1 && TRACY_ENABLE == 1
    #include <tracy/Tracy.hpp>
#endif


namespace ray {
#if RAY_CPU_PROFILE == 1 && TRACY_ENABLE == 1
        constexpr uint32_t ray_profile_color(float r, float g, float b) {
             return (uint32_t(r * 255.f) << 16)
                  | (uint32_t(g * 255.f) << 8)
                  |  uint32_t(b * 255.f);
        }

        constexpr uint32_t ray_profile_color(glm::vec3 c) {
               return ray_profile_color(c.x, c.y, c.z);
        }

        constexpr uint32_t ray_profile_color(glm::vec4 c) {
                return ray_profile_color(c.x, c.y, c.z);
        }

        class ray_profile_scope {
        public:
                ray_profile_scope(const tracy::SourceLocationData* loc, std::string_view dynamic_name = {})
                    : m_zone(loc, TRACY_CALLSTACK, true)
                {
                        if (!dynamic_name.empty())
                                m_zone.Name(dynamic_name.data(), (uint16_t)dynamic_name.size());
                }

                void text(std::string_view t) {
                        m_zone.Text(t.data(), t.size());
                }

                void value(uint64_t v) {
                        m_zone.Value(v);
                }

        private:
                tracy::ScopedZone m_zone;
        };

#define RAY_PROFILE_SCOPE(name, color) \
        static constexpr tracy::SourceLocationData \
                RAY_CONCAT(_ray_loc_, __LINE__) { \
                        nullptr, __FUNCTION__, __FILE__, \
                        (uint32_t)__LINE__, ray_profile_color(color) }; \
        ray_profile_scope RAY_CONCAT(_ray_scope_, __LINE__) \
                (&RAY_CONCAT(_ray_loc_, __LINE__), name)

#define RAY_PROFILE_FRAME() FrameMark

#define RAY_CONCAT_IMPL(a, b) a##b
#define RAY_CONCAT(a, b) RAY_CONCAT_IMPL(a, b)

#else

#define RAY_PROFILE_SCOPE(name, color)    (void)0
#define RAY_PROFILE_FRAME()               (void)0

#endif
};
