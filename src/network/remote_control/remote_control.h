#pragma once
#include "glm/fwd.hpp"
#include "glm/vec4.hpp"
#include "utils/ray_error.h"

#include <array>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <thread>

namespace ray::network {

enum class remote_command_type {
        none = 0,
        pass_ticks_after = 1, // [tick_amount] execute after command
        set_camera_position = 2, // [x, y, zoom] in pixel, world
        set_mouse_position = 3, // [x, y] in pixel, world
        set_mouse_left_button = 4, // [x > 0] bool
        set_mouse_right_button = 5, // [x > 0] bool
        add_mouse_scroll = 6, // [scalar]
        screenshot = 7, // [disable_compress > 0], screenshot of the last available frame, not frame of command
        hud_info = 8, // nothing
        debug_command = 9, // [x, y, z, w] args
        shutdown = 10,
        // do_session_log = 11, TODO: duplicates default.log in session_xxx.log
        count // = 10
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
        std::array<remote_answer_entry, (int) remote_command_type::count> answer_map;
};


class remote_control_client {
public:
        void async_launch(std::string_view server_full_addr);
        void wait_shutdown();

        std::optional<remote_command_frame_set> receive_next_frame_command();
        void send_answer(const remote_answer_frame_set& command_answer);

private:
        void network_loop(std::stop_token stop, std::string host, std::string port);

        static remote_command_frame_set command_frame_from_json(const std::string& in_json);
        static std::string answer_frame_to_json(const remote_answer_frame_set& in_answer);
        static const char* remote_command_type_name(remote_command_type index);
        static remote_command_type remote_command_type_index(std::string_view name);

        std::mutex incoming_mtx;
        std::queue<remote_command_frame_set> incoming_queue;

        std::mutex outgoing_mtx;
        std::queue<remote_answer_frame_set> outgoing_queue;

        std::jthread net_thread;
};

};
