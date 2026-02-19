#pragma once
#include "pipeline.h"
#include "graphics/rhi/g_app_driver.h"

#include <cstring>
#include <fstream>
#include <print>

namespace ray::graphics {
enum class e_space_type : glm::u8 {
        screen = 0,
        world
};

#if RAY_GRAPHICS_ENABLE

struct ray_vertex {
        glm::float32 pos[2];
        glm::float32 uv[2];
};

struct object_2d_pipeline_data_model {
        struct pipeline : base_pipeline_data_model::pipeline {
                glm::u32 time_ms = 0;
                glm::vec4 camera_transform = {0,0,1,0}; // x_px, y_px, scale, 1.0
        };

        struct draw_obj : base_pipeline_data_model::draw_obj {
                e_space_type space_basis = e_space_type::screen;
                glm::u32 z_order = 0; // bigger on top. void to touch frequently. e_space_type::screen set significant bit to 1
                glm::vec4 transform = {}; // x_pos, y_pos, x_size, y_size
                glm::vec4 color {};

                glm::u32 get_render_order() const {
                        const glm::u32 space_type_bit = (space_basis == e_space_type::screen)
                                ? glm::u32(1) << (sizeof(glm::u32) * CHAR_BIT - 1)
                                : 0u;
                        return glm::u32(z_order) | space_type_bit;
                }
        };
};

// TODO: texture_pipeline, text_pipeline, raypath_pipeline
template<class PipelineDataModel = object_2d_pipeline_data_model>
class object_2d_pipeline : public base_pipeline<PipelineDataModel> {
public:
        static constexpr glm::u32 k_init_graphic_object_capacity = 64;

        object_2d_pipeline(pipeline_arguments in_args);
        virtual ~object_2d_pipeline() override;

        virtual void draw_commands(VkCommandBuffer in_command_buffer, glm::u32 frame_index) override;
        virtual void update_swapchain(VkFormat in_swapchain_format, glm::uvec2 resolution) override;

protected:
        struct alignas(16) pipe_frame_ubo {
                glm::u32 time_ms = 0;
                glm::u32 _pad0 = 0, _pad1 = 0, _pad2 = 0;
                glm::vec4 camera_transform_ndc = glm::vec4(0,0,1,0);
        };
        static_assert(sizeof(pipe_frame_ubo) % 16 == 0);

        struct pipe_frame_ubo_gpu {
                VkBuffer buffer = VK_NULL_HANDLE;
                VkDeviceMemory memory = VK_NULL_HANDLE;
                void* mapped = nullptr; // pipe_frame_ubo
                bool in_flight_data_valid = false;
        };

        struct alignas(16) draw_obj_ssbo_item {
                glm::vec4 transform_ndc = {}; // x_ndc, y_ndc, w_ndc, h_ndc
                glm::vec4 color = {};
                glm::u32 render_order = 0;
                glm::u32 space_basis = 0;
                glm::u32 _pad0 = 0, _pad1 = 0;
        };
        static_assert(sizeof(draw_obj_ssbo_item) % 16 == 0);

        struct draw_obj_ssbo_gpu {
                VkBuffer buffer = VK_NULL_HANDLE;
                VkDeviceMemory memory = VK_NULL_HANDLE;
                void* mapped = nullptr; // draw_obj_ssbo_item[]
                glm::u32 capacity = 0;  // must be in frames_in_flight sync
                std::vector<glm::uint8> in_flight_data_valid {};
        };

        struct render_order_entry {
                size_t index = UINT64_MAX;
                glm::u32 render_order = UINT32_MAX;
        };

protected:
        void rebuild_order();

        virtual draw_obj_handle_id add_new_draw_obj() override;
        virtual void remove_draw_obj(draw_obj_handle_id to_remove) override;

        void init_pipeline();
        void destroy_pipeline();

        void init_descriptor_sets(VkDevice device);
        void destroy_descriptor_sets(VkDevice device);

        glm::u32 find_memory_type(glm::u32 typeBits, VkMemoryPropertyFlags props);
        bool create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                VkMemoryPropertyFlags props, VkBuffer& outBuf, VkDeviceMemory& outMem, VkDevice device);
        VkShaderModule create_shader_module_from_file(const char* path);

        void create_buffers();
        void destroy_buffers();

        void create_vertex_buffer(VkDevice device);
        void destroy_vertex_buffer(VkDevice device);
        void create_pipe_ubo_buffer(VkDevice device);
        void destroy_pipe_ubo_buffer(VkDevice device);
        void create_draw_obj_ssbo_buffers(VkDevice device, glm::u32 init_capacity);
        void destroy_draw_obj_ssbo_buffers(VkDevice device);
        void resize_capacity_draw_obj_ssbo_buffers(VkDevice device, glm::u32 new_capacity);

protected:
        bool render_order_dirty = false;
        std::vector<render_order_entry> render_order;

        VkPipelineLayout vk_pipeline_layout = VK_NULL_HANDLE;
        VkPipeline vk_pipeline = VK_NULL_HANDLE;

        VkDescriptorSetLayout vk_descriptor_set_layout = VK_NULL_HANDLE;
        VkDescriptorPool vk_descriptor_pool = VK_NULL_HANDLE;
        std::array<VkDescriptorSet, g_app_driver::k_frames_in_flight> vk_descriptor_sets {VK_NULL_HANDLE};

        // TODO: move to shared data resource block
        VkBuffer vk_vertex_buf = VK_NULL_HANDLE;
        VkDeviceMemory vk_vertex_mem = VK_NULL_HANDLE;
        VkBuffer vk_idx_buf = VK_NULL_HANDLE;
        VkDeviceMemory vk_idx_mem = VK_NULL_HANDLE;

        std::array<pipe_frame_ubo_gpu, g_app_driver::k_frames_in_flight> pipe_frame_ubos_data {};
        std::array<draw_obj_ssbo_gpu, g_app_driver::k_frames_in_flight> draw_ssbos_data {};
};


template<class PipelineDataModel>
object_2d_pipeline<PipelineDataModel>::object_2d_pipeline(pipeline_arguments in_args)
        : base_pipeline<PipelineDataModel>(in_args) {
        create_buffers();
        init_pipeline();
}


template<class PipelineDataModel>
object_2d_pipeline<PipelineDataModel>::~object_2d_pipeline() {
        destroy_pipeline();
        destroy_buffers();
}


template<class PipelineDataModel>
void object_2d_pipeline<PipelineDataModel>::draw_commands(VkCommandBuffer in_command_buffer, glm::u32 frame_index) {
#ifdef RAY_DEBUG_NO_OPT
        assert(frame_index < g_app_driver::k_frames_in_flight);
        assert(draw_ssbos_data[frame_index].capacity >= this->draw_obj_data.size());
#endif

        vkCmdBindPipeline(in_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline);

        VkDeviceSize off = 0;
        vkCmdBindVertexBuffers(in_command_buffer, 0, 1, &vk_vertex_buf, &off);
        vkCmdBindIndexBuffer(in_command_buffer, vk_idx_buf, 0, VK_INDEX_TYPE_UINT16);

        const bool in_flight_pipe_valid = pipe_frame_ubos_data[frame_index].in_flight_data_valid;
        if (this->pipe_data.need_update || !in_flight_pipe_valid) {
                if (this->pipe_data.need_update) {
                        for (size_t frame_flight = 0; frame_flight < pipe_frame_ubos_data.size(); ++frame_flight) {
                                pipe_frame_ubos_data[frame_flight].in_flight_data_valid = false;
                        }
                }

                pipe_frame_ubos_data[frame_index].in_flight_data_valid = true;
                this->pipe_data.need_update = false;

                pipe_frame_ubo* ubo_low_obj = reinterpret_cast<pipe_frame_ubo*>(pipe_frame_ubos_data[frame_index].mapped);
                ubo_low_obj->time_ms = this->pipe_data.time_ms;
                ubo_low_obj->camera_transform_ndc = this->pipe_data.camera_transform;
                ubo_low_obj->camera_transform_ndc.x /= this->resolution.x;
                ubo_low_obj->camera_transform_ndc.y /= this->resolution.y;
        }

        const bool dirty_update = render_order_dirty;
        if (render_order_dirty) {
                rebuild_order();
                render_order_dirty = false;
        }

        draw_obj_ssbo_item* ssbo_low_obj_arr = reinterpret_cast<draw_obj_ssbo_item*>(draw_ssbos_data[frame_index].mapped);
        for (size_t i = 0; i < render_order.size(); ++i) {
                auto& render_entry = render_order[i];
                auto& draw_data = this->draw_obj_data[render_entry.index];

                const bool in_flight_item_valid = draw_ssbos_data[frame_index].in_flight_data_valid[i];
                const bool data_need_update = dirty_update || draw_data.need_update;
                if (!data_need_update && in_flight_item_valid) {
                        continue;
                }

                if (data_need_update) {
                        for (size_t frame_flight = 0; frame_flight < draw_ssbos_data.size(); ++frame_flight) {
                                draw_ssbos_data[frame_flight].in_flight_data_valid[i] = false;
                        }
                }
                draw_ssbos_data[frame_index].in_flight_data_valid[i] = true;

                draw_data.need_update = false;
                const bool dirty_order = draw_data.get_render_order() != render_entry.render_order;
                render_order_dirty |= !dirty_update && dirty_order;

                draw_obj_ssbo_item& ssbo_low_obj = ssbo_low_obj_arr[i];

                ssbo_low_obj.color = draw_data.color;
                ssbo_low_obj.render_order = draw_data.get_render_order();
                ssbo_low_obj.space_basis = (glm::u32)draw_data.space_basis;

                ssbo_low_obj.transform_ndc = draw_data.transform;
                ssbo_low_obj.transform_ndc.x /= this->resolution.x;
                ssbo_low_obj.transform_ndc.y /= this->resolution.y;
                ssbo_low_obj.transform_ndc.z /= this->resolution.x;
                ssbo_low_obj.transform_ndc.w /= this->resolution.y;
        }

        vkCmdBindDescriptorSets(
                in_command_buffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                vk_pipeline_layout,
                0,
                1,
                &vk_descriptor_sets[frame_index],
                0,
                nullptr
        );

        if (!render_order.empty()) {
                vkCmdDrawIndexed(in_command_buffer, 6, render_order.size(), 0, 0, 0);
        }
}


template<class PipelineDataModel>
void object_2d_pipeline<PipelineDataModel>::update_swapchain(VkFormat in_swapchain_format, glm::uvec2 in_resolution) {
        destroy_pipeline();
        this->swapchain_format = in_swapchain_format;
        this->resolution = in_resolution;
        init_pipeline();

        for (glm::u32 i = 0; i < pipe_frame_ubos_data.size(); ++i) {
                pipe_frame_ubos_data[i].in_flight_data_valid = false;
        }

        for (glm::u32 i = 0; i < draw_ssbos_data.size(); ++i) {
                std::fill(draw_ssbos_data[i].in_flight_data_valid.begin(), draw_ssbos_data[i].in_flight_data_valid.end(), 0);
        }
}


template<class PipelineDataModel>
void object_2d_pipeline<PipelineDataModel>::rebuild_order() {
        render_order.clear();
        render_order.reserve(this->draw_obj_data.size());

        for (size_t i = 0; i < this->draw_obj_data.size(); ++i) {
                auto& obj_data = this->draw_obj_data[i];
                render_order.push_back({ i, obj_data.get_render_order() });
        }

        std::stable_sort(render_order.begin(), render_order.end(),
            [](const render_order_entry& a, const render_order_entry& b) {
                    return a.render_order < b.render_order;
            });
}


template<class PipelineDataModel>
draw_obj_handle_id object_2d_pipeline<PipelineDataModel>::add_new_draw_obj() {
        draw_obj_handle_id handle = base_pipeline<PipelineDataModel>::add_new_draw_obj();
        render_order_dirty = true;

        if (draw_ssbos_data[0].mapped != nullptr) {
                const glm::u32 curr_capacity = draw_ssbos_data[0].capacity;
                const size_t required_size = this->draw_obj_data.size();

                if (curr_capacity < required_size) {
                        VkDevice device = g_app_driver::thread_safe().device;
                        if (device != VK_NULL_HANDLE) {
                                const glm::u32 new_capacity = draw_ssbos_data[0].capacity * 2;
                                resize_capacity_draw_obj_ssbo_buffers(device, new_capacity);
                        }
                }
        }

        return handle;
}


template<class PipelineDataModel>
void object_2d_pipeline<PipelineDataModel>::remove_draw_obj(draw_obj_handle_id to_remove) {
        base_pipeline<PipelineDataModel>::remove_draw_obj(to_remove);
        render_order_dirty = true;

        if (draw_ssbos_data[0].mapped != nullptr) {
                const glm::u32 curr_capacity = draw_ssbos_data[0].capacity;
                const size_t required_size = this->draw_obj_data.size();

                const glm::u32 optimized_capacity = std::max(curr_capacity / 2, k_init_graphic_object_capacity);
                if (optimized_capacity >= required_size) {
                        VkDevice device = g_app_driver::thread_safe().device;
                        if (device != VK_NULL_HANDLE) {
                                resize_capacity_draw_obj_ssbo_buffers(device, optimized_capacity);
                        }
                }
        }
}


template<class PipelineDataModel>
void object_2d_pipeline<PipelineDataModel>::init_pipeline() {
        if (this->swapchain_format == VK_FORMAT_UNDEFINED) { // It is ok, maybe will be updated later.
                return;
        }

        if (vk_pipeline_layout != VK_NULL_HANDLE || vk_pipeline != VK_NULL_HANDLE) {
                std::println("object_2d_pipeline: can't init. vk_pipeline_layout or vk_pipeline init already.");
                return;
        }

        VkDevice device = g_app_driver::thread_safe().device;
        if (device == VK_NULL_HANDLE) {
                return;
        }

        init_descriptor_sets(device);

        VkPipelineLayoutCreateInfo pipeline_layout_cinf { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipeline_layout_cinf.setLayoutCount = 1;
        pipeline_layout_cinf.pSetLayouts = &vk_descriptor_set_layout;

        if (vkCreatePipelineLayout(device, &pipeline_layout_cinf, nullptr, &vk_pipeline_layout) != VK_SUCCESS) {
                std::println("object_2d_pipeline: vkCreatePipelineLayout failed");
                return;
        }

        VkShaderModule vert = create_shader_module_from_file("../shaders/camera.vert.spv");
        VkShaderModule frag = create_shader_module_from_file("../shaders/mono_color.frag.spv");

        if (!vert || !frag) {
                return;
        }

        VkPipelineShaderStageCreateInfo pipeline_stages[2]{};
        pipeline_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipeline_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipeline_stages[0].module = vert;
        pipeline_stages[0].pName = "main";

        pipeline_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipeline_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipeline_stages[1].module = frag;
        pipeline_stages[1].pName = "main";

        VkVertexInputBindingDescription vertex_input_state_bind_desc {};
        vertex_input_state_bind_desc.binding = 0;
        vertex_input_state_bind_desc.stride = sizeof(ray_vertex);
        vertex_input_state_bind_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription vertex_input_attr_desc[2] {};
        vertex_input_attr_desc[0].location = 0;
        vertex_input_attr_desc[0].binding = 0;
        vertex_input_attr_desc[0].format = VK_FORMAT_R32G32_SFLOAT;
        vertex_input_attr_desc[0].offset = offsetof(ray_vertex, pos);

        vertex_input_attr_desc[1].location = 1;
        vertex_input_attr_desc[1].binding = 0;
        vertex_input_attr_desc[1].format = VK_FORMAT_R32G32_SFLOAT;
        vertex_input_attr_desc[1].offset = offsetof(ray_vertex, uv);

        VkPipelineVertexInputStateCreateInfo vertex_input_state_cinf { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vertex_input_state_cinf.vertexBindingDescriptionCount = 1;
        vertex_input_state_cinf.pVertexBindingDescriptions = &vertex_input_state_bind_desc;
        vertex_input_state_cinf.vertexAttributeDescriptionCount = 2;
        vertex_input_state_cinf.pVertexAttributeDescriptions = vertex_input_attr_desc;

        VkPipelineInputAssemblyStateCreateInfo input_assembly_state_cinf { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        input_assembly_state_cinf.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewport_state_cinf { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewport_state_cinf.viewportCount = 1;
        viewport_state_cinf.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterization_state_cinf { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rasterization_state_cinf.polygonMode = VK_POLYGON_MODE_FILL;
        rasterization_state_cinf.cullMode = VK_CULL_MODE_NONE;
        rasterization_state_cinf.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterization_state_cinf.depthClampEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisample_state_cinf { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        multisample_state_cinf.sampleShadingEnable = VK_FALSE;
        multisample_state_cinf.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState color_blend_attch_state {};
        color_blend_attch_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        color_blend_attch_state.blendEnable = VK_TRUE;
        color_blend_attch_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        color_blend_attch_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        color_blend_attch_state.colorBlendOp        = VK_BLEND_OP_ADD;

        // A   = src.a * 1 + dst.a * (1-src.a)  (fine for UI)
        // add in .frag: outColor = vec4(vColor.rgb * vColor.a, vColor.a);
        //color_blend_attch_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        //color_blend_attch_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        //color_blend_attch_state.alphaBlendOp        = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo color_blend_state_cinf { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        color_blend_state_cinf.logicOpEnable = VK_FALSE;
        color_blend_state_cinf.attachmentCount = 1;
        color_blend_state_cinf.pAttachments = &color_blend_attch_state;

        VkDynamicState dynamic_states[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamic_state_cinf { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamic_state_cinf.dynamicStateCount = 2;
        dynamic_state_cinf.pDynamicStates = dynamic_states;

        VkPipelineRenderingCreateInfo render_cinf { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
        render_cinf.colorAttachmentCount = 1;
        render_cinf.pColorAttachmentFormats = &this->swapchain_format;

        VkGraphicsPipelineCreateInfo pipeline_create_info { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pipeline_create_info.pNext = &render_cinf;
        pipeline_create_info.stageCount = 2;
        pipeline_create_info.pStages = pipeline_stages;
        pipeline_create_info.pVertexInputState = &vertex_input_state_cinf;
        pipeline_create_info.pInputAssemblyState = &input_assembly_state_cinf;
        pipeline_create_info.pViewportState = &viewport_state_cinf;
        pipeline_create_info.pRasterizationState = &rasterization_state_cinf;
        pipeline_create_info.pMultisampleState = &multisample_state_cinf;
        pipeline_create_info.pColorBlendState = &color_blend_state_cinf;
        pipeline_create_info.pDynamicState = &dynamic_state_cinf;
        pipeline_create_info.renderPass = VK_NULL_HANDLE;
        pipeline_create_info.subpass = 0;
        pipeline_create_info.layout = vk_pipeline_layout;

        const auto result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &vk_pipeline);

        vkDestroyShaderModule(device, vert, nullptr);
        vkDestroyShaderModule(device, frag, nullptr);

        if (result != VK_SUCCESS) {
                std::println("object_2d_pipeline: vkCreateGraphicsPipelines failed.");
                return;
        }
}


template<class PipelineDataModel>
void object_2d_pipeline<PipelineDataModel>::destroy_pipeline() {
        VkDevice device = g_app_driver::thread_safe().device;
        if (device == VK_NULL_HANDLE) {
                return;
        }

        destroy_descriptor_sets(device);

        if (vk_pipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(device, vk_pipeline, nullptr);
                vk_pipeline = VK_NULL_HANDLE;
        }

        if (vk_pipeline_layout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(device, vk_pipeline_layout, nullptr);
                vk_pipeline_layout = VK_NULL_HANDLE;
        }
}


template<class PipelineDataModel>
void object_2d_pipeline<PipelineDataModel>::init_descriptor_sets(VkDevice device) {
        VkDescriptorSetLayoutBinding layout_bindings[2] {};

        // pipe UBO
        layout_bindings[0].binding = 0;
        layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layout_bindings[0].descriptorCount = 1;
        layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        // draw objects SSBO
        layout_bindings[1].binding = 1;
        layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        layout_bindings[1].descriptorCount = 1;
        layout_bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layout_info {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        layout_info.bindingCount = 2;
        layout_info.pBindings = layout_bindings;

        VkResult desc_layout_ok = vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &vk_descriptor_set_layout);
        if (desc_layout_ok != VK_SUCCESS) {
                std::println("object_2d_pipeline: vkCreateDescriptorSetLayout failed.");
                return;
        }

        //

        VkDescriptorPoolSize pool_sizes[2] {};
        pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_sizes[0].descriptorCount = g_app_driver::k_frames_in_flight;
        pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        pool_sizes[1].descriptorCount = g_app_driver::k_frames_in_flight;

        VkDescriptorPoolCreateInfo pool_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = g_app_driver::k_frames_in_flight;
        pool_info.poolSizeCount = 2;
        pool_info.pPoolSizes = pool_sizes;

        VkResult desc_pool_ok = vkCreateDescriptorPool(device, &pool_info, nullptr, &vk_descriptor_pool);
        if (desc_pool_ok != VK_SUCCESS) {
                std::println("object_2d_pipeline: vkCreateDescriptorPool failed.");
                return;
        }

        std::vector<VkDescriptorSetLayout> layouts(g_app_driver::k_frames_in_flight, vk_descriptor_set_layout);
        VkDescriptorSetAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        alloc_info.descriptorPool = vk_descriptor_pool;
        alloc_info.descriptorSetCount = g_app_driver::k_frames_in_flight;
        alloc_info.pSetLayouts = layouts.data();

        VkResult desc_alloc_ok = vkAllocateDescriptorSets(device, &alloc_info, vk_descriptor_sets.data());
        if (desc_alloc_ok != VK_SUCCESS) {
                std::println("object_2d_pipeline: vkAllocateDescriptorSets failed.");
                return;
        }

        for (size_t i = 0; i < vk_descriptor_sets.size(); i++)
        {
                VkDescriptorBufferInfo buf_info {};
                buf_info.buffer = pipe_frame_ubos_data[i].buffer;
                buf_info.offset = 0;
                buf_info.range  = sizeof(pipe_frame_ubo);

                VkDescriptorBufferInfo ssbo_info {};
                ssbo_info.buffer = draw_ssbos_data[i].buffer;
                ssbo_info.offset = 0;
                ssbo_info.range  = VK_WHOLE_SIZE;

                VkWriteDescriptorSet writes[2] {};

                writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writes[0].dstSet = vk_descriptor_sets[i];
                writes[0].dstBinding = 0;
                writes[0].dstArrayElement = 0;
                writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                writes[0].descriptorCount = 1;
                writes[0].pBufferInfo = &buf_info;

                writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writes[1].dstSet = vk_descriptor_sets[i];
                writes[1].dstBinding = 1;
                writes[1].dstArrayElement = 0;
                writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                writes[1].descriptorCount = 1;
                writes[1].pBufferInfo = &ssbo_info;

                vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
        }
}


template<class PipelineDataModel>
void object_2d_pipeline<PipelineDataModel>::destroy_descriptor_sets(VkDevice device) {
        if (vk_descriptor_pool != VK_NULL_HANDLE) {
                vkFreeDescriptorSets(device, vk_descriptor_pool, g_app_driver::k_frames_in_flight, vk_descriptor_sets.data());
        }

        if (vk_descriptor_set_layout != VK_NULL_HANDLE) {
                vkDestroyDescriptorSetLayout(device, vk_descriptor_set_layout, nullptr);
        }

        if (vk_descriptor_pool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(device, vk_descriptor_pool, nullptr);
        }

        vk_descriptor_pool = VK_NULL_HANDLE;
        vk_descriptor_set_layout = VK_NULL_HANDLE;
        vk_descriptor_sets.fill(VK_NULL_HANDLE);
}


template<class PipelineDataModel>
glm::u32 object_2d_pipeline<PipelineDataModel>::find_memory_type(glm::u32 typeBits, VkMemoryPropertyFlags props) {
        VkPhysicalDevice physical = g_app_driver::thread_safe().physical;
        if (physical == VK_NULL_HANDLE) {
                return UINT32_MAX;
        }

        VkPhysicalDeviceMemoryProperties mp{};
        vkGetPhysicalDeviceMemoryProperties(physical, &mp);

        for (glm::u32 i = 0; i < mp.memoryTypeCount; ++i) {
                if ((typeBits & (1u << i)) && ((mp.memoryTypes[i].propertyFlags & props) == props)) {
                        return i;
                }
        }

        return UINT32_MAX;
}


template<class PipelineDataModel>
bool object_2d_pipeline<PipelineDataModel>::create_buffer(
        VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, VkBuffer& outBuf, VkDeviceMemory& outMem, VkDevice device) {

        VkBufferCreateInfo bci{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bci.size = size;
        bci.usage = usage;
        bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bci, nullptr, &outBuf) != VK_SUCCESS) {
                std::println("object_2d_pipeline: vkCreateBuffer failed");
                return false;
        }

        VkMemoryRequirements mr{};
        vkGetBufferMemoryRequirements(device, outBuf, &mr);

        uint32_t memType = find_memory_type(mr.memoryTypeBits, props);
        if (memType == UINT32_MAX) {
                std::println("object_2d_pipeline: find_memory_type failed");
                return false;
        }

        VkMemoryAllocateInfo mai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        mai.allocationSize = mr.size;
        mai.memoryTypeIndex = memType;

        if (vkAllocateMemory(device, &mai, nullptr, &outMem) != VK_SUCCESS) {
                std::println("object_2d_pipeline: vkAllocateMemory failed");
                return false;
        }

        vkBindBufferMemory(device, outBuf, outMem, 0);

        return true;
}


template<class PipelineDataModel>
VkShaderModule object_2d_pipeline<PipelineDataModel>::create_shader_module_from_file(const char *path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
                std::println("object_2d_pipeline: failed to open shader file: {}", path);
                return VK_NULL_HANDLE;
        }

        size_t size = (size_t)file.tellg();
        file.seekg(0);

        std::vector<uint32_t> code((size + 3) / 4);
        file.read((char*)code.data(), (std::streamsize)size);

        VkShaderModuleCreateInfo smci{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        smci.codeSize = code.size() * sizeof(uint32_t);
        smci.pCode = code.data();

        VkDevice device = g_app_driver::thread_safe().device;
        if (device == VK_NULL_HANDLE) {
                return VK_NULL_HANDLE;
        }

        VkShaderModule mod = VK_NULL_HANDLE;
        if (vkCreateShaderModule(device, &smci, nullptr, &mod) != VK_SUCCESS) {
                std::println("object_2d_pipeline: vkCreateShaderModule failed: {}", path);
                return VK_NULL_HANDLE;
        }

        return mod;
}


template<class PipelineDataModel>
void object_2d_pipeline<PipelineDataModel>::create_buffers() {
        VkDevice device = g_app_driver::thread_safe().device;
        if (device == VK_NULL_HANDLE) {
                return;
        }

        create_vertex_buffer(device);
        create_pipe_ubo_buffer(device);
        create_draw_obj_ssbo_buffers(device, k_init_graphic_object_capacity);
}


template<class PipelineDataModel>
void object_2d_pipeline<PipelineDataModel>::destroy_buffers() {
        VkDevice device = g_app_driver::thread_safe().device;
        if (device == VK_NULL_HANDLE) {
                return;
        }

        destroy_draw_obj_ssbo_buffers(device);
        destroy_pipe_ubo_buffer(device);
        destroy_vertex_buffer(device);
}


template<class PipelineDataModel>
void object_2d_pipeline<PipelineDataModel>::create_vertex_buffer(VkDevice device) {
        static const ray_vertex verts[] = {
                {{-1.f, -1.f}, {0.f, 1.f}},
                {{1.f, -1.f}, {1.f, 1.f}},
                {{1.f, 1.f}, {1.f, 0.f}},
                {{-1.f, 1.f}, {0.f, 0.f}},
            };
        static const uint16_t idx[] = { 0, 1, 2, 2, 3, 0 };

        VkDeviceSize verts_size = sizeof(verts);
        VkDeviceSize idx_size = sizeof(idx);

        const bool vert_success = create_buffer(verts_size,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                vk_vertex_buf, vk_vertex_mem, device);
        const bool idx_success = create_buffer(idx_size,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                vk_idx_buf, vk_idx_mem, device);

        if (!vert_success || !idx_success) {
                std::println("object_2d_pipeline: create_buffers failed. vert_success {}, idx_success {}", vert_success, idx_success);
                return;
        }

        void* p = nullptr;
        vkMapMemory(device, vk_vertex_mem, 0, verts_size, 0, &p); // means this buffer is writable for cpu
        std::memcpy(p, verts, verts_size); // might be not copy, but direct compute on p memoty
        vkUnmapMemory(device, vk_vertex_mem);

        vkMapMemory(device, vk_idx_mem, 0, idx_size, 0, &p);
        std::memcpy(p, idx, idx_size);
        vkUnmapMemory(device, vk_idx_mem);
}


template<class PipelineDataModel>
void object_2d_pipeline<PipelineDataModel>::destroy_vertex_buffer(VkDevice device){
        if (vk_vertex_buf) {
                vkDestroyBuffer(device, vk_vertex_buf, nullptr);
                vk_vertex_buf = VK_NULL_HANDLE;
        }
        if (vk_vertex_mem) {
                vkFreeMemory(device, vk_vertex_mem, nullptr);
                vk_vertex_mem = VK_NULL_HANDLE;
        }
        if (vk_idx_buf) {
                vkDestroyBuffer(device, vk_idx_buf, nullptr);
                vk_idx_buf = VK_NULL_HANDLE;
        }
        if (vk_idx_mem) {
                vkFreeMemory(device, vk_idx_mem, nullptr);
                vk_idx_mem = VK_NULL_HANDLE;
        }
}


template<class PipelineDataModel>
void object_2d_pipeline<PipelineDataModel>::create_pipe_ubo_buffer(VkDevice device) {
        VkDeviceSize obj_size = sizeof(pipe_frame_ubo);

        for (size_t i = 0; i < pipe_frame_ubos_data.size(); i++)
        {
                const bool buff_ok = create_buffer(
                    obj_size,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    pipe_frame_ubos_data[i].buffer,
                    pipe_frame_ubos_data[i].memory,
                    device
                );

                if (!buff_ok) {
                        std::printf("object_2d_pipeline::create_pipe_ubo_buffer failed to create buffer.");
                        break;
                }

                VkResult memory_ok = vkMapMemory(device, pipe_frame_ubos_data[i].memory, 0, obj_size, 0, &pipe_frame_ubos_data[i].mapped);

                if (memory_ok != VK_SUCCESS) {
                        std::printf("object_2d_pipeline::create_pipe_ubo_buffer failed to map memory.");
                        break;
                }
        }
}


template<class PipelineDataModel>
void object_2d_pipeline<PipelineDataModel>::destroy_pipe_ubo_buffer(VkDevice device) {
        for (uint32_t i = 0; i < pipe_frame_ubos_data.size(); i++) {
                if (pipe_frame_ubos_data[i].mapped) {
                        vkUnmapMemory(device, pipe_frame_ubos_data[i].memory);
                        pipe_frame_ubos_data[i].mapped = nullptr;
                }

                if (pipe_frame_ubos_data[i].buffer) {
                        vkDestroyBuffer(device, pipe_frame_ubos_data[i].buffer, nullptr);
                        pipe_frame_ubos_data[i].buffer = VK_NULL_HANDLE;
                }

                if (pipe_frame_ubos_data[i].memory) {
                        vkFreeMemory(device, pipe_frame_ubos_data[i].memory, nullptr);
                        pipe_frame_ubos_data[i].memory = VK_NULL_HANDLE;
                }

                pipe_frame_ubos_data[i].mapped = nullptr;
        }
}


template<class PipelineDataModel>
void object_2d_pipeline<PipelineDataModel>::create_draw_obj_ssbo_buffers(VkDevice device, glm::u32 init_capacity) {
        init_capacity = std::max<glm::u32>(init_capacity, 1u);

        VkDeviceSize buf_size = (VkDeviceSize)init_capacity * (VkDeviceSize)sizeof(draw_obj_ssbo_item);

        for (glm::u32 i = 0; i < draw_ssbos_data.size(); ++i) {
                const bool buff_ok = create_buffer(
                    buf_size,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    draw_ssbos_data[i].buffer,
                    draw_ssbos_data[i].memory,
                    device
                );

                if (!buff_ok) {
                        std::printf("object_2d_pipeline::create_draw_obj_ssbo_buffers failed to create buffer.\n");
                        break;
                }

                VkResult memory_ok = vkMapMemory(device, draw_ssbos_data[i].memory, 0, buf_size, 0, &draw_ssbos_data[i].mapped);
                if (memory_ok != VK_SUCCESS) {
                        std::printf("object_2d_pipeline::create_draw_obj_ssbo_buffers failed to map memory.\n");
                        break;
                }

                draw_ssbos_data[i].capacity = init_capacity;
                draw_ssbos_data[i].in_flight_data_valid.resize(init_capacity, 0);
                std::fill(draw_ssbos_data[i].in_flight_data_valid.begin(), draw_ssbos_data[i].in_flight_data_valid.end(), 0);
        }
}


template<class PipelineDataModel>
void object_2d_pipeline<PipelineDataModel>::destroy_draw_obj_ssbo_buffers(VkDevice device) {
        for (glm::u32 i = 0; i < draw_ssbos_data.size(); ++i) {
                if (draw_ssbos_data[i].mapped) {
                        vkUnmapMemory(device, draw_ssbos_data[i].memory);
                        draw_ssbos_data[i].mapped = nullptr;
                }

                if (draw_ssbos_data[i].buffer) {
                        vkDestroyBuffer(device, draw_ssbos_data[i].buffer, nullptr);
                        draw_ssbos_data[i].buffer = VK_NULL_HANDLE;
                }

                if (draw_ssbos_data[i].memory) {
                        vkFreeMemory(device, draw_ssbos_data[i].memory, nullptr);
                        draw_ssbos_data[i].memory = VK_NULL_HANDLE;
                }

                draw_ssbos_data[i].capacity = 0;
                draw_ssbos_data[i].in_flight_data_valid.clear();
        }
}


template<class PipelineDataModel>
void object_2d_pipeline<PipelineDataModel>::resize_capacity_draw_obj_ssbo_buffers(VkDevice device, glm::u32 new_capacity) {
        new_capacity = std::max<glm::u32>(new_capacity, 1u);

        // Safe + dumb.
        vkDeviceWaitIdle(device);

        destroy_draw_obj_ssbo_buffers(device);
        create_draw_obj_ssbo_buffers(device, new_capacity);

        if (vk_descriptor_pool != VK_NULL_HANDLE) {
                for (glm::u32 i = 0; i < vk_descriptor_sets.size(); ++i) {
                        VkDescriptorBufferInfo ssbo_info{};
                        ssbo_info.buffer = draw_ssbos_data[i].buffer;
                        ssbo_info.offset = 0;
                        ssbo_info.range  = VK_WHOLE_SIZE;

                        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
                        write.dstSet = vk_descriptor_sets[i];
                        write.dstBinding = 1;
                        write.dstArrayElement = 0;
                        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                        write.descriptorCount = 1;
                        write.pBufferInfo = &ssbo_info;

                        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
                }
        }
}

#endif
};
