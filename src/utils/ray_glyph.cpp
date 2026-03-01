#include "ray_glyph.h"

#include "ray_log.h"
#include <format>
#include <iostream>
#include <string>
#include <fstream>

using namespace ray;

static void _trim_cr(std::string& s) {
        if (!s.empty() && s.back() == '\r') {
                s.pop_back();
        }
}

static bool _split_csv(const std::string& line, std::vector<std::string_view>& out, size_t max_split) {
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

static glm::u32 _parse_u32(std::string_view sv) {
        const std::string tmp(sv);
        char* end = nullptr;
        unsigned long v = std::strtoul(tmp.c_str(), &end, 10);
        if (end == tmp.c_str()) {
                ray_log(e_log_type::warning, "Bad integer in CSV");
                return 0;
        }
        return static_cast<glm::u32>(v);
}

static double _parse_f64(std::string_view sv) {
        const std::string tmp(sv);
        char* end = nullptr;
        double v = std::strtod(tmp.c_str(), &end);
        if (end == tmp.c_str()) {
                ray_log(e_log_type::warning, "Bad float in CSV");
                return 0;
        }
        return v;
}


ray_error _load_glyph_mapping_csv_file(const std::filesystem::path& csv_mapping_file, std::vector<glyph_raw_mapping_entry>& out_mapping_vec) {
        std::ifstream file(csv_mapping_file, std::ios::binary);
        if (!file) {
                return std::format("Failed to open glyph CSV: {}", csv_mapping_file.string());
        }

        out_mapping_vec.clear();
        out_mapping_vec.reserve(256);

        std::string line;
        std::vector<std::string_view> cols;

        while (std::getline(file, line)) {
                _trim_cr(line);

                if (line.empty()) {
                        continue;
                }

                if (!_split_csv(line, cols, 10)) {
                        continue;
                }

                if (cols.size() < 6) {
                        continue;
                }

                const glm::u32 codepoint = _parse_u32(cols[0]);

                if (codepoint > 255) {
                        ray_log(e_log_type::warning, "unsupported glyph mapping > 255: {}", cols[0]);
                        continue;
                }

                glyph_raw_mapping_entry entry {};
                if (cols.size() < 10) {
                        continue;
                }

                entry.mapped_character = static_cast<unsigned char>(_parse_u32(cols[0]));
                entry.advance_em = _parse_f64(cols[1]);

                entry.plane_left_em = _parse_f64(cols[2]);
                entry.plane_bottom_em = _parse_f64(cols[3]);
                entry.plane_right_em = _parse_f64(cols[4]);
                entry.plane_top_em = _parse_f64(cols[5]);

                entry.atlas_left_px = _parse_f64(cols[6]);
                entry.atlas_bottom_px = _parse_f64(cols[7]);
                entry.atlas_right_px = _parse_f64(cols[8]);
                entry.atlas_top_px = _parse_f64(cols[9]);

                out_mapping_vec.push_back(entry);
        }

        return {};
}


ray_error glyph_font_data::load_files(std::filesystem::path rgba_atlas_file, std::filesystem::path csv_mapping_file) {
        ray_error rgba_error = load_rgba_file(rgba_atlas_file);
        if (rgba_error.has_value()) {
                return rgba_error;
        }

        ray_error csv_error = load_mapping_file(csv_mapping_file);
        if (csv_error.has_value()) {
                return csv_error;
        }

        if (!is_valid()) {
                return "Extracted data is not valid";
        }

        return {};
}


bool glyph_font_data::is_valid() const {
        return !image_atlas.pixels_rgba.empty()
                && (image_atlas.width * image_atlas.height * 4) == image_atlas.pixels_rgba.size();
}


static std::uint32_t _read_u32_be(std::istream& stream) {
        std::uint8_t b[4]{};
        stream.read(reinterpret_cast<char*>(b), 4);
        return (std::uint32_t(b[0]) << 24) | (std::uint32_t(b[1]) << 16) | (std::uint32_t(b[2]) << 8) | std::uint32_t(b[3]);
}


ray_error glyph_font_data::load_rgba_file(const std::filesystem::path& rgba_atlas_file) {
        image_atlas = glyph_rgba_atlas();

        std::ifstream file(rgba_atlas_file, std::ios::binary);

        if (!file) {
                image_atlas = glyph_rgba_atlas();
                return std::format("Failed to open .rgba file: {}", rgba_atlas_file.string());
        }

        char descriptor[4] {};
        file.read(descriptor, 4);
        if (!(descriptor[0]=='R' && descriptor[1]=='G' && descriptor[2]=='B' && descriptor[3]=='A')) {
                image_atlas = glyph_rgba_atlas();
                return std::format("Not an RGBA file (bad start descriptor) RGBA != {}", descriptor);
        }

        image_atlas.width = _read_u32_be(file);
        image_atlas.height = _read_u32_be(file);

        const std::size_t byte_count = std::size_t(image_atlas.width) * std::size_t(image_atlas.height) * 4u;
        image_atlas.pixels_rgba.resize(byte_count);
        // TODO: write into vulkan mapped memory directly
        file.read(reinterpret_cast<char*>(image_atlas.pixels_rgba.data()), std::streamsize(byte_count));

        if (!file) {
                image_atlas = glyph_rgba_atlas();
                return "RGBA file truncated";
        }

        const glm::u32 expected_size = image_atlas.width * image_atlas.height * 4u;
        if (image_atlas.pixels_rgba.size() != expected_size) {
                image_atlas = glyph_rgba_atlas();
                return std::format("RGBA file sizes not matching: {} != {} (out.width {} * out.height {} * 4u)", image_atlas.pixels_rgba.size(), expected_size, image_atlas.width, image_atlas.height);
        }

        return {};
}


ray_error glyph_font_data::load_mapping_file(const std::filesystem::path& csv_mapping_file) {
        std::vector<glyph_raw_mapping_entry> vec_mapping;
        ray_error load_error = _load_glyph_mapping_csv_file(csv_mapping_file, vec_mapping);

        if (load_error.has_value()) {
                return load_error;
        }

        const glm::f32 inv_w = 1.0f / (glm::f32)std::max(image_atlas.width, 1u);
        const glm::f32 inv_h = 1.0f / (glm::f32)std::max(image_atlas.height, 1u);
        plane_line_top_em = 1.f;

        for (const auto& mapping : vec_mapping) {
                plane_mapping[mapping.mapped_character] = glyph_plane_mapping {
                        .advance_em = mapping.advance_em,
                        .plane_em = glm::vec4(
                                mapping.plane_left_em,
                                mapping.plane_bottom_em,
                                mapping.plane_right_em,
                                mapping.plane_top_em
                        )};

                uv_mapping[mapping.mapped_character] = glyph_uv_mapping {
                        .uv_rect = glm::vec4(
                                mapping.atlas_left_px * inv_w,
                                mapping.atlas_top_px * inv_h,
                                mapping.atlas_right_px * inv_w,
                                mapping.atlas_bottom_px * inv_h
                        )};

                plane_line_top_em = std::max(plane_line_top_em, mapping.plane_top_em);
        }

        return {};
}