module;
#include "src/graphics/graphic_libs.h"
#include <print>
module ray.graphics.rhi;

using namespace ray;
using namespace ray::graphics;


renderer::renderer(GLFWwindow* basis_win) {
        instance_ptr = new VkInstance(VK_NULL_HANDLE);
        surface_ptr = new VkSurfaceKHR(VK_NULL_HANDLE);

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

        if (vkCreateInstance(&ci, nullptr, instance_ptr) != VK_SUCCESS) {
                std::println("vkCreateInstance failed");
                return;
        }

        volkLoadInstance(*instance_ptr);

        if (glfwCreateWindowSurface(*instance_ptr, basis_win, nullptr, &surface_ptr) != VK_SUCCESS) {
                std::println("glfwCreateWindowSurface failed");
                return;
        }
}


renderer::~renderer() {
        if (*instance_ptr != VK_NULL_HANDLE) {
                if (*surface_ptr != VK_NULL_HANDLE) {
                        vkDestroySurfaceKHR(*instance_ptr, *surface_ptr, nullptr);
                }
                vkDestroyInstance(*instance_ptr, nullptr);
        }

        delete surface_ptr;
        delete instance_ptr;
}