#pragma once


namespace ray::graphics {
#if RAY_GRAPHICS_ENABLE

class scene_logic {
        scene_logic();
        bool tick(window& win, renderer& rend);
        ~scene_logic();
};

#endif
};