#version 450
layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 vUV;
layout(location = 1) out vec4 vColor;

layout(set = 0, binding = 0) uniform pipe_frame_ubo {
    int time_ms;
    int _pad0;
    int _pad1;
    int _pad2;
    vec4 camera_transform_ndc; // x_px, y_px, scale, 1.0
} pipe_frame;

struct pipe2d_draw_obj_ssbo {
    vec4 transform_ndc;  // x_ndc, y_ndc, wFactor, hFactor
    vec4 color;
    uint space_basis;
    uint _pad0;
    uint _pad1;
    uint _pad2;
};

layout(std430, set = 0, binding = 1) readonly buffer pipe2d_draw_obj_ssbos {
    pipe2d_draw_obj_ssbo objs[];
} ssbo;


//void main() {
//    vUV = inUV;
//
//    vec2 p = inPos;
//    p = (p - pipe_frame.camera_transform_ndc.xy) * pipe_frame.camera_transform_ndc.z;
//    gl_Position = vec4(p, 0.0, 1.0);
//}


void main() {
    pipe2d_draw_obj_ssbo obj = ssbo.objs[gl_InstanceIndex];

    vec2 pos = obj.transform_ndc.xy;
    vec2 scl = obj.transform_ndc.zw;

    if (obj.space_basis == 1u) { // world basis
        vec2 camPos = pipe_frame.camera_transform_ndc.xy * 2.0;
        float camScale = pipe_frame.camera_transform_ndc.z;

        pos = (pos - camPos) * camScale;
        scl = scl * camScale;
    }

    vec2 ndc = pos + inPos * scl;
    // No render_order, it done by ssao object ordering on cpu.
    // Enabling transparency in this way.
    float depth = 0;
    gl_Position = vec4(ndc, depth, 1.0);

    vUV = inUV;
    vColor = obj.color;
}