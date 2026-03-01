#pragma once
#include "../object_2d_pipeline.h"
#include "utils/ray_font.h"


namespace ray::graphics {

#if RAY_GRAPHICS_ENABLE

struct glyph_pipeline_data_model {
        struct pipeline : object_2d_pipeline_data_model::pipeline {
        };

        struct draw_obj : object_2d_pipeline_data_model::draw_obj {
                unsigned char content_glyph = 0;
                glm::f32 text_outline_size_ndc = 1.f;
                glm::vec4 text_outline_color {};
                glm::vec4 background_color {};
        };

        using pipe2d_frame_ubo = object_2d_pipeline_data_model::pipe2d_frame_ubo;

        struct alignas(16) pipe2d_draw_obj_ssbo {
                glm::vec4 transform_ndc = {}; // x_ndc, y_ndc, w_ndc, h_ndc
                glm::vec4 uv_rect; // u0,v0,u1,v1 glyph in atlas UV
                glm::vec4 color = {};
                glm::u32 space_basis = 0.f;
                glm::u32 display_enable = 1.f;
                glm::f32 outline_size_ndc = 0.f;
                glm::i32 _pad0 = 0;

                glm::vec4 outline_color;
                glm::vec4 background_color;
        };
};


struct glyph_uv_mapping {
        unsigned char mapped_character = 0;
        glm::vec4 uv_rect = {};
};

class glyph_pipeline final : public object_2d_pipeline<glyph_pipeline_data_model> {
public:
        using object_2d_pipeline::object_2d_pipeline;
public:
        void provide_construction_data_loader(std::weak_ptr<glyph_font_data_loader> data_loader);
        virtual void update_render_obj(const typename glyph_pipeline_data_model::draw_obj& inout_draw_data, typename glyph_pipeline_data_model::pipe2d_draw_obj_ssbo& inout_ssbo_obj) override;

protected:
        virtual std::filesystem::path get_vertex_shader_path() const override;
        virtual std::filesystem::path get_fragment_shader_path() const override;

        virtual void create_graphical_buffers(VkDevice device) override;
        virtual void destroy_graphical_buffers(VkDevice device) override;

        virtual std::vector<VkDescriptorSetLayoutBinding> gen_vk_layout_bindings() override;
        virtual std::vector<VkDescriptorPoolSize> gen_vk_pool_sizes(glm::u32 frame_amount) override;
        virtual std::vector<VkWriteDescriptorSet> gen_vk_descriptor_sets(
                const VkDescriptorSet& in_descriptor_set, glm::u32 frame_index,
                std::list<VkDescriptorBufferInfo>& buf_info_lifetime, std::list<VkDescriptorImageInfo>& img_info_lifetime) override;
        virtual VkPipelineColorBlendAttachmentState gen_vk_pipe_color_blend_atch() const override;

        // TODO: move to shared static memory
        VkImage atlas_image = VK_NULL_HANDLE;
        VkDeviceMemory atlas_memory = VK_NULL_HANDLE;
        VkImageView atlas_view = VK_NULL_HANDLE;
        VkSampler atlas_sampler = VK_NULL_HANDLE;

        std::weak_ptr<glyph_font_data_loader> construction_data_loader;
        std::unordered_map<unsigned char, glyph_uv_mapping> glyph_mapping;

private:
        void create_atlas_texture(VkDevice device);
        void destroy_atlas_texture(VkDevice device);
};

#endif
};