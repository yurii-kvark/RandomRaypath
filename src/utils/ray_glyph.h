#pragma once
#include "ray_error.h"


#include <vector>
#include <string_view>
#include "glm/glm.hpp"

#include <array>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace ray {

struct glyph_rgba_atlas {
        glm::u32 width = 0;
        glm::u32 height = 0;
        std::vector<glm::u8> pixels_rgba; // width * height * 4
};

struct glyph_uv_mapping {
        glm::vec4 uv_rect = {};
};

struct glyph_plane_mapping {
        glm::f32 advance_em = 0.4f;
        glm::vec4 plane_em = {}; // left, bottom, right, top

        glm::f32 left_em() const { return plane_em.x; }
        glm::f32 top_em() const { return plane_em.y; }
        glm::f32 right_em() const { return plane_em.z; }
        glm::f32 bottom_em() const { return plane_em.w; }
};


struct glyph_raw_mapping_entry {
        unsigned char mapped_character = 0;
        glm::f32 advance_em = 0;

        glm::f32 plane_left_em = 0;
        glm::f32 plane_top_em = 0;
        glm::f32 plane_right_em = 0;
        glm::f32 plane_bottom_em = 0;

        glm::f32 atlas_left_px = 0;
        glm::f32 atlas_top_px = 0;
        glm::f32 atlas_right_px = 0;
        glm::f32 atlas_bottom_px = 0;
};

class glyph_font_data {
public:
        static constexpr char default_rgba_atlas_file[] = "../resource/font/gsanscode_w500_mtsdf.rgba";
        static constexpr char default_csv_mapping_file[] = "../resource/font/gsanscode_w500.csv";

        ray_error load_files(std::filesystem::path rgba_atlas_file, std::filesystem::path csv_mapping_file);
        bool is_valid() const;

        // read only
        glyph_rgba_atlas image_atlas {};
        std::array<glyph_plane_mapping, 256> plane_mapping {};
        std::array<glyph_uv_mapping, 256> uv_mapping {};
        glm::f32 plane_line_top_em = 1.f;

protected:
        ray_error load_rgba_file(const std::filesystem::path& rgba_atlas_file);
        ray_error load_mapping_file(const std::filesystem::path& csv_mapping_file);
};

};
