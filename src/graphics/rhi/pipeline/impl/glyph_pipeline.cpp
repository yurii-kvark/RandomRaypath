#include "glyph_pipeline.h"
#include "utils/ray_log.h"

#include <algorithm>
#include <cstring>
#include <cstdint>

#if RAY_GRAPHICS_ENABLE

using namespace ray::graphics;
using namespace ray;

void glyph_pipeline::update_render_obj(const typename glyph_pipeline_data_model::draw_obj& inout_draw_data, typename glyph_pipeline_data_model::pipe2d_draw_obj_ssbo& inout_ssbo_obj) {

        inout_ssbo_obj.uv_rect = glyph_mapping[inout_draw_data.content_glyph].uv_rect;

        inout_ssbo_obj.display_enable = inout_draw_data.content_glyph != 0;
        inout_ssbo_obj.outline_size_ndc = inout_draw_data.text_outline_size_ndc;

        inout_ssbo_obj.outline_color = inout_draw_data.text_outline_color;
        inout_ssbo_obj.background_color = inout_draw_data.background_color;

        inout_ssbo_obj.color = inout_draw_data.color;
        inout_ssbo_obj.space_basis = (glm::u32)inout_draw_data.space_basis;

        inout_ssbo_obj.transform_ndc = inout_draw_data.transform;
        inout_ssbo_obj.transform_ndc.x /= this->resolution.x;
        inout_ssbo_obj.transform_ndc.y /= this->resolution.y;
        inout_ssbo_obj.transform_ndc.z /= this->resolution.x;
        inout_ssbo_obj.transform_ndc.w /= this->resolution.y;
}


std::filesystem::path glyph_pipeline::get_vertex_shader_path() const {
        return "glyph_mtsdf.vert.spv";
}


std::filesystem::path ray::graphics::glyph_pipeline::get_fragment_shader_path() const {
        return "glyph_mtsdf.frag.spv";
}


void glyph_pipeline::create_graphical_buffers(VkDevice device) {
        object_2d_pipeline::create_graphical_buffers(device);
        create_atlas_texture(device);
}


void glyph_pipeline::destroy_graphical_buffers(VkDevice device) {
        destroy_atlas_texture(device);
        object_2d_pipeline::destroy_graphical_buffers(device);
}


struct rgba_image {
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::vector<std::uint8_t> pixels_rgba; // width * height * 4
};

static std::uint32_t read_u32_be(std::istream& stream) {
        std::uint8_t b[4]{};
        stream.read(reinterpret_cast<char*>(b), 4);
        return (std::uint32_t(b[0]) << 24) | (std::uint32_t(b[1]) << 16) | (std::uint32_t(b[2]) << 8) | std::uint32_t(b[3]);
}

static rgba_image load_rgba_file(const std::string& file_path) {
        std::ifstream file(file_path, std::ios::binary);
        if (!file) {
                ray_log(e_log_type::warning, "Failed to open .rgba file: {}", file_path);
                return rgba_image();
        }

        char descriptor[4] {};
        file.read(descriptor, 4);
        if (!(descriptor[0]=='R' && descriptor[1]=='G' && descriptor[2]=='B' && descriptor[3]=='A')) {
                ray_log(e_log_type::warning, "Not an RGBA file (bad start descriptor) RGBA != {}", descriptor);
                return rgba_image();
        }

        rgba_image out;
        out.width  = read_u32_be(file);
        out.height = read_u32_be(file);

        const std::size_t byte_count = std::size_t(out.width) * std::size_t(out.height) * 4u;
        out.pixels_rgba.resize(byte_count);
        // TODO: write into vulkan mapped memory directly
        file.read(reinterpret_cast<char*>(out.pixels_rgba.data()), std::streamsize(byte_count));

        if (!file) {
                ray_log(e_log_type::warning, "RGBA file truncated");
                return rgba_image();
        }

        const std::uint32_t expected_size = out.width * out.height * 4u;
        if (out.pixels_rgba.size() != expected_size) {
                ray_log(e_log_type::warning, "RGBA file sizes not matching: {} != {} (out.width {} * out.height {} * 4u)", out.pixels_rgba.size(), expected_size, out.width, out.height);
                return rgba_image();
        }

        return out;
}


static inline void trim_cr(std::string& s) {
        if (!s.empty() && s.back() == '\r') {
                s.pop_back();
        }
}

static bool split_csv(const std::string& line, std::vector<std::string_view>& out, size_t max_split) {
        out.clear();
        out.reserve(max_split);

        const char* b = line.data();
        const char* e = b + line.size();

        const char* token_begin = b;
        for (const char* p = b; p != e; ++p) {
                if (*p == ',') {
                        out.emplace_back(token_begin, std::size_t(p - token_begin));
                        token_begin = p + 1;
                }
        }
        out.emplace_back(token_begin, std::size_t(e - token_begin));
        return !out.empty();
}

static std::uint32_t parse_u32(std::string_view sv) {
        const std::string tmp(sv);
        char* end = nullptr;
        unsigned long v = std::strtoul(tmp.c_str(), &end, 10);
        if (end == tmp.c_str()) {
                ray_log(e_log_type::warning, "Bad integer in CSV");
                return 0;
        }
        return static_cast<std::uint32_t>(v);
}

static double parse_f64(std::string_view sv) {
        const std::string tmp(sv);
        char* end = nullptr;
        double v = std::strtod(tmp.c_str(), &end);
        if (end == tmp.c_str()) {
                ray_log(e_log_type::warning, "Bad float in CSV");
                return 0;
        }
        return v;
}

std::vector<glyph_mapping_entry> load_glyph_mapping_csv_file(const std::string& csv_path) {
        std::ifstream file(csv_path, std::ios::binary);
        if (!file) {
                ray_log(e_log_type::warning, "Failed to open glyph CSV: {}", csv_path);
                return {};
        }

        std::vector<glyph_mapping_entry> result;
        result.reserve(256);

        std::string line;
        std::vector<std::string_view> cols;

        while (std::getline(file, line)) {
                trim_cr(line);

                if (line.empty()) {
                        continue;
                }

                if (!split_csv(line, cols, 10)) {
                        continue;
                }

                if (cols.size() < 6) {
                        continue;
                }

                const std::uint32_t codepoint = parse_u32(cols[0]);

                if (codepoint > 255) {
                        ray_log(e_log_type::warning, "unsupported glyph mapping > 255: {}", cols[0]);
                        continue;
                }

                glyph_mapping_entry entry{};
                if (cols.size() < 10) {
                        continue;
                }

                entry.mapped_character = static_cast<unsigned char>(parse_u32(cols[0]));
                entry.advance_em       = parse_f64(cols[1]);

                entry.plane_left_em   = parse_f64(cols[2]);
                entry.plane_bottom_em = parse_f64(cols[3]);
                entry.plane_right_em  = parse_f64(cols[4]);
                entry.plane_top_em    = parse_f64(cols[5]);

                entry.atlas_left_px   = parse_f64(cols[6]);
                entry.atlas_bottom_px = parse_f64(cols[7]);
                entry.atlas_right_px  = parse_f64(cols[8]);
                entry.atlas_top_px    = parse_f64(cols[9]);

                result.push_back(entry);
        }

        return result;
}


void glyph_pipeline::create_atlas_texture(VkDevice device) {

        rgba_image loaded_image_data = load_rgba_file("../resource/font/gsanscode_w500_mtsdf.rgba");

        if (loaded_image_data.pixels_rgba.empty()) {
                ray_log(e_log_type::fatal, "RGBA font file failed to load.");
                return;
        }

        std::vector<glyph_mapping_entry> glyph_vec = load_glyph_mapping_csv_file("../resource/font/gsanscode_w500.csv");

        if (glyph_vec.empty()) {
                ray_log(e_log_type::fatal, "glyph_mapping font file failed to load");
                return;
        }

        glyph_mapping.clear();
        glyph_mapping.reserve(256);

        const glm::f32 inv_w = 1.0f / (glm::f32)loaded_image_data.width;
        const glm::f32 inv_h = 1.0f / (glm::f32)loaded_image_data.height;

        for (const auto& em : glyph_vec) {
                glyph_mapping[em.mapped_character] = glyph_uv_mapping{
                        .mapped_character = em.mapped_character,
                        .uv_rect = glm::vec4(
                            (glm::f32)em.atlas_left_px * inv_w,
                            (glm::f32)em.atlas_top_px * inv_h,
                            (glm::f32)em.atlas_right_px * inv_w,
                            (glm::f32)em.atlas_bottom_px * inv_h
                            )
                };
        }

        const VkImageCreateInfo image_info {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .extent = { loaded_image_data.width, loaded_image_data.height, 1 },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        const VkResult create_image_res = vkCreateImage(device, &image_info, nullptr, &atlas_image);
        if (create_image_res != VK_SUCCESS) {
                ray_log(e_log_type::fatal, "text_msdf_pipeline::create_atlas_texture failed vkCreateImage = {}", (glm::u32)create_image_res);
                return;
        }

        VkMemoryRequirements mem_req {};
        vkGetImageMemoryRequirements(device, atlas_image, &mem_req);

        const glm::u32 mem_type = find_memory_type(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkMemoryAllocateInfo alloc_info { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        alloc_info.allocationSize = mem_req.size;
        alloc_info.memoryTypeIndex = mem_type;

        const VkResult allocate_image_res = vkAllocateMemory(device, &alloc_info, nullptr, &atlas_memory);
        if (allocate_image_res != VK_SUCCESS) {
                ray_log(e_log_type::fatal, "text_msdf_pipeline::create_atlas_texture failed vkAllocateMemory = {}", (glm::u32)allocate_image_res);
                return;
        }

        const VkResult bind_image_res = vkBindImageMemory(device, atlas_image, atlas_memory, 0);
        if (bind_image_res != VK_SUCCESS) {
                ray_log(e_log_type::fatal, "text_msdf_pipeline::create_atlas_texture failed vkBindImageMemory = {}", (glm::u32)bind_image_res);
                return;
        }

        { // Copy to device image with stage buffer.
                VkBuffer staging_buffer = VK_NULL_HANDLE;
                VkDeviceMemory staging_memory = VK_NULL_HANDLE;

                if (!create_buffer(
                        loaded_image_data.pixels_rgba.size(),
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        staging_buffer, staging_memory, device)) {
                        ray_log(e_log_type::fatal, "text_msdf_pipeline::create_atlas_texture failed create staging buffer");
                        return;
                        }

                void* mapped = nullptr;
                const VkResult map_res = vkMapMemory(device, staging_memory, 0, VK_WHOLE_SIZE, 0, &mapped);
                if (map_res != VK_SUCCESS) {
                        ray_log(e_log_type::fatal, "text_msdf_pipeline::create_atlas_texture failed vkMapMemory = {}", (glm::u32)map_res);
                        vkDestroyBuffer(device, staging_buffer, nullptr);
                        vkFreeMemory(device, staging_memory, nullptr);
                        return;
                }

                std::uint8_t* tex = reinterpret_cast<std::uint8_t*>(mapped);
                std::memcpy(tex, loaded_image_data.pixels_rgba.data(), loaded_image_data.pixels_rgba.size());
                vkUnmapMemory(device, staging_memory);

                const glm::u32 gfx_family = g_app_driver::thread_safe().gfx_family;
                VkQueue gfx_queue = g_app_driver::thread_safe().gfx_queue;

                VkCommandPool transfer_cmd_pool = VK_NULL_HANDLE;
                VkCommandPoolCreateInfo pool_info { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
                pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
                pool_info.queueFamilyIndex = gfx_family;
                const VkResult create_pool_res = vkCreateCommandPool(device, &pool_info, nullptr, &transfer_cmd_pool);
                if (create_pool_res != VK_SUCCESS) {
                        ray_log(e_log_type::fatal, "text_msdf_pipeline::create_atlas_texture failed vkCreateCommandPool = {}", (glm::u32)create_pool_res);
                        vkDestroyBuffer(device, staging_buffer, nullptr);
                        vkFreeMemory(device, staging_memory, nullptr);
                        return;
                }

                VkCommandBuffer transfer_cmd = VK_NULL_HANDLE;
                VkCommandBufferAllocateInfo cmd_alloc_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
                cmd_alloc_info.commandPool = transfer_cmd_pool;
                cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                cmd_alloc_info.commandBufferCount = 1;
                const VkResult alloc_cmd_res = vkAllocateCommandBuffers(device, &cmd_alloc_info, &transfer_cmd);
                if (alloc_cmd_res != VK_SUCCESS) {
                        ray_log(e_log_type::fatal, "text_msdf_pipeline::create_atlas_texture failed vkAllocateCommandBuffers = {}", (glm::u32)alloc_cmd_res);
                        vkDestroyCommandPool(device, transfer_cmd_pool, nullptr);
                        vkDestroyBuffer(device, staging_buffer, nullptr);
                        vkFreeMemory(device, staging_memory, nullptr);
                        return;
                }

                VkCommandBufferBeginInfo begin_info { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
                begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                const VkResult begin_cmd_res = vkBeginCommandBuffer(transfer_cmd, &begin_info);
                if (begin_cmd_res != VK_SUCCESS) {
                        ray_log(e_log_type::fatal, "text_msdf_pipeline::create_atlas_texture failed vkBeginCommandBuffer = {}", (glm::u32)begin_cmd_res);
                        vkDestroyCommandPool(device, transfer_cmd_pool, nullptr);
                        vkDestroyBuffer(device, staging_buffer, nullptr);
                        vkFreeMemory(device, staging_memory, nullptr);
                        return;
                }

                VkImageMemoryBarrier to_transfer_dst { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
                to_transfer_dst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                to_transfer_dst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                to_transfer_dst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                to_transfer_dst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                to_transfer_dst.image = atlas_image;
                to_transfer_dst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                to_transfer_dst.subresourceRange.baseMipLevel = 0;
                to_transfer_dst.subresourceRange.levelCount = 1;
                to_transfer_dst.subresourceRange.baseArrayLayer = 0;
                to_transfer_dst.subresourceRange.layerCount = 1;
                to_transfer_dst.srcAccessMask = 0;
                to_transfer_dst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                vkCmdPipelineBarrier(
                        transfer_cmd,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        0,
                        0, nullptr,
                        0, nullptr,
                        1, &to_transfer_dst
                );

                VkBufferImageCopy copy_region {};
                copy_region.bufferOffset = 0;
                copy_region.bufferRowLength = 0;
                copy_region.bufferImageHeight = 0;
                copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copy_region.imageSubresource.mipLevel = 0;
                copy_region.imageSubresource.baseArrayLayer = 0;
                copy_region.imageSubresource.layerCount = 1;
                copy_region.imageOffset = { 0, 0, 0 };
                copy_region.imageExtent = { loaded_image_data.width, loaded_image_data.height, 1 };
                vkCmdCopyBufferToImage(transfer_cmd, staging_buffer, atlas_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

                VkImageMemoryBarrier to_read_only { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
                to_read_only.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                to_read_only.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                to_read_only.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                to_read_only.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                to_read_only.image = atlas_image;
                to_read_only.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                to_read_only.subresourceRange.baseMipLevel = 0;
                to_read_only.subresourceRange.levelCount = 1;
                to_read_only.subresourceRange.baseArrayLayer = 0;
                to_read_only.subresourceRange.layerCount = 1;
                to_read_only.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                to_read_only.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                vkCmdPipelineBarrier(
                        transfer_cmd,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0,
                        0, nullptr,
                        0, nullptr,
                        1, &to_read_only
                );

                const VkResult end_cmd_res = vkEndCommandBuffer(transfer_cmd);
                if (end_cmd_res != VK_SUCCESS) {
                        ray_log(e_log_type::fatal, "text_msdf_pipeline::create_atlas_texture failed vkEndCommandBuffer = {}", (glm::u32)end_cmd_res);
                        vkDestroyCommandPool(device, transfer_cmd_pool, nullptr);
                        vkDestroyBuffer(device, staging_buffer, nullptr);
                        vkFreeMemory(device, staging_memory, nullptr);
                        return;
                }

                VkSubmitInfo submit_info { VK_STRUCTURE_TYPE_SUBMIT_INFO };
                submit_info.commandBufferCount = 1;
                submit_info.pCommandBuffers = &transfer_cmd;
                const VkResult queue_submit_res = vkQueueSubmit(gfx_queue, 1, &submit_info, VK_NULL_HANDLE);
                if (queue_submit_res != VK_SUCCESS) {
                        ray_log(e_log_type::fatal, "text_msdf_pipeline::create_atlas_texture failed vkQueueSubmit = {}", (glm::u32)queue_submit_res);
                        vkDestroyCommandPool(device, transfer_cmd_pool, nullptr);
                        vkDestroyBuffer(device, staging_buffer, nullptr);
                        vkFreeMemory(device, staging_memory, nullptr);
                        return;
                }

                vkQueueWaitIdle(gfx_queue);

                vkDestroyCommandPool(device, transfer_cmd_pool, nullptr);
                vkDestroyBuffer(device, staging_buffer, nullptr);
                vkFreeMemory(device, staging_memory, nullptr);
        }

        VkImageViewCreateInfo view_info { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        view_info.image = atlas_image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.layerCount = 1;

        const VkResult image_view_res = vkCreateImageView(device, &view_info, nullptr, &atlas_view);
        if (image_view_res != VK_SUCCESS) {
                ray_log(e_log_type::fatal, "text_msdf_pipeline::create_atlas_texture failed vkCreateImageView = {}", (glm::u32)image_view_res);
                return;
        }

        VkSamplerCreateInfo sampler_info { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        sampler_info.magFilter = VK_FILTER_LINEAR;
        sampler_info.minFilter = VK_FILTER_LINEAR;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sampler_info.minLod = 0.0f;
        sampler_info.maxLod = 0.0f;
        sampler_info.mipLodBias = 0.0f;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.anisotropyEnable = VK_TRUE;
        sampler_info.maxAnisotropy = 8.0f;

        const VkResult sampler_res = vkCreateSampler(device, &sampler_info, nullptr, &atlas_sampler);
        if (sampler_res != VK_SUCCESS) {
                ray_log(e_log_type::fatal, "text_msdf_pipeline::create_atlas_texture failed vkCreateSampler = {}", (glm::u32)sampler_res);
                return;
        }
}


void glyph_pipeline::destroy_atlas_texture(VkDevice device) {
        if (atlas_sampler != VK_NULL_HANDLE) {
                vkDestroySampler(device, atlas_sampler, nullptr);
                atlas_sampler = VK_NULL_HANDLE;
        }
        if (atlas_view != VK_NULL_HANDLE) {
                vkDestroyImageView(device, atlas_view, nullptr);
                atlas_view = VK_NULL_HANDLE;
        }
        if (atlas_image != VK_NULL_HANDLE) {
                vkDestroyImage(device, atlas_image, nullptr);
                atlas_image = VK_NULL_HANDLE;
        }
        if (atlas_memory != VK_NULL_HANDLE) {
                vkFreeMemory(device, atlas_memory, nullptr);
                atlas_memory = VK_NULL_HANDLE;
        }
}


std::vector<VkDescriptorSetLayoutBinding> glyph_pipeline::gen_vk_layout_bindings() {
        std::vector<VkDescriptorSetLayoutBinding> layout_bindings = object_2d_pipeline::gen_vk_layout_bindings();
        layout_bindings.push_back ( {
                .binding = (uint32_t)layout_bindings.size(),
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        });
        return layout_bindings;
}


std::vector<VkDescriptorPoolSize> glyph_pipeline::gen_vk_pool_sizes(glm::u32 frame_amount) {
        std::vector<VkDescriptorPoolSize> pool_sizes = object_2d_pipeline::gen_vk_pool_sizes(frame_amount);
        pool_sizes.push_back ( {
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = frame_amount
                });
        return pool_sizes;
}


std::vector<VkWriteDescriptorSet> glyph_pipeline::gen_vk_descriptor_sets(
        const VkDescriptorSet& in_descriptor_set, glm::u32 frame_index,
        std::list<VkDescriptorBufferInfo>& buf_info_lifetime, std::list<VkDescriptorImageInfo>& img_info_lifetime) {

        std::vector<VkWriteDescriptorSet> descriptors = object_2d_pipeline::gen_vk_descriptor_sets(in_descriptor_set, frame_index, buf_info_lifetime, img_info_lifetime);

        img_info_lifetime.push_back( VkDescriptorImageInfo {
                .sampler = atlas_sampler,
                .imageView = atlas_view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL // Good for perf and quality, need stage buffer to upload.
        });
        const auto it_image_info = std::prev(img_info_lifetime.end());

        descriptors.push_back( VkWriteDescriptorSet {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = in_descriptor_set,
                .dstBinding = (uint32_t)descriptors.size(), // 2
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &*it_image_info
        });

        return descriptors;
}


VkPipelineColorBlendAttachmentState glyph_pipeline::gen_vk_pipe_color_blend_atch() const {
        return VkPipelineColorBlendAttachmentState {
                .blendEnable = VK_TRUE,
                .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .alphaBlendOp = VK_BLEND_OP_ADD,
                .colorWriteMask =
                        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                        | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
        };
}

#endif