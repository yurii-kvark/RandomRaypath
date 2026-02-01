/*
#include <filesystem>
#include <print>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

import ray.config;

int main() {
        //auto config1 = ray::config::client_renderer::load(std::filesystem::path {"../config/client_renderer.toml"});
       // auto config2 = ray::config::client_renderer::load(std::filesystem::path {"../config/client_renderer2.toml"});
       // auto config3 = ray::config::client_renderer::load(std::filesystem::path {"../config/client_renderer3.toml"});

       // std::println("Config 1: \n{}\n---------", config1);
       // std::println("Config 2: \n{}\n---------", config2);
       // std::println("Config 3: \n{}\n---------", config3);

        std::println("Hello!");

        return 0;
}*/

#define VK_NO_PROTOTYPES
#include <volk.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <vector>
#include <iostream>

int main() {
        if (!glfwInit()) return 1;
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* w = glfwCreateWindow(1280, 720, "vk", nullptr, nullptr);

        if (volkInitialize() != VK_SUCCESS) {
                std::cout << "No Vulkan runtime (vulkan-1.dll/libvulkan.so missing)\n";
                return 2;
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

        VkInstance instance = VK_NULL_HANDLE;
        if (vkCreateInstance(&ci, nullptr, &instance) != VK_SUCCESS) {
                std::cout << "vkCreateInstance failed\n";
                return 3;
        }

        volkLoadInstance(instance); // <-- теперь instance-функции гарантированно загружены

        VkSurfaceKHR surface = VK_NULL_HANDLE;
        if (glfwCreateWindowSurface(instance, w, nullptr, &surface) != VK_SUCCESS) {
                std::cout << "glfwCreateWindowSurface failed\n";
                return 4;
        }

        std::cout << "Vulkan instance + surface OK\n";

        while (!glfwWindowShouldClose(w)) glfwPollEvents();

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(w);
        glfwTerminate();
        return 0;
}