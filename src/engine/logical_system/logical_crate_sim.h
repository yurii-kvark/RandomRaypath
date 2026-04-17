#pragma once
#include "logical_text_line.h"
#include "graphics/rhi/pipeline/object_2d_pipeline.h"
#include "graphics/rhi/pipeline/impl/solid_rect_pipeline.h"
#include "utils/ray_error.h"

#include <vector>

namespace ray::graphics {
class window;
class pipeline_manager;
};

namespace ray::logical {

using namespace graphics;

class logical_crate_sim {
public:
        ray_error init(window& win, pipeline_manager& pipe);
        void tick(glm::f64 delta_second, glm::vec4 new_cam_transform, window& win, pipeline_manager& pipe);
        void destroy(pipeline_manager& pipe);

        std::vector<graphics::pipeline_handle<graphics::object_2d_pipeline<>>> get_pipelines();

        void add_crate(glm::vec2 init_pos, glm::f32 size_px, char name_glyph, glm::vec4 background_color, glm::vec4 glyph_color);
        void add_crate(glm::vec2 init_pos, glm::vec2 size_px, char name_glyph, glm::vec4 background_color, glm::vec4 glyph_color);

protected:
        struct crate_entry {
                logical_text_line_handler text_line;
                graphics::draw_obj_handle<graphics::solid_rect_pipeline> rect_background;
                graphics::draw_obj_handle<graphics::solid_rect_pipeline> rect_outline;

                glm::vec4 phys_transform {}; // top_right, x (->), y (v), pos_xy_px, scale_xy_px
                glm::vec4 original_back_color {};
                glm::vec2 velocity {};

                glm::vec2 get_center() const {
                        return glm::vec2(phys_transform.x + phys_transform.z * 0.5f, phys_transform.y + phys_transform.w * 0.5f);
                }

                glm::vec4 get_background_transform() const {
                        const float outline_size_px = 2;
                        return glm::vec4(phys_transform.x + outline_size_px, phys_transform.y + outline_size_px, phys_transform.z - outline_size_px * 2, phys_transform.w - outline_size_px * 2);
                }
                glm::vec4 get_text_transform() const {
                        const glm::f32 text_size_px = get_text_size();
                        const glm::vec2 text_pos = {phys_transform.x + text_size_px / 4 , phys_transform.y };
                        return glm::vec4(text_pos.x, text_pos.y, 0, text_size_px);
                }
                float get_outline_size() const {
                        return get_text_size() / 20.f;
                }
                float get_text_size() const {
                        return phys_transform.w * 0.8;
                }

                bool is_point_inside(glm::vec2 point) const {
                        return point.x >= phys_transform.x && point.x < phys_transform.x + phys_transform.z && point.y >= phys_transform.y && point.y < phys_transform.y + phys_transform.w;
                }
        };

private:
        logical_text_line_manager text_line_manager {};
        graphics::pipeline_handle<graphics::solid_rect_pipeline> rect_pipeline_handle {};

        std::vector<crate_entry> crate_sim_objs {};
        std::optional<glm::u32> focus_index = std::nullopt;
        bool crate_captured = false;
        glm::vec2 init_mouse_pos;
        glm::vec2 init_crate_pos;
};
};
