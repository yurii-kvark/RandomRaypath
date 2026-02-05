#pragma once
#include "config/client_renderer.h"

#include <thread>


namespace ray::graphics {

#if RAY_GRAPHICS_ENABLE
class async_graphical_loop {
public:
        async_graphical_loop(config::client_renderer in_config);
        ~async_graphical_loop();

        bool is_alive() const;

        void signal_terminate();
        void wait_blocking();

        async_graphical_loop(const async_graphical_loop&) = delete;
        async_graphical_loop& operator=(const async_graphical_loop&) = delete;

        async_graphical_loop(async_graphical_loop&&) noexcept = default;
        async_graphical_loop& operator=(async_graphical_loop&&) noexcept = default;

private:
        std::jthread worker_t;
};
#endif
};

