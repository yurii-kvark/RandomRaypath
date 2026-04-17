#include "logical_crate_sim.h"

#include "graphics/rhi/pipeline/pipeline_manager.h"
#include "graphics/window/window.h"


#include <optional>

using namespace ray;
using namespace ray::logical;
using namespace ray::graphics;


namespace {
        // Physics tuning parameters
        constexpr glm::f32 k_move_dragging_coef = 4.0f;   // velocity damping per second
        constexpr glm::f32 k_hit_resilience    = 0.6f;    // restitution on collision
        constexpr glm::f32 k_drag_force_coef   = 800.0f;  // force per distance unit when dragging
}


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


void logical_crate_sim::tick(glm::f64 delta_sec, glm::vec4 new_cam_transform, window& win, pipeline_manager& pipe) {
        RAY_PROFILE_SCOPE("logical_crate_sim::tick", glm::vec3(0., 0., 1.));

        glm::vec2 viewport_px = (glm::vec2)pipe.get_target_resolution();
        glm::vec2 screen_mouse_pos = win.get_mouse_position() - viewport_px * 0.5f;
        glm::vec2 world_mouse = (screen_mouse_pos / new_cam_transform.z) + glm::vec2 {new_cam_transform.x, new_cam_transform.y};

        // --- Drag interaction (force-based) ---
        if (focus_index.has_value()) {
                const bool was_crate_captured = crate_captured;
                crate_captured = win.get_mouse_button_left();

                crate_entry& entry = crate_sim_objs[*focus_index];

                if (crate_captured) {
                        if (was_crate_captured != crate_captured) {
                                init_mouse_pos = world_mouse;
                                init_crate_pos = glm::vec2(entry.phys_transform.x, entry.phys_transform.y);
                        }

                        // Force proportional to distance between crate center and cursor.
                        const glm::vec2 crate_center = entry.get_center();
                        const glm::vec2 force = (world_mouse - crate_center) * k_drag_force_coef;
                        entry.velocity += force * (float)delta_sec;
                }
        } else {
                crate_captured = false;
        }

        // --- Physics integration (damping + position update) for ALL crates ---
        {
                const glm::f32 damping = glm::max(0.0f, 1.0f - k_move_dragging_coef * (float)delta_sec);
                for (crate_entry& c : crate_sim_objs) {
                        c.velocity *= damping;
                        c.phys_transform.x += c.velocity.x * delta_sec;
                        c.phys_transform.y += c.velocity.y * delta_sec;
                }
        }

        // --- AABB collision detection & resolution (all pairs, no rotation) ---
        {
                const glm::u32 n = (glm::u32)crate_sim_objs.size();
                for (glm::u32 i = 0; i + 1 < n; ++i) {
                        crate_entry& a = crate_sim_objs[i];
                        for (glm::u32 j = i + 1; j < n; ++j) {
                                crate_entry& b = crate_sim_objs[j];

                                // Compute overlap along each axis.
                                const glm::vec2 a_half = glm::vec2(a.phys_transform.z, a.phys_transform.w) * 0.5f;
                                const glm::vec2 b_half = glm::vec2(b.phys_transform.z, b.phys_transform.w) * 0.5f;
                                const glm::vec2 a_c = a.get_center();
                                const glm::vec2 b_c = b.get_center();
                                const glm::vec2 delta = b_c - a_c;
                                const glm::vec2 overlap = (a_half + b_half) - glm::abs(delta);

                                if (overlap.x <= 0.0f || overlap.y <= 0.0f) {
                                        continue; // no collision
                                }

                                // Resolve along the minimum-overlap axis.
                                glm::vec2 normal {}; // points from a -> b
                                glm::f32  pen    = 0.0f;
                                if (overlap.x < overlap.y) {
                                        normal = glm::vec2(delta.x >= 0.0f ? 1.0f : -1.0f, 0.0f);
                                        pen    = overlap.x;
                                } else {
                                        normal = glm::vec2(0.0f, delta.y >= 0.0f ? 1.0f : -1.0f);
                                        pen    = overlap.y;
                                }

                                // Positional correction: push each crate half the penetration.
                                const glm::vec2 correction = normal * (pen * 0.5f);
                                a.phys_transform.x -= correction.x;
                                a.phys_transform.y -= correction.y;
                                b.phys_transform.x += correction.x;
                                b.phys_transform.y += correction.y;

                                // Impulse: reflect relative velocity along the normal with restitution.
                                const glm::vec2 v_rel = b.velocity - a.velocity;
                                const glm::f32  v_rel_n = glm::dot(v_rel, normal);
                                if (v_rel_n < 0.0f) {
                                        // Equal mass assumption: j = -(1+e) * v_rel_n / 2
                                        const glm::f32 j = -(1.0f + k_hit_resilience) * v_rel_n * 0.5f;
                                        const glm::vec2 impulse = normal * j;
                                        a.velocity -= impulse;
                                        b.velocity += impulse;
                                }
                        }
                }
        }

        // --- Hover/focus detection (only when not currently dragging) ---
        if (!crate_captured) {
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
                        }
                }
        }

        // --- Update graphics transforms/colors for ALL crates every frame ---
        for (glm::u32 i = 0; i < crate_sim_objs.size(); ++i) {
                crate_entry& entry = crate_sim_objs[i];

                glm::vec4 bg_color     = entry.original_back_color;
                glm::vec4 outline_col  = ray_colors::solid(entry.original_back_color * 0.1f);

                const bool is_focused = focus_index.has_value() && *focus_index == i;
                if (is_focused) {
                        if (crate_captured) {
                                bg_color    = entry.original_back_color * 1.5f;
                                outline_col = entry.original_back_color * 0.4f;
                        } else {
                                bg_color    = entry.original_back_color * 1.5f;
                                outline_col = ray_colors::solid(entry.original_back_color * 0.1f);
                        }
                }

                if (auto rect_data = entry.rect_background.access_draw_obj_data()) {
                        rect_data->color     = bg_color;
                        rect_data->transform = entry.get_background_transform();
                }

                if (auto rect_data = entry.rect_outline.access_draw_obj_data()) {
                        rect_data->color     = outline_col;
                        rect_data->transform = entry.phys_transform;
                }

                if (entry.text_line) {
                        entry.text_line->update_transform(entry.get_text_transform());
                }
        }
}


std::vector<pipeline_handle<object_2d_pipeline<>>> logical_crate_sim::get_pipelines(){
        return {
                rect_pipeline_handle,
                text_line_manager.get_pipeline() };
}


void logical_crate_sim::add_crate(glm::vec2 init_pos, glm::f32 size_px, char name_glyph, glm::vec4 background_color, glm::vec4 glyph_color) {
        add_crate(init_pos, glm::vec2(size_px, size_px), name_glyph, background_color, glyph_color);
}


void logical_crate_sim::add_crate(glm::vec2 init_pos, glm::vec2 size_px, char name_glyph, glm::vec4 background_color, glm::vec4 glyph_color) {
        crate_entry entry;

        entry.phys_transform = glm::vec4(init_pos.x, init_pos.y, size_px.x, size_px.y);
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

        const char glyph_cstr[] = { name_glyph, '\0' };

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
