module;
#include "src/graphics/graphic_libs.h"

#include <print>
#include <vector>
module ray.graphics.rhi;

using namespace ray;
using namespace ray::graphics;


renderer::renderer(GLFWwindow* basis_win) {
        if (basis_win == nullptr) {
                std::println("vkCreateInstance failed. basis_win == nullptr.");
                return;
        }

        if (volkInitialize() != VK_SUCCESS) {
                std::println("No Vulkan runtime (vulkan-1.dll/libvulkan.so missing)");
                return;
        }

        uint32_t extCount = 0;
        const char** exts = glfwGetRequiredInstanceExtensions(&extCount);
        std::vector<const char*> extensions(exts, exts + extCount);

        VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo ci{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        ci.pApplicationInfo = &appInfo;
        ci.enabledExtensionCount = (uint32_t)extensions.size();
        ci.ppEnabledExtensionNames = extensions.data();

        if (vkCreateInstance(&ci, nullptr, &instance) != VK_SUCCESS) {
                std::println("vkCreateInstance failed");
                return;
        }

        volkLoadInstance(instance);

        if (glfwCreateWindowSurface(instance, basis_win, nullptr, &surface) != VK_SUCCESS) {
                std::println("glfwCreateWindowSurface failed");
                return;
        }
}


renderer::~renderer() {
        if (instance != VK_NULL_HANDLE) {
                if (surface != VK_NULL_HANDLE) {
                        vkDestroySurfaceKHR(instance, surface, nullptr);
                }
                vkDestroyInstance(instance, nullptr);
        }
}