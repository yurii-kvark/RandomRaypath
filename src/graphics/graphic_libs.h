#pragma once

#ifndef RAY_GRAPHICS_ENABLE
#define RAY_GRAPHICS_ENABLE 0
#endif

// TODO: implement disabled graphics
#if RAY_GRAPHICS_ENABLE

#define VK_NO_PROTOTYPES
#include <volk.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#endif