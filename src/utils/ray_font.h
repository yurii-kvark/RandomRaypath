#pragma once
#include <vector>
#include <string_view>
#include "glm/glm.hpp"

#include <string>

namespace ray {

struct glyph_mapping_entry {
        unsigned char mapped_character = 0;
        double advance_em = 0;

        double plane_left_em = 0;
        double plane_bottom_em = 0;
        double plane_right_em = 0;
        double plane_top_em = 0;

        double atlas_left_px = 0;
        double atlas_bottom_px = 0;
        double atlas_right_px = 0;
        double atlas_top_px = 0;
};

struct rgba_image {
        glm::u32 width = 0;
        glm::u32 height = 0;
        std::vector<glm::u8> pixels_rgba; // width * height * 4
};

class glyph_font_data_loader {
public:
        static constexpr char default_rgba_fontpath[] = "../resource/font/gsanscode_w500_mtsdf.rgba";
        static constexpr char default_csv_mappath[] = "../resource/font/gsanscode_w500.csv";

        glyph_font_data_loader(std::string_view in_rgba_font_file, std::string_view in_csv_font_map)
                : rgba_font_file(in_rgba_font_file), csv_font_map(in_csv_font_map)
                {}

        const rgba_image& load_image(); // no image cache
        const std::vector<glyph_mapping_entry>& load_raw_mapping(); // will cache

        void reset_image_cache();

protected:
        void load_rgba_file();
        void load_glyph_mapping_csv_file();

        std::vector<glyph_mapping_entry> glyph_raw_mapping;
        rgba_image glyph_image_atlas;

        std::string rgba_font_file;
        std::string csv_font_map;

        bool image_loaded = false;
        bool mapping_loaded = false;
};

};
