#pragma once
#include "config/config.h"
#include "graphics/window/window.h"
#include "network/remote_control/remote_control.h"
#include "utils/ray_error.h"


namespace ray::graphics {
class window;
class pipeline_manager;
};

namespace ray::logical {

using namespace ray::graphics;

class i_logical_scene {
public:
        void setup_config(config::render_server_config in_server_config) {
                server_config = std::move(in_server_config);
        }

        virtual ray_error init(window& win, pipeline_manager& pipe) = 0;
        virtual bool tick(window& win, pipeline_manager& pipe) = 0;
        virtual void cleanup(window& win, pipeline_manager& pipe) = 0;

        virtual ~i_logical_scene() = default;

        virtual network::remote_answer_frame_set inject_remote_control_pre(window& win, renderer& rend, const network::remote_command_frame_set& this_frame_command) = 0;
        virtual network::remote_answer_frame_set inject_remote_control_post(window& win, renderer& rend, const network::remote_answer_frame_set& answer_set_pre, const network::remote_command_frame_set& this_frame_command) = 0;

protected:
        config::render_server_config server_config;
};

};