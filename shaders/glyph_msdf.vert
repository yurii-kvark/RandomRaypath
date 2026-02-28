#version 450
layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 vUV;
layout(location = 1) out vec4 vFillColor;
layout(location = 2) out vec4 vOutlineColor;
layout(location = 3) out vec4 vBackgroundColor;
layout(location = 4) out float vTextWeight;
layout(location = 5) out float vOutlineSize;

layout(set = 0, binding = 0) uniform pipe_frame_ubo {
    int time_ms;
    int _pad0;
    int _pad1;
    int _pad2;
    vec4 camera_transform_ndc;
} pipe_frame;


struct pipe2d_draw_obj_ssbo {
    vec4 transform_ndc; // x_ndc, y_ndc, w_ndc, h_ndc
    vec4 uv_rect; // u0,v0,u1,v1 in atlas UV
    vec4 color;
    int space_basis;
    float weight;
    float outline_size;
    int _pad0;

    vec4 outline_color;
    vec4 background_color;
};

layout(std430, set = 0, binding = 1) readonly buffer pipe2d_draw_obj_ssbo_s {
    pipe2d_draw_obj_ssbo objs[];
} ssbo;


void main() {
    pipe2d_draw_obj_ssbo inst = ssbo.objs[gl_InstanceIndex];

    vec2 pos = inst.transform_ndc.xy;
    vec2 scl = inst.transform_ndc.zw;

    if (inst.space_basis == 1u) { // world basis
        vec2 camPos = pipe_frame.camera_transform_ndc.xy * 2.0;
        float camScale = pipe_frame.camera_transform_ndc.z;

        pos = (pos - camPos) * camScale;
        scl = scl * camScale;
    }

    vec2 ndc = pos + inPos * scl;
    gl_Position = vec4(ndc, 0.0, 1.0);

    vUV = mix(inst.uv_rect.xy, inst.uv_rect.zw, inUV);
    vFillColor = inst.color;
    vOutlineColor = inst.outline_color;
    vBackgroundColor = inst.background_color;
    vTextWeight = inst.weight;
    vOutlineSize = inst.outline_size;
}