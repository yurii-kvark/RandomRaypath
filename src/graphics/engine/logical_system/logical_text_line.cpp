#include "logical_text_line.h"

#include "graphics/rhi/pipeline/pipeline_manager.h"


#if RAY_GRAPHICS_ENABLE

using namespace ray::graphics;


void logical_text_line::update_content(std::string_view in_new_content) {
        auto pipe_ptr = pipe.obj_ptr.lock();
        if (!pipe_ptr) {
                return;
        }

        auto data_loader_ptr = data_loader.lock();
        if (!data_loader_ptr) {
                return;
        }

        const std::array<glyph_plane_mapping, 256>& planes = data_loader_ptr->plane_mapping;
        const glm::f32 line_top_em = data_loader_ptr->plane_line_top_em;

        glm::f32 cursor_x_em = 0.f;

        for (size_t i = 0; i < draw_obj_handles.size(); ++i) {
                auto glyph_data = pipe_ptr->get_draw_model<glyph_pipeline>(draw_obj_handles[i]);

                if (!glyph_data) {
                        break;
                }

                const unsigned char glyph_value = i < in_new_content.length() ? in_new_content[i] : 0;
                glyph_data->content_glyph = glyph_value;
                glyph_data->need_update = true;

                const glyph_plane_mapping& plane = planes[glyph_value];
                glyph_data->transform = iterate_line_transform(plane, line_top_em, cursor_x_em);
        }
}


void logical_text_line::init(const std::weak_ptr<glyph_font_data>& in_loader, const pipeline_handle<glyph_pipeline>& in_pipe, logical_text_line_args in_args) {
        pipe = in_pipe;
        data_loader = in_loader;

        auto pipe_ptr = pipe.obj_ptr.lock();
        if (!pipe_ptr) {
                return;
        }

        auto data_loader_ptr = data_loader.lock();
        if (!data_loader_ptr) {
                return;
        }

        const glm::u32 allot_capacity = std::max((glm::u32)in_args.content_text.length(), in_args.static_capacity);
        draw_obj_handles.reserve(allot_capacity);

        pivot_transform = in_args.transform;

        const std::array<glyph_plane_mapping, 256>& planes = data_loader_ptr->plane_mapping;
        const glm::f32 line_top_em = data_loader_ptr->plane_line_top_em;

        glm::f32 cursor_x_em = 0.f;

        for (size_t i = 0; i < allot_capacity; ++i) {
                draw_obj_handle_id obj_handle_id = pipe_ptr->create_draw_obj();
                auto glyph_data = pipe_ptr->get_draw_model<glyph_pipeline>(obj_handle_id);

                if (!glyph_data) {
                        break;
                }

                draw_obj_handles.push_back(obj_handle_id);

                const unsigned char glyph_value = i < in_args.content_text.length() ? in_args.content_text[i] : 0;
                glyph_data->content_glyph = glyph_value;

                glyph_data->text_outline_size_ndc = in_args.outline_size_ndc;
                glyph_data->text_outline_color = in_args.outline_color;
                glyph_data->background_color = in_args.background_color;
                glyph_data->space_basis = in_args.space_basis;
                glyph_data->z_order = in_args.z_order;
                glyph_data->color = in_args.text_color;
                glyph_data->need_update = true;

                const glyph_plane_mapping& plane = planes[glyph_value];
                glyph_data->transform = iterate_line_transform(plane, line_top_em, cursor_x_em);
        }
}

glm::vec4 logical_text_line::iterate_line_transform(const glyph_plane_mapping& in_plane, glm::f32 line_top_em, glm::f32& i_cursor_x_em) {
        const glm::f32 glyph_scale_px = std::max(1.f, pivot_transform.w * 0.75f);
        const glm::f32 glyph_width_em = std::max(0.f,  in_plane.right_em() - in_plane.left_em());
        const glm::f32 glyph_height_em = std::max(0.f, in_plane.bottom_em() - in_plane.top_em());

        const glm::f32 left_px = pivot_transform.x + (i_cursor_x_em + in_plane.left_em()) * glyph_scale_px;
        const glm::f32 top_px = pivot_transform.y + (line_top_em + in_plane.top_em()) * glyph_scale_px;

        const glm::f32 width_px = glyph_width_em * glyph_scale_px;
        const glm::f32 height_px = glyph_height_em * glyph_scale_px;

        i_cursor_x_em += std::max(0.f, in_plane.advance_em);

        return glm::vec4(left_px, top_px, width_px, height_px);
}


void logical_text_line::destroy() {
        auto pipe_ptr = pipe.obj_ptr.lock();
        if (!pipe_ptr) {
                return;
        }

        for (const auto handle_id : draw_obj_handles) {
                pipe_ptr->destroy_draw_obj(handle_id);
        }
}


void logical_text_line_manager::init(pipeline_manager& pipe, glm::u32 render_order) {
        text_pipeline_handle = pipe.create_pipeline<glyph_pipeline>(render_order, false);

        auto pipe_ptr = static_cast<glyph_pipeline*>(text_pipeline_handle.obj_ptr.lock().get());
        if (!pipe_ptr) {
                return;
        }

        data_loader = std::make_shared<glyph_font_data>();
        if (!data_loader) {
                return;
        }

        ray_error load_error = data_loader->load_files(
                glyph_font_data::default_rgba_atlas_file,
                glyph_font_data::default_csv_mapping_file);

        if (load_error.has_value()) {
                ray_log(e_log_type::fatal, "can't load glyph_font_data: {}", *load_error);
                return;
        }

        pipe_ptr->provide_construction_data_loader(data_loader);
        pipe_ptr->construct_pipeline();
}


void logical_text_line_manager::destroy(pipeline_manager& pipe) {
        pipe.destroy_pipeline(text_pipeline_handle);
}


pipeline_handle<object_2d_pipeline<>> logical_text_line_manager::get_pipeline() {
        return text_pipeline_handle;
}


struct logical_text_line_deleter {
        void operator()(logical_text_line* line) const {
                if (!line) {
                       return;
                }

                line->destroy();
                delete line;
        }
};


std::shared_ptr<logical_text_line> logical_text_line_manager::create_text_line(logical_text_line_args in_args) {
        if (!text_pipeline_handle.is_valid()) {
                return nullptr;
        }

        std::shared_ptr<logical_text_line> new_line = std::shared_ptr<logical_text_line>(new logical_text_line(), logical_text_line_deleter {});

        if (!new_line || !data_loader) {
                ray_log(e_log_type::fatal, "can't create new logical_text_line");
                return nullptr;
        }

        new_line->init(data_loader, text_pipeline_handle, in_args);

        return new_line;
}

#endif