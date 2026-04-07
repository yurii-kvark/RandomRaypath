#pragma once
#include "config/client_renderer.h"

#include <thread>


namespace ray::logical {

class async_logical_loop {
public:
        async_logical_loop(config::client_renderer in_config);
        ~async_logical_loop();

        bool is_alive() const;

        void signal_terminate();
        void wait_blocking();

        async_logical_loop(const async_logical_loop&) = delete;
        async_logical_loop& operator=(const async_logical_loop&) = delete;

        async_logical_loop(async_logical_loop&&) noexcept = default;
        async_logical_loop& operator=(async_logical_loop&&) noexcept = default;

private:
        std::jthread worker_t;
};

};

