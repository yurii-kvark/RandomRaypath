#include "logical_text_line.h"

#include "graphics/rhi/pipeline/pipeline_manager.h"


#if RAY_GRAPHICS_ENABLE

using namespace ray::graphics;


void logical_text_line::update_content(std::string_view in_new_content) {
        auto pipe_ptr = pipe.obj_ptr.lock();
        if (!pipe_ptr) {
                return;
        }

        for (size_t i = 0; i < draw_obj_handles.size(); ++i) {
                auto glyph_data = pipe_ptr->get_draw_model<glyph_pipeline>(draw_obj_handles[i]);

                if (!glyph_data) {
                        break;
                }

                const unsigned char glyph_value = i < in_new_content.length() ? in_new_content[i] : 0;
                glyph_data->content_glyph = glyph_value;
                glyph_data->transform = {i * 50, 0, 20, 20}; // TODO: x_pos_px, y_pos_px, x_size_px, y_size_px         glm::vec4 in_args.transform = glm::vec4(0.f, 0.f, 0.f, 50.f); // x_pos_px, y_pos_px, 0, y_size_px (pivot top left)
                glyph_data->need_update = true;
        }
}


void logical_text_line::init(const glyph_font_data_loader& data_loader, const pipeline_handle<glyph_pipeline>& in_pipe, logical_text_line_args in_args) {
        pipe = in_pipe;

        if (!pipe.is_valid()) {
                return;
        }

        const glm::u32 allot_capacity = std::max((glm::u32)in_args.content_text.length(), in_args.static_capacity);

        auto pipe_ptr = pipe.obj_ptr.lock();
        if (!pipe_ptr) {
                return;
        }

        draw_obj_handles.reserve(allot_capacity);

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
                glyph_data->transform = {i * 50, 0, 20, 20}; // TODO: x_pos_px, y_pos_px, x_size_px, y_size_px         glm::vec4 in_args.transform = glm::vec4(0.f, 0.f, 0.f, 50.f); // x_pos_px, y_pos_px, 0, y_size_px (pivot top left)
                glyph_data->color = in_args.text_color;
                glyph_data->need_update = true;
        }

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

        data_loader = std::make_shared<glyph_font_data_loader>(
                glyph_font_data_loader::default_rgba_fontpath,
                glyph_font_data_loader::default_csv_mappath);

        if (!data_loader) {
                return;
        }

        pipe_ptr->provide_construction_data_loader(data_loader);
        pipe_ptr->construct_pipeline();

        data_loader->reset_image_cache();
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

        new_line->init(*data_loader, text_pipeline_handle, in_args);

        return new_line;
}

#endif