#include "g_app_driver.h"

#include <cstring>
#include <memory>
#include <print>
#include <set>
#include <vector>

using namespace ray;
using namespace ray::graphics;

#if RAY_GRAPHICS_ENABLE

void g_app_driver::driver_handler::try_init_with_surface(VkSurfaceKHR surface) {
        if (!g_handler) {
                return;
        }

        if (g_handler->device != VK_NULL_HANDLE) {
                return;
        }

        std::scoped_lock lock(g_handler->init_mtx);
        if (g_handler->device == VK_NULL_HANDLE) {
                g_handler->graphic_init_surface(surface);
        }
}

void g_app_driver::driver_handler_deleter::operator()(driver_handler* handler) const {
        if (!handler) {
                return;
        }

        auto* driver = handler->g_handler;
        delete handler;

        if (!driver) {
                return;
        }

        std::scoped_lock lock(driver->init_mtx);

        if (driver->current_drivers_num.fetch_sub(1) == 1) {
                driver->init_done.store(false, std::memory_order_relaxed);
                driver->graphic_destroy();
        }
}


g_app_driver& g_app_driver::thread_safe() {
        static g_app_driver g_driver_obj {};
        return g_driver_obj;
}


std::shared_ptr<g_app_driver::driver_handler> g_app_driver::init_driver_handler() {
        std::scoped_lock lock(init_mtx);
        if (!init_done.load(std::memory_order_relaxed)) {

                static bool static_init_done = false;
                if (!static_init_done) {
                        static_init_done = true;

                        if (volkInitialize() != VK_SUCCESS) {
                                std::println("renderer: no Vulkan runtime (missing vulkan-1.dll / libvulkan.so)");
                        }
                }

                graphic_init();
                init_done.store(true, std::memory_order_relaxed);
        }


        current_drivers_num.fetch_add(1, std::memory_order_relaxed);
        return std::shared_ptr<driver_handler>(new driver_handler{this}, driver_handler_deleter{});
}


g_app_driver::~g_app_driver() {
        assert(current_drivers_num == 0);
}


void g_app_driver::graphic_destroy() {
        if (device != VK_NULL_HANDLE) {
                vkDestroyDevice(device, nullptr);
                device = VK_NULL_HANDLE;
        }

        if (instance != VK_NULL_HANDLE) {
                vkDestroyInstance(instance, nullptr);
                instance = VK_NULL_HANDLE;
        }

        gfx_family = UINT32_MAX;
        present_family = UINT32_MAX;
        physical = VK_NULL_HANDLE;
        gfx_queue = VK_NULL_HANDLE;
        present_queue = VK_NULL_HANDLE;
}


namespace device_helpers {

bool has_extension(VkPhysicalDevice device, const char* extName) {
        uint32_t ext_count = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &ext_count, nullptr);
        std::vector<VkExtensionProperties> exts(ext_count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &ext_count, exts.data());
        for (auto& e : exts) {
                if (std::strcmp(e.extensionName, extName) == 0) {
                        return true;
                }
        }
        return false;
}


glm::u64 device_local_bytes(VkPhysicalDevice device) {
        VkPhysicalDeviceMemoryProperties mp{};
        vkGetPhysicalDeviceMemoryProperties(device, &mp);

        glm::u64 sum = 0;
        for (glm::u32 i = 0; i < mp.memoryHeapCount; ++i) {
                if (mp.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                        sum += mp.memoryHeaps[i].size;
                }
        }
        return sum;
}

std::string device_name(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(device, &props);
        return props.deviceName;
}


bool swapchain_ok(VkPhysicalDevice device, VkSurfaceKHR surface) {
        glm::u32 format_count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
        glm::u32 mode_count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &mode_count, nullptr);
        return (format_count > 0) && (mode_count > 0);
}


bool device_type_ok(VkPhysicalDevice device, bool& out_is_discrete) {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(device, &props);

        out_is_discrete = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        const bool is_integrated = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;

        return out_is_discrete || is_integrated;
}


bool find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface,
                         glm::u32& out_gfx, glm::u32& out_present) {
        glm::u32 q_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &q_count, nullptr);
        std::vector<VkQueueFamilyProperties> q_props(q_count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &q_count, q_props.data());

        out_gfx = UINT32_MAX;
        out_present = UINT32_MAX;

        for (glm::u32 i = 0; i < q_count; ++i) {
                if (q_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                        out_gfx = i;
                }

                VkBool32 present = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present);
                if (present) {
                        out_present = i;
                }

                if (out_gfx != UINT32_MAX && out_present != UINT32_MAX) {
                        break;
                }
        }

        return out_gfx != UINT32_MAX && out_present != UINT32_MAX;
}
};



void g_app_driver::graphic_init() {
        uint32_t ext_count = 0;
        const char** exts = glfwGetRequiredInstanceExtensions(&ext_count);
        std::vector<const char*> extensions(exts, exts + ext_count);

        VkApplicationInfo app_info{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
        app_info.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo ci{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        ci.pApplicationInfo = &app_info;
        ci.enabledExtensionCount = (uint32_t)extensions.size();
        ci.ppEnabledExtensionNames = extensions.data();

        if (vkCreateInstance(&ci, nullptr, &instance) != VK_SUCCESS) {
                std::println("renderer: vkCreateInstance failed");
                return;
        }

        volkLoadInstance(instance);
}


void g_app_driver::graphic_init_surface(VkSurfaceKHR surface) {
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
        if (device_count == 0) {
                std::println("renderer: no physical devices found");
                return;
        }

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

        glm::u32 best_i = UINT32_MAX;
        glm::u32 best_gfx_f = UINT32_MAX;
        glm::u32 best_present_f = UINT32_MAX;
        glm::u64 best_v_ram = 0;
        std::string best_device_name;

        for (glm::u32 i = 0; i < device_count; ++i) {
                auto& device_el = devices[i];

                std::string name = device_helpers::device_name(device_el);
                std::println("renderer: detected GPU: {}", name);

                if (!device_helpers::has_extension(device_el, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
                        continue;
                }

                glm::u32 gfx = UINT32_MAX;
                glm::u32 present = UINT32_MAX;
                if (!device_helpers::find_queue_families(device_el, surface, gfx, present)) {
                        continue;
                }

                if (!device_helpers::swapchain_ok(device_el, surface)) {
                        continue;
                }

                bool is_discrete = false;
                if (!device_helpers::device_type_ok(device_el, is_discrete)) {
                        continue;
                }

                glm::u64 memory_volume = device_helpers::device_local_bytes(device_el);

                // Huge bias to discrete
                glm::u64 v_ram = memory_volume + (is_discrete ? (1ull << 42) : 0ull);

                if (best_v_ram < v_ram) {
                        best_v_ram = v_ram;
                        best_gfx_f = gfx;
                        best_present_f = present;
                        best_i = i;
                        best_device_name = name;
                }
        }

        if (best_i != UINT32_MAX) {
                physical = devices[best_i];
                gfx_family = best_gfx_f;
                present_family = best_present_f;
        }

        if (physical == VK_NULL_HANDLE) {
                std::println("renderer: failed to find suitable GPU");
                return;
        }

        std::println("INFO renderer: Seleced GPU device [{}]", best_device_name);

        std::set<glm::u32> require_family_indexes = {
                gfx_family,
                present_family
        };

        float prio = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        for (auto index_family : require_family_indexes) {
                queue_create_infos.push_back ( {
                        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                        .queueFamilyIndex = index_family,
                        .queueCount = 1,
                        .pQueuePriorities = &prio
                });
        };

        VkPhysicalDeviceVulkan13Features f13{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
        f13.dynamicRendering = VK_TRUE;

        const char* dev_exts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        VkDeviceCreateInfo device_create_info{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        device_create_info.pNext = &f13;
        device_create_info.queueCreateInfoCount = (glm::u32)queue_create_infos.size();
        device_create_info.pQueueCreateInfos = queue_create_infos.data();
        device_create_info.enabledExtensionCount = 1;
        device_create_info.ppEnabledExtensionNames = dev_exts;

        if (vkCreateDevice(physical, &device_create_info, nullptr, &device) != VK_SUCCESS) {
                std::println("renderer: vkCreateDevice failed");
                return;
        }

        volkLoadDevice(device);

        vkGetDeviceQueue(device, gfx_family, 0, &gfx_queue);
        vkGetDeviceQueue(device, present_family, 0, &present_queue);
}

#endif
