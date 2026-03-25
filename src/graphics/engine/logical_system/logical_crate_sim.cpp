#include "logical_crate_sim.h"

#include "graphics/rhi/pipeline/pipeline_manager.h"
#include "graphics/window/window.h"


#include <optional>

#if RAY_GRAPHICS_ENABLE

using namespace ray;
using namespace ray::graphics;


ray_error logical_crate_sim::init(window& win, pipeline_manager& pipe){
        rect_pipeline_handle = pipe.create_pipeline<solid_rect_pipeline>(ray_pipeline_order::world_obj);

        if (!rect_pipeline_handle.is_valid()) {
                return "Cannot create solid_rect_pipeline";
        }

        ray_error manager_error = text_line_manager.init(pipe, ray_pipeline_order::world_obj + 1);
        if (manager_error.has_value()) {
                return manager_error;
        }

        return {};
}


void logical_crate_sim::tick(glm::vec4 new_cam_transform, window& win, pipeline_manager& pipe) {
        RAY_PROFILE_SCOPE("logical_crate_sim::tick", glm::vec3(0., 0., 1.));

        glm::vec2 viewport_px = (glm::vec2)pipe.get_target_resolution();
        glm::vec2 screen_mouse_pos = win.get_mouse_position() - viewport_px * 0.5f;
        glm::vec2 world_mouse = (screen_mouse_pos / new_cam_transform.z) + glm::vec2 {new_cam_transform.x, new_cam_transform.y};

        if (focus_index.has_value()) { // drag
                const bool was_crate_captured = crate_captured;
                crate_captured = win.get_mouse_button_left();

                crate_entry& entry = crate_sim_objs[*focus_index];

                if (crate_captured) {
                        if (was_crate_captured != crate_captured) {
                                init_mouse_pos = world_mouse;
                                init_crate_pos = glm::vec2(entry.phys_transform.x, entry.phys_transform.y);
                        }

                        glm::vec2 delta_drag = world_mouse - init_mouse_pos;
                        entry.phys_transform.x = init_crate_pos.x + delta_drag.x;
                        entry.phys_transform.y = init_crate_pos.y + delta_drag.y;

                        if (auto rect_data = entry.rect_background.access_draw_obj_data()) {
                                rect_data->color = entry.original_back_color * 1.5f;
                                rect_data->transform = entry.get_background_transform();
                        }

                        if (auto rect_data = entry.rect_outline.access_draw_obj_data()) {
                                rect_data->color = entry.original_back_color * 0.4f;
                                rect_data->transform = entry.phys_transform;
                        }

                        if (entry.text_line) {
                                entry.text_line->update_transform(entry.get_text_transform());
                        }
                } else {
                        if (auto rect_data = entry.rect_background.access_draw_obj_data()) {
                                rect_data->color = entry.original_back_color * 2.1f;
                                rect_data->transform = entry.get_background_transform();
                        }

                        if (auto rect_data = entry.rect_outline.access_draw_obj_data()) {
                                rect_data->color = ray_colors::solid(entry.original_back_color * 1.0f);
                                rect_data->transform = entry.phys_transform;
                        }
                }
        }

        if (!crate_captured) { // hover
                std::optional<glm::u32> new_focus_index {};

                for (glm::u32 i = 0; i < crate_sim_objs.size(); ++i) {
                        crate_entry& i_crate = crate_sim_objs[i];
                        if (i_crate.is_point_inside(world_mouse)) {
                                new_focus_index = i;
                                break;
                        }
                }

                const bool diff_focus =
                        (new_focus_index.has_value() != focus_index.has_value())
                        || (new_focus_index.has_value() && focus_index.has_value() && new_focus_index.value() != focus_index.value());

                if (diff_focus) {
                        if (focus_index.has_value()) {
                                if (auto rect_data = crate_sim_objs[*focus_index].rect_background.access_draw_obj_data()) {
                                        rect_data->color = crate_sim_objs[*focus_index].original_back_color;
                                }

                                if (auto rect_data = crate_sim_objs[*focus_index].rect_outline.access_draw_obj_data()) {
                                        rect_data->color = ray_colors::solid(crate_sim_objs[*focus_index].original_back_color * 0.1f);
                                }
                        }

                        focus_index = {};

                        if (new_focus_index.has_value()) {
                                focus_index = *new_focus_index;

                                if (auto rect_data = crate_sim_objs[*focus_index].rect_background.access_draw_obj_data()) {
                                        rect_data->color = crate_sim_objs[*focus_index].original_back_color * 1.5f;
                                }

                                if (auto rect_data = crate_sim_objs[*focus_index].rect_outline.access_draw_obj_data()) {
                                        rect_data->color = ray_colors::solid(crate_sim_objs[*focus_index].original_back_color * 0.1f);
                                }
                        }
                }
        }
}


std::vector<pipeline_handle<object_2d_pipeline<>>> logical_crate_sim::get_pipelines(){
        return {
                rect_pipeline_handle,
                text_line_manager.get_pipeline() };
}


void logical_crate_sim::add_crate(glm::vec2 init_pos, glm::f32 size_px, char name_glyph, glm::vec4 background_color, glm::vec4 glyph_color) {
        crate_entry entry;

        entry.phys_transform = glm::vec4(init_pos.x, init_pos.y, size_px, size_px);
        entry.original_back_color = ray_colors::solid(background_color * 0.4f);
        entry.rect_background = rect_pipeline_handle.create_draw_obj();
        if (auto rect_data = entry.rect_background.access_draw_obj_data()) {
                rect_data->space_basis = e_space_type::world;
                rect_data->z_order = crate_sim_objs.size() + 2;
                rect_data->transform = entry.get_background_transform();
                rect_data->color = entry.original_back_color;
        }

        entry.rect_outline = rect_pipeline_handle.create_draw_obj();
        if (auto rect_data = entry.rect_outline.access_draw_obj_data()) {
                rect_data->space_basis = e_space_type::world;
                rect_data->z_order = crate_sim_objs.size() + 1;
                rect_data->transform = entry.phys_transform;
                rect_data->color = ray_colors::solid(entry.original_back_color * 0.1f);
        }

        const char glyph_cstr[] = { name_glyph };

        const logical_text_line_args text_args = logical_text_line_args {
                .content_text = glyph_cstr,
                .static_capacity = 1,
                .space_basis = e_space_type::world,
                .transform = entry.get_text_transform(),
                .z_order = (glm::u32)crate_sim_objs.size() + 1,
                .text_color = ray_colors::solid(glyph_color),
                .outline_size_px = entry.get_outline_size(),
                .outline_color = ray_colors::solid(ray_colors::black),
                .background_color = ray_colors::transparent,
        };

        entry.text_line = text_line_manager.create_text_line(text_args);

        crate_sim_objs.emplace_back(std::move(entry));
}

void logical_crate_sim::destroy(pipeline_manager& pipe) {
        pipe.destroy_pipeline(rect_pipeline_handle);
        text_line_manager.destroy(pipe);
}

#endif