#include "ray_font.h"

#include "ray_log.h"

#include <fstream>

using namespace ray;

static std::uint32_t read_u32_be(std::istream& stream) {
        std::uint8_t b[4]{};
        stream.read(reinterpret_cast<char*>(b), 4);
        return (std::uint32_t(b[0]) << 24) | (std::uint32_t(b[1]) << 16) | (std::uint32_t(b[2]) << 8) | std::uint32_t(b[3]);
}


void glyph_font_data_loader::load_rgba_file() {
        glyph_image_atlas = rgba_image();

        std::ifstream file(rgba_font_file, std::ios::binary);
        if (!file) {
                ray_log(e_log_type::warning, "Failed to open .rgba file: {}", rgba_font_file);
                glyph_image_atlas = rgba_image();
                return;
        }

        char descriptor[4] {};
        file.read(descriptor, 4);
        if (!(descriptor[0]=='R' && descriptor[1]=='G' && descriptor[2]=='B' && descriptor[3]=='A')) {
                ray_log(e_log_type::warning, "Not an RGBA file (bad start descriptor) RGBA != {}", descriptor);
                glyph_image_atlas = rgba_image();
                return;
        }

        glyph_image_atlas.width  = read_u32_be(file);
        glyph_image_atlas.height = read_u32_be(file);

        const std::size_t byte_count = std::size_t(glyph_image_atlas.width) * std::size_t(glyph_image_atlas.height) * 4u;
        glyph_image_atlas.pixels_rgba.resize(byte_count);
        // TODO: write into vulkan mapped memory directly
        file.read(reinterpret_cast<char*>(glyph_image_atlas.pixels_rgba.data()), std::streamsize(byte_count));

        if (!file) {
                ray_log(e_log_type::warning, "RGBA file truncated");
                glyph_image_atlas = rgba_image();
                return;
        }

        const std::uint32_t expected_size = glyph_image_atlas.width * glyph_image_atlas.height * 4u;
        if (glyph_image_atlas.pixels_rgba.size() != expected_size) {
                ray_log(e_log_type::warning, "RGBA file sizes not matching: {} != {} (out.width {} * out.height {} * 4u)", glyph_image_atlas.pixels_rgba.size(), expected_size, glyph_image_atlas.width, glyph_image_atlas.height);
                glyph_image_atlas = rgba_image();
                return;
        }
}


static void trim_cr(std::string& s) {
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

void glyph_font_data_loader::load_glyph_mapping_csv_file() {
        glyph_raw_mapping.clear();

        std::ifstream file(csv_font_map, std::ios::binary);
        if (!file) {
                ray_log(e_log_type::warning, "Failed to open glyph CSV: {}", csv_font_map);
                return;
        }

        glyph_raw_mapping.reserve(256);

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

                glyph_raw_mapping.push_back(entry);
        }
}


const rgba_image& glyph_font_data_loader::load_image() {
        if (!image_loaded) {
                load_rgba_file();
                image_loaded = true;
        }
        return glyph_image_atlas;
}


const std::vector<glyph_mapping_entry>& glyph_font_data_loader::load_raw_mapping() {
        if (!mapping_loaded) {
                load_glyph_mapping_csv_file();
                mapping_loaded = true;
        }
        return glyph_raw_mapping;
}


void glyph_font_data_loader::reset_image_cache() {
        glyph_image_atlas = load_image();
        image_loaded = false;

}