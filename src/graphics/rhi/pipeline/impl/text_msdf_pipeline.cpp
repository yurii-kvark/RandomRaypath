#include "text_msdf_pipeline.h"
#include "utils/ray_log.h"

#include <algorithm>
#include <cstring>
#include <cstdint>

#if RAY_GRAPHICS_ENABLE

using namespace ray::graphics;
using namespace ray;

void text_msdf_pipeline::update_render_obj(typename text_msdf_pipeline_data_model::draw_obj& inout_draw_data, typename text_msdf_pipeline_data_model::pipe2d_draw_obj_ssbo& inout_ssbo_obj) {

        if (inout_draw_data.glyph_need_update) {
                inout_ssbo_obj.uv_rect = glm::vec4(0.1, 0.4, 0.3, 0.6); // u0,v0,u1,v1 glyph in atlas UV

                inout_draw_data.glyph_need_update = false;
        }

        inout_ssbo_obj.weight = inout_draw_data.text_weight;
        inout_ssbo_obj.outline_size = inout_draw_data.text_outline_size;

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


std::filesystem::path text_msdf_pipeline::get_vertex_shader_path() const {
        return "text_msdf.vert.spv";
}


std::filesystem::path ray::graphics::text_msdf_pipeline::get_fragment_shader_path() const {
        return "text_msdf.frag.spv";
}


void text_msdf_pipeline::create_graphical_buffers(VkDevice device) {
        object_2d_pipeline::create_graphical_buffers(device);
        create_atlas_texture(device);
}


void text_msdf_pipeline::destroy_graphical_buffers(VkDevice device) {
        destroy_atlas_texture(device);
        object_2d_pipeline::destroy_graphical_buffers(device);
}


#include <vector>
#include <fstream>
#include <stdexcept>
#include <string>

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

std::unordered_map<unsigned char, glyph_mapping_entry> load_glyph_mapping_csv_file(const std::string& csv_path) {
        std::ifstream file(csv_path, std::ios::binary);
        if (!file) {
                ray_log(e_log_type::warning, "Failed to open glyph CSV: {}", csv_path);
                return {};
        }

        std::unordered_map<unsigned char, glyph_mapping_entry> result;
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
                entry.mapped_character = static_cast<unsigned char>(codepoint);
                entry.advance_em = parse_f64(cols[1]);

                const double plane_left   = parse_f64(cols[2]);
                const double plane_top    = parse_f64(cols[3]);
                const double plane_right  = parse_f64(cols[4]);
                const double plane_bottom = parse_f64(cols[5]);

                entry.x_begin_em = plane_left;
                entry.x_end_em   = plane_right;
                entry.y_begin_em = plane_top;
                entry.y_end_em   = plane_bottom;

                result[entry.mapped_character] = entry;
        }

        return result;
}

void text_msdf_pipeline::create_atlas_texture(VkDevice device) {

        rgba_image loaded_image_data = load_rgba_file("../resource/font/gsanscode_w500_mtsdf.rgba");

        if (loaded_image_data.pixels_rgba.empty()) {
                ray_log(e_log_type::fatal, "RGBA font file failed to load.");
                return;
        }

        glyph_mapping = load_glyph_mapping_csv_file("../resource/font/gsanscode_w500.csv");

        if (glyph_mapping.empty()) {
                ray_log(e_log_type::fatal, "glyph_mapping font file failed to load");
                return;
        }

        const VkImageCreateInfo image_info {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .extent = { loaded_image_data.width, loaded_image_data.height, 1 },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_LINEAR,
                .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED,
        };

        const VkResult create_image_res = vkCreateImage(device, &image_info, nullptr, &atlas_image);
        if (create_image_res != VK_SUCCESS) {
                ray_log(e_log_type::fatal, "text_msdf_pipeline::create_atlas_texture failed vkCreateImage = {}", (glm::u32)create_image_res);
                return;
        }

        VkMemoryRequirements mem_req {};
        vkGetImageMemoryRequirements(device, atlas_image, &mem_req);

        const glm::u32 mem_type = find_memory_type(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
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

        void* mapped = nullptr;
        vkMapMemory(device, atlas_memory, 0, VK_WHOLE_SIZE, 0, &mapped);
        std::uint8_t* tex = reinterpret_cast<std::uint8_t*>(mapped);

        std::memcpy(tex, loaded_image_data.pixels_rgba.data(), loaded_image_data.pixels_rgba.size());

        vkUnmapMemory(device, atlas_memory);

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
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.maxLod = 1.0f;

        const VkResult sampler_res = vkCreateSampler(device, &sampler_info, nullptr, &atlas_sampler);
        if (sampler_res != VK_SUCCESS) {
                ray_log(e_log_type::fatal, "text_msdf_pipeline::create_atlas_texture failed vkCreateSampler = {}", (glm::u32)sampler_res);
                return;
        }
}


void text_msdf_pipeline::destroy_atlas_texture(VkDevice device) {
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


std::vector<VkDescriptorSetLayoutBinding> text_msdf_pipeline::generate_layout_bindings() {
        std::vector<VkDescriptorSetLayoutBinding> layout_bindings = object_2d_pipeline::generate_layout_bindings();
        layout_bindings.push_back ( {
                .binding = (uint32_t)layout_bindings.size(),
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        });
        return layout_bindings;
}


std::vector<VkDescriptorPoolSize> text_msdf_pipeline::generate_pool_sizes(glm::u32 frame_amount) {
        std::vector<VkDescriptorPoolSize> pool_sizes = object_2d_pipeline::generate_pool_sizes(frame_amount);
        pool_sizes.push_back ( {
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = frame_amount
                });
        return pool_sizes;
}


std::vector<VkWriteDescriptorSet> text_msdf_pipeline::generate_descriptor_sets(
        const VkDescriptorSet& in_descriptor_set, glm::u32 frame_index,
        std::list<VkDescriptorBufferInfo>& buf_info_lifetime, std::list<VkDescriptorImageInfo>& img_info_lifetime) {

        std::vector<VkWriteDescriptorSet> descriptors = object_2d_pipeline::generate_descriptor_sets(in_descriptor_set, frame_index, buf_info_lifetime, img_info_lifetime);

        img_info_lifetime.push_back( VkDescriptorImageInfo {
                .sampler = atlas_sampler,
                .imageView = atlas_view,
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL
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

#endif