#pragma once
#include "graphics/graphic_libs.h"
#include "glm/glm.hpp"
#include "utils/index_pool.h"

#include <algorithm>
#include <memory>


namespace ray::graphics {

#if RAY_GRAPHICS_ENABLE

//using pipeline_id = gen_index;
using draw_obj_id = gen_index;

class i_pipeline;

template<class ToPipeline, class FromPipeline>
concept ValidPipelineModelUpcastable =
    std::derived_from<typename FromPipeline::pipeline_data_model_t, typename ToPipeline::pipeline_data_model_t> &&
    std::derived_from<typename FromPipeline::draw_obj_model_t, typename ToPipeline::draw_obj_model_t>;

template<class Pipeline = i_pipeline>
struct pipeline_handle {
        std::weak_ptr<i_pipeline> obj_ptr;

        pipeline_handle() = default;

        bool is_valid() const { return !obj_ptr.expired(); }
        operator bool() const { return is_valid(); }

        template<typename OtherPipeline>
        requires ValidPipelineModelUpcastable<Pipeline, OtherPipeline>
        pipeline_handle(const pipeline_handle<OtherPipeline>& other)
                : obj_ptr(other.obj_ptr) {}
};

struct draw_obj_handle_id {
        draw_obj_id id = UINT64_MAX;
        size_t cached_index = INT64_MAX;
};

template<class Pipeline = i_pipeline>
struct draw_obj_handle {
        pipeline_handle<Pipeline> pipe_handle;
        draw_obj_handle_id obj_index;

        draw_obj_handle() = default;

        bool is_valid() const { return obj_index.id != UINT64_MAX && pipe_handle.is_valid(); }
        operator bool() const { return is_valid(); }

        template<typename OtherPipeline>
        requires ValidPipelineModelUpcastable<Pipeline, OtherPipeline>
        draw_obj_handle(const draw_obj_handle<OtherPipeline>& other)
            : pipe_handle(other.pipe_handle), obj_index(other.obj_index) {}
};

struct i_pipeline_data_model {
        struct pipeline { // POD type
                bool need_update = true; // init update true
        };

        struct draw_obj { // POD type
                bool need_update = true; // init update true
                draw_obj_id id = UINT64_MAX;
        };
};


template <typename PipeLine>
concept ValidPipelineDataModel = requires {
        typename PipeLine::pipeline_data_model_t;
        requires std::derived_from<typename PipeLine::pipeline_data_model_t, i_pipeline_data_model::pipeline>;
        typename PipeLine::draw_obj_model_t;
        requires std::derived_from<typename PipeLine::draw_obj_model_t, i_pipeline_data_model::draw_obj>;
};

struct pipeline_arguments {
        std::weak_ptr<index_pool> index_pool;
        VkFormat swapchain_format;
        glm::uvec2 resolution;
        glm::u32 render_order = 0;
};

class i_pipeline {
public:
        using pipeline_data_model_t = typename i_pipeline_data_model::pipeline;
        using draw_obj_model_t = typename i_pipeline_data_model::draw_obj;

        i_pipeline() = delete;

        i_pipeline(pipeline_arguments in_args)
                : obj_index_pool(std::move(in_args.index_pool))
                , swapchain_format(in_args.swapchain_format)
                , resolution(in_args.resolution)
                , pipe_render_order(in_args.render_order) {}
        virtual ~i_pipeline() = default;

        virtual void draw_commands(VkCommandBuffer command_buffer, glm::u32 frame_index) = 0;
        virtual void update_swapchain(VkFormat swapchain_format, glm::uvec2 resolution) = 0;

        draw_obj_handle_id create_draw_obj() {
                return add_new_draw_obj();
        }

        void destroy_draw_obj(draw_obj_handle_id obj_id) {
                remove_draw_obj(obj_id);
        }

        template <class PipeLine>
        requires std::derived_from<PipeLine, i_pipeline>
                && ValidPipelineDataModel<PipeLine>
        PipeLine::pipeline_data_model_t* get_pipeline_model() {
                return static_cast<PipeLine::pipeline_data_model_t*>(get_pipeline_data());
        }

        template <class PipeLine>
        requires std::derived_from<PipeLine, i_pipeline>
                && ValidPipelineDataModel<PipeLine>
        PipeLine::draw_obj_model_t* get_draw_model(draw_obj_handle_id obj_id) {
                return static_cast<PipeLine::draw_obj_model_t*>(get_draw_data(obj_id));
        }

        glm::u32 get_render_order() {
                return pipe_render_order;
        }
protected:
        virtual i_pipeline_data_model::pipeline* get_pipeline_data() = 0;
        virtual i_pipeline_data_model::draw_obj* get_draw_data(draw_obj_handle_id& obj_id) = 0;
        virtual draw_obj_handle_id add_new_draw_obj() = 0;
        virtual void remove_draw_obj(draw_obj_handle_id to_remove) = 0;

        std::weak_ptr<index_pool> obj_index_pool;
        VkFormat swapchain_format = VK_FORMAT_UNDEFINED;
        glm::uvec2 resolution = glm::uvec2(1);
        glm::u32 pipe_render_order = 0;
};


struct base_pipeline_data_model {
        struct pipeline : i_pipeline_data_model::pipeline {
        };

        struct draw_obj : i_pipeline_data_model::draw_obj {
        };
};

template<class PipelineDataModel = base_pipeline_data_model>
requires std::derived_from<typename PipelineDataModel::pipeline, i_pipeline_data_model::pipeline>
         && std::derived_from<typename PipelineDataModel::draw_obj, i_pipeline_data_model::draw_obj>
class base_pipeline: public i_pipeline {
public:
        using pipeline_data_model_t = typename PipelineDataModel::pipeline;
        using draw_obj_model_t = typename PipelineDataModel::draw_obj;

        using i_pipeline::i_pipeline;

protected:
        pipeline_data_model_t pipe_data;
        std::vector<draw_obj_model_t> draw_obj_data;

        virtual i_pipeline_data_model::pipeline* get_pipeline_data() override {
                return &pipe_data;
        }

        virtual i_pipeline_data_model::draw_obj* get_draw_data(draw_obj_handle_id& obj_id) override {
                auto found_obj_it = find_draw_obj(obj_id);

                if (found_obj_it != draw_obj_data.end()) {
                        obj_id.cached_index = (size_t)std::distance(draw_obj_data.begin(), found_obj_it);
                        return std::addressof(*found_obj_it);
                }

                return nullptr;
        }

        virtual draw_obj_handle_id add_new_draw_obj() override {
                draw_obj_id new_id = UINT64_MAX;

                if (auto obj_i_pool = obj_index_pool.lock()) {
                        new_id = obj_i_pool->alloc();
                }

                auto found_obj_it = std::lower_bound(
                        draw_obj_data.begin(), draw_obj_data.end(), new_id,
                        [](const draw_obj_model_t& a, draw_obj_id value) {
                                return a.id < value;
                        });

                auto inserted_obj = draw_obj_data.insert(found_obj_it, draw_obj_model_t());
                inserted_obj->id = new_id;

                return {
                        .id = new_id,
                        .cached_index = (size_t)std::distance(draw_obj_data.begin(), inserted_obj),
                };
        }

        virtual void remove_draw_obj(draw_obj_handle_id to_remove) override {
                auto found_obj_it = find_draw_obj(to_remove);

                if (found_obj_it != draw_obj_data.end()) {
                        if (auto obj_i_pool = obj_index_pool.lock()) {
                                obj_i_pool->free(found_obj_it->id);
                        }
                        draw_obj_data.erase(found_obj_it);
                }
        }

        virtual ~base_pipeline() {
                if (auto obj_i_pool = obj_index_pool.lock()) {
                        for (auto it : draw_obj_data) {
                                obj_i_pool->free(it.id);
                        }
                }
        }

private:
        std::vector<draw_obj_model_t>::iterator find_draw_obj(draw_obj_handle_id obj_id) {
                if (obj_id.cached_index < draw_obj_data.size()) {
                        if (draw_obj_data[obj_id.cached_index].id == obj_id.id) {
                                return draw_obj_data.begin() + obj_id.cached_index;
                        }
                }

                // binary search
                auto found_obj_it = std::lower_bound(
                    draw_obj_data.begin(), draw_obj_data.end(), obj_id.id,
                    [](const draw_obj_model_t& a, draw_obj_id value) {
                        return a.id < value;
                    }
                );

                if (found_obj_it != draw_obj_data.end() && found_obj_it->id == obj_id.id) {
                        return found_obj_it;
                }

                return draw_obj_data.end();
        }
};

#endif
};
