#pragma once
#include "glm/fwd.hpp"
#include "glm/vec4.hpp"
#include "utils/ray_error.h"

#include <queue>
#include <stop_token>

namespace ray::network {

enum class remote_command_type {
        none = 0,
        pass_ticks_after = 0, // tick_amount excute after command
        set_camera_position = 1, // x, y, zoom, in pixel, world
        set_mouse_position = 2, // in pixel, world
        set_mouse_left_button = 3, // bool
        set_mouse_right_button = 4, // bool
        add_mouse_scroll = 5, // scalar
        screenshot = 6, // screenshot of the last available frame, not frame of command
        hud_info = 7,
        debug_command = 8,
        count
};

struct remote_command_entry {
        bool enabled = false;
        glm::vec4 value;
};

struct remote_command_frame_set {
        glm::u32 net_session_tag = 0;
        glm::u32 net_id = 0; // id of command frame

        std::array<remote_command_entry, (int)remote_command_type::count> command_map;
};


struct remote_answer_entry {
        bool enabled = false;
        std::string verbal_message;
};

struct remote_answer_frame_set {
        glm::u32 net_session_tag = 0;
        glm::u32 net_id = 0;
        std::array<remote_answer_entry, (int)remote_command_type::count> answer_map;
};


class remote_control_client {
public:
        void async_launch(std::string_view server_full_addr);
        void wait_shutdown();

        std::optional<remote_command_frame_set> receive_next_frame_command();
        void send_answer(const remote_answer_frame_set& command_answer);
};

};