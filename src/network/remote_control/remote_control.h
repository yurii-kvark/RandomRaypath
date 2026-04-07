#pragma once
#include "glm/fwd.hpp"
#include "utils/ray_error.h"

#include <queue>

namespace ray::network {

// enum class command_type {
//         submit_tick = 0, // ticks_amount, by default do nothing, in -tickless mode all previous commands will be executed and perform one tick
//         set_camera_position = 1, // x, y, zoom, in pixel, world
//         set_mouse_position = 2, // in pixel, world
//         set_mouse_left_button = 3, // bool
//         set_mouse_right_button = 4, // bool
//         add_mouse_scroll = 5, // scalar
//         screenshot = 6,
// };
//
// class tick_command_set {
// public:
//         glm::u32 pass_ticks_after = 0;
//
// };
//
// class tick_info_set {
// public:
//
// };
//
// using command_flow = std::queue<remote_command>;
//
// class remote_control_client {
//         void async_launch(std::string_view server_full_addr);
//
//         void receive_commands(command_flow& base_flow);
//         void send_commands(const command_flow& );
// };

};