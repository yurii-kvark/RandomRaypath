
#include "base_scene.h"

#include "utils/ray_log.h"

using namespace ray;
using namespace ray::graphics;
using namespace ray::logical;

namespace {
using command_type = network::remote_command_type;

constexpr glm::u32 command_index(command_type t) {
        return static_cast<glm::u32>(t);
}
} // namespace


network::remote_answer_frame_set base_scene::inject_remote_control_pre(
        window& win,
        renderer& rend,
        const network::remote_command_frame_set& this_frame_command) {

        network::remote_answer_frame_set answer_set;
        answer_set.net_id = this_frame_command.net_id;
        answer_set.net_session_tag = this_frame_command.net_session_tag;

        const auto mark_ok = [&](command_type t, std::string msg) {
                auto& entry = answer_set.answer_map[command_index(t)];
                entry.enabled = true;
                entry.verbal_message = std::move(msg);
        };

        for (glm::u32 i = 0; i < command_index(command_type::count); ++i) {
                const auto& cmd = this_frame_command.command_map[i];
                if (!cmd.enabled) {
                        continue;
                }

                const command_type type = static_cast<command_type>(i);
                switch (type) {
                case command_type::set_camera_position: {
                        world_processor.set_camera_transform(glm::vec4(cmd.value.x, cmd.value.y, cmd.value.z, 0.0f));
                        const glm::vec4 applyed_t = world_processor.get_camera_transform();
                        mark_ok(type, std::format("camera position set: {}:{}:{} :: {}", applyed_t.x, applyed_t.y, applyed_t.z, applyed_t.w));
                        break;
                }
                case command_type::set_mouse_position: {
                        win.inject_mouse_position(glm::vec2(cmd.value.x, cmd.value.y));
                        const glm::vec2 applyed_pos = win.get_mouse_position();
                        mark_ok(type, std::format("mouse position injected: in {}:{} - out {}:{}", cmd.value.x, cmd.value.y, applyed_pos.x, applyed_pos.y));
                        break;
                }
                case command_type::set_mouse_left_button: {
                        const bool input_left = (int)std::round(cmd.value.x) != 0;
                        win.inject_mouse_button_left(input_left);
                        const bool output_left = win.get_mouse_button_left();
                        mark_ok(type, std::format("left mouse button injected: in {}, out {}", input_left, output_left));
                        break;
                }
                case command_type::set_mouse_right_button: {
                        const bool input_right = (int)std::round(cmd.value.x) != 0;
                        win.inject_mouse_button_right(input_right);
                        const bool output_right = win.get_mouse_button_left();
                        mark_ok(type, std::format("right mouse button injected: in {}, out {}", input_right, output_right));
                        break;
                }
                case command_type::add_mouse_scroll: {
                        win.inject_mouse_scroll_add(static_cast<glm::f64>(cmd.value.x));
                        mark_ok(type, std::format("mouse scroll added: {}", cmd.value.x));
                        break;
                }
                case command_type::screenshot:
                        rend.request_screenshot();
                        break;
                case command_type::hud_info:

                case command_type::pass_ticks_after: // handles on tick level
                case command_type::count:
                default:
                        break;
                }
        }

        return answer_set;
}


network::remote_answer_frame_set base_scene::inject_remote_control_post(window& win, renderer& rend, const network::remote_answer_frame_set& answer_set_pre, const network::remote_command_frame_set& this_frame_command) {
        network::remote_answer_frame_set answer_set = answer_set_pre;
        answer_set.net_id = this_frame_command.net_id;
        answer_set.net_session_tag = this_frame_command.net_session_tag;

        const auto mark_ok = [&](command_type t, std::string msg) {
                auto& entry = answer_set.answer_map[command_index(t)];
                entry.verbal_message = !entry.enabled ? msg : std::format( "pre: {} post: {}", entry.verbal_message, msg);
                entry.enabled = true;
        };

        for (glm::u32 i = 0; i < command_index(command_type::count); ++i) {
                const auto& cmd = this_frame_command.command_map[i];
                if (!cmd.enabled) {
                        continue;
                }

                const command_type type = static_cast<command_type>(i);
                switch (type) {
                case command_type::set_camera_position:  // process in pre
                case command_type::set_mouse_position:
                case command_type::set_mouse_left_button:
                case command_type::set_mouse_right_button:
                case command_type::add_mouse_scroll:
                        break;
                case command_type::screenshot: {
                        const std::string screenshot_filepath = std::format("../mcp_logs/screenshot/session_{}/net{}_frame{}.png", answer_set.net_session_tag, answer_set.net_id, hud_info.frame_counter);
                        const ray_error screenshot_error = rend.execute_screenshot_save_png(screenshot_filepath);

                        if (screenshot_error.has_value()) {
                                mark_ok(type, std::format("Can't take screenshot, error: {}", *screenshot_error));
                        } else {
                                mark_ok(type, std::format("App took screenshot as {}.", screenshot_filepath));
                        }
                        break;
                }
                case command_type::hud_info: {
                        const std::string last_full_text = hud_info.get_last_full_text();
                        mark_ok(type, std::format("this tick full hud_info: {}", last_full_text));
                        break;
                }
                case command_type::pass_ticks_after: // handles on tick level
                case command_type::count:
                default:
                        break;
                }
        }

        return answer_set;
}


ray_error base_scene::init(window& win, pipeline_manager& pipe) {
        ray_error hud_error = hud_info.init(win, pipe, server_config.style.color_hud_info);
        if (hud_error) {
                return hud_error;
        }

        ray_error grid_error = grid_system.init(win, pipe, server_config.style.color_grid);
        if (grid_error) {
                return grid_error;
        }

        world_processor.register_pipeline(grid_system.get_pipeline());

        return {};
}

bool base_scene::tick(const tick_time_info& tick_time, window& win, pipeline_manager& pipe) {
        world_processor.tick(tick_time, win, pipe);

        const glm::vec4 new_cam_transform = world_processor.get_camera_transform();
        hud_info.update_camera_transform_info(new_cam_transform);

        hud_info.tick(tick_time, win, pipe);
        return true;
}



void base_scene::cleanup(window& win, pipeline_manager& pipe) {
        hud_info.destroy(win, pipe);
        grid_system.destroy(win, pipe);
}

