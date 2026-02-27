#include "text_msdf_pipeline.h"
#include "utils/ray_log.h"

#include <algorithm>
#include <cstring>
#include <cstdint>

#if RAY_GRAPHICS_ENABLE

using namespace ray::graphics;
using namespace ray;


namespace {
constexpr glm::u32 k_flag_is_glyph = 1u << 0;
constexpr glm::u32 k_flag_has_background = 1u << 1;
constexpr glm::f32 k_base_font_height_px = 32.f;
constexpr glm::f32 k_cell_w_px = 20.f;
constexpr glm::f32 k_cell_h_px = 32.f;
constexpr glm::f32 k_whitespace_advance_px = 18.f;
constexpr glm::f32 k_line_height_mul = 1.25f;
constexpr glm::f32 k_px_range = 4.f;
};


void text_msdf_pipeline::update_render_obj(glm::u32 frame_index, bool dirty_update) {
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

/*
void text_msdf_pipeline::draw_commands(VkCommandBuffer in_command_buffer, glm::u32 frame_index) {
        assert(frame_index < g_app_driver::k_frames_in_flight);

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

                frame_ubo_t* ubo_low_obj = reinterpret_cast<frame_ubo_t*>(pipe_frame_ubos_data[frame_index].mapped);
                ubo_low_obj->time_ms = this->pipe_data.time_ms;
                ubo_low_obj->camera_transform_ndc = this->pipe_data.camera_transform;
                ubo_low_obj->camera_transform_ndc.x /= this->resolution.x;
                ubo_low_obj->camera_transform_ndc.y /= this->resolution.y;
        }

        bool any_dirty = packed_instances_dirty;
        for (auto& draw_data : this->draw_obj_data) {
                any_dirty |= draw_data.need_update;
        }

        if (any_dirty) {
                rebuild_instances();
                packed_instances_dirty = false;
                for (auto& draw_data : this->draw_obj_data) {
                        draw_data.need_update = false;
                }

                const glm::u32 required_size = std::max<size_t>(packed_instances.size(), 1);
                if (draw_ssbos_data[0].capacity < required_size) {
                        resize_capacity_draw_obj_ssbo_buffers(g_app_driver::thread_safe().device, std::max(draw_ssbos_data[0].capacity * 2, required_size));
                }
                if (required_size < draw_ssbos_data[0].capacity / 2 && draw_ssbos_data[0].capacity > k_init_graphic_object_capacity) {
                        resize_capacity_draw_obj_ssbo_buffers(g_app_driver::thread_safe().device, std::max(draw_ssbos_data[0].capacity / 2, k_init_graphic_object_capacity));
                }

                for (size_t frame_flight = 0; frame_flight < draw_ssbos_data.size(); ++frame_flight) {
                        std::fill(draw_ssbos_data[frame_flight].in_flight_data_valid.begin(), draw_ssbos_data[frame_flight].in_flight_data_valid.end(), 0);
                }
        }

        if (!packed_instances.empty()) {
                auto* ssbo = reinterpret_cast<draw_obj_ssbo_t*>(draw_ssbos_data[frame_index].mapped);
                std::memcpy(ssbo, packed_instances.data(), packed_instances.size() * sizeof(draw_obj_ssbo_t));
        }

        vkCmdBindDescriptorSets(in_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_layout, 0, 1, &vk_descriptor_sets[frame_index], 0, nullptr);

        if (!packed_instances.empty()) {
                vkCmdDrawIndexed(in_command_buffer, 6, packed_instances.size(), 0, 0, 0);
        }
}



void text_msdf_pipeline::decode_utf8_codepoints(draw_obj_model_t& draw_data) {
        draw_data.cached_codepoints.clear();
        const std::u8string_view text(draw_data.text_content);

        for (size_t i = 0; i < text.size();) {
                const std::uint8_t c0 = static_cast<std::uint8_t>(text[i]);
                glm::u32 cp = 0;
                size_t adv = 1;
                if ((c0 & 0x80u) == 0u) {
                        cp = c0;
                } else if ((c0 & 0xE0u) == 0xC0u && (i + 1) < text.size()) {
                        cp = ((c0 & 0x1Fu) << 6) | (static_cast<std::uint8_t>(text[i + 1]) & 0x3Fu);
                        adv = 2;
                } else if ((c0 & 0xF0u) == 0xE0u && (i + 2) < text.size()) {
                        cp = ((c0 & 0x0Fu) << 12)
                             | ((static_cast<std::uint8_t>(text[i + 1]) & 0x3Fu) << 6)
                             | (static_cast<std::uint8_t>(text[i + 2]) & 0x3Fu);
                        adv = 3;
                } else if ((c0 & 0xF8u) == 0xF0u && (i + 3) < text.size()) {
                        cp = ((c0 & 0x07u) << 18)
                             | ((static_cast<std::uint8_t>(text[i + 1]) & 0x3Fu) << 12)
                             | ((static_cast<std::uint8_t>(text[i + 2]) & 0x3Fu) << 6)
                             | (static_cast<std::uint8_t>(text[i + 3]) & 0x3Fu);
                        adv = 4;
                }
                draw_data.cached_codepoints.push_back(cp);
                i += adv;
        }
}

void text_msdf_pipeline::rebuild_instances() {
        std::vector<order_entry> order {};
        order.reserve(this->draw_obj_data.size());

        for (glm::u32 i = 0; i < this->draw_obj_data.size(); ++i) {
                const auto& draw_data = this->draw_obj_data[i];
                const glm::u32 space_type_bit = (draw_data.space_basis == e_space_type::screen)
                        ? glm::u32(1) << (sizeof(glm::u32) * CHAR_BIT - 1)
                        : 0u;
                order.push_back({ i, draw_data.z_order | space_type_bit, i });
        }

        std::stable_sort(order.begin(), order.end(), [](const order_entry& a, const order_entry& b) {
                if (a.render_order == b.render_order) {
                        return a.seq < b.seq;
                }
                return a.render_order < b.render_order;
        });

        packed_instances.clear();

        for (const auto& entry : order) {
                auto& draw_data = this->draw_obj_data[entry.draw_index];
                decode_utf8_codepoints(draw_data);
                draw_data.cached_line_breaks.clear();
                draw_data.instance_offset = packed_instances.size();
                draw_data.instance_count = 0;

                const glm::f32 px_to_ndc_x = 1.f / this->resolution.x;
                const glm::f32 px_to_ndc_y = 1.f / this->resolution.y;
                const glm::f32 scale = std::max(draw_data.height_px, 1.f) / k_base_font_height_px;
                const glm::f32 line_h = k_cell_h_px * k_line_height_mul * scale;
                const glm::f32 tab_w = k_whitespace_advance_px * scale * std::max(draw_data.tab_size_spaces, 1u);

                glm::f32 min_x = 0.f;
                glm::f32 min_y = 0.f;
                glm::f32 max_x = 0.f;
                glm::f32 max_y = 0.f;
                bool has_box = false;

                glm::f32 pen_x = 0.f;
                glm::f32 pen_y = 0.f;
                glm::u32 last_whitespace_index = UINT32_MAX;

                for (glm::u32 cp_i = 0; cp_i < draw_data.cached_codepoints.size(); ++cp_i) {
                        const glm::u32 cp = draw_data.cached_codepoints[cp_i];
                        if (cp == '\n') {
                                draw_data.cached_line_breaks.push_back(cp_i);
                                pen_x = 0.f;
                                pen_y += line_h;
                                last_whitespace_index = UINT32_MAX;
                                continue;
                        }

                        if (cp == '\t') {
                                const glm::f32 next_tab = std::floor((pen_x + tab_w) / tab_w) * tab_w;
                                pen_x = std::max(next_tab, pen_x + tab_w);
                                last_whitespace_index = cp_i;
                                continue;
                        }

                        if (cp == ' ') {
                                pen_x += k_whitespace_advance_px * scale;
                                last_whitespace_index = cp_i;
                                continue;
                        }

                        if (draw_data.wrap_enabled && draw_data.wrap_width_px > 0.f && pen_x > draw_data.wrap_width_px) {
                                draw_data.cached_line_breaks.push_back(last_whitespace_index == UINT32_MAX ? cp_i : last_whitespace_index);
                                pen_x = 0.f;
                                pen_y += line_h;
                        }

                        draw_obj_ssbo_t inst {};
                        inst.fill_rgba = draw_data.fill_color;
                        inst.outline_rgba = draw_data.outline_color;
                        inst.background_rgba = draw_data.background_color;
                        inst.uv_rect = glm::vec4(0.f, 0.f, 1.f, 1.f);

                        inst.msdf_params.x = k_px_range;
                        inst.msdf_params.y = draw_data.outline_thickness_px;
                        inst.msdf_params.z = static_cast<glm::f32>(k_flag_is_glyph);

                        glm::vec4 tr_px = draw_data.transform;
                        tr_px.x += pen_x;
                        tr_px.y += pen_y;
                        tr_px.z = k_cell_w_px * scale;
                        tr_px.w = k_cell_h_px * scale;

                        inst.transform_ndc = tr_px;
                        inst.transform_ndc.x *= px_to_ndc_x;
                        inst.transform_ndc.y *= px_to_ndc_y;
                        inst.transform_ndc.z *= px_to_ndc_x;
                        inst.transform_ndc.w *= px_to_ndc_y;

                        packed_instances.push_back(inst);
                        draw_data.instance_count += 1;

                        min_x = has_box ? std::min(min_x, tr_px.x) : tr_px.x;
                        min_y = has_box ? std::min(min_y, tr_px.y) : tr_px.y;
                        max_x = has_box ? std::max(max_x, tr_px.x + tr_px.z) : (tr_px.x + tr_px.z);
                        max_y = has_box ? std::max(max_y, tr_px.y + tr_px.w) : (tr_px.y + tr_px.w);
                        has_box = true;

                        pen_x += tr_px.z;
                }

                if (draw_data.background_color.a > 0.f && has_box) {
                        draw_obj_ssbo_t bg_inst {};
                        bg_inst.fill_rgba = draw_data.fill_color;
                        bg_inst.outline_rgba = draw_data.outline_color;
                        bg_inst.background_rgba = draw_data.background_color;
                        bg_inst.uv_rect = glm::vec4(0.f, 0.f, 1.f, 1.f);
                        bg_inst.msdf_params = glm::vec4(k_px_range, 0.f, static_cast<glm::f32>(k_flag_has_background), 0.f);

                        bg_inst.transform_ndc = glm::vec4(min_x, min_y, max_x - min_x, max_y - min_y);
                        bg_inst.transform_ndc.x *= px_to_ndc_x;
                        bg_inst.transform_ndc.y *= px_to_ndc_y;
                        bg_inst.transform_ndc.z *= px_to_ndc_x;
                        bg_inst.transform_ndc.w *= px_to_ndc_y;

                        const auto insert_it = packed_instances.begin() + draw_data.instance_offset;
                        packed_instances.insert(insert_it, bg_inst);
                        draw_data.instance_count += 1;

                        for (auto& other_draw : this->draw_obj_data) {
                                if (other_draw.instance_offset > draw_data.instance_offset) {
                                        other_draw.instance_offset += 1;
                                }
                        }
                }
        }
}
*/

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

        rgba_image loaded_image_data = load_rgba_file("../resources/font/gsanscode_w500_mtsdf.rgba");

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