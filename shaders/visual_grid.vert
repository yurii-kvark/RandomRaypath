#version 450
layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 vUV;
layout(location = 1) flat out vec4 vColor;
layout(location = 2) flat out vec4 vBackgroundColor;
layout(location = 3) flat out vec2 vGridSize_ndc;
layout(location = 4) flat out vec2 vLineSize_ndc;
layout(location = 5) flat out vec4 vCameraTransform_ndc;
layout(location = 6) flat out vec4 vRectTransform_ndc;
layout(location = 7) flat out int  vSpaceBasis;
layout(location = 8) flat out int vApplyFragCam;


layout(set = 0, binding = 0) uniform pipe_frame_ubo {
    int time_ms;
    int _pad0;
    int _pad1;
    int _pad2;
    vec4 camera_transform_ndc; // x_px, y_px, scale, 1.0
} pipe_frame;

struct visual_grid_ssbo_entry { // visual_grid_ssbo_boj
    vec4 transform_ndc; // x_ndc, y_ndc, w_ndc, h_ndc
    vec4 color;
    vec4 background_color;
    vec2 grid_size_ndc;
    vec2 line_size_ndc;
    int space_basis;
    int apply_camera_to_frag;
};

layout(std430, set = 0, binding = 1) readonly buffer visual_grid_draw_obj_ssbos {
    visual_grid_ssbo_entry objs[];
} ssbo;


void main() {
    visual_grid_ssbo_entry obj = ssbo.objs[gl_InstanceIndex];

    vec2 pos = obj.transform_ndc.xy;
    vec2 scl = obj.transform_ndc.zw;

    // World-space NDC position of this fragment (pre-camera).
    // For space_basis==0 (screen) this equals screen NDC directly.
    // For space_basis==1 (world) this is the world position before the camera transform.
    // The fragment shader uses this as the grid coordinate so the grid tiles in a
    // globally consistent coordinate space, not relative to the rect's [0,1] UV extent.
    vUV = pos + inPos * scl;
    vGridSize_ndc = obj.grid_size_ndc * scl * 2;
    vLineSize_ndc = obj.line_size_ndc * scl * 2;

    if (obj.space_basis == 1u) { // world basis
        vec2 camPos = pipe_frame.camera_transform_ndc.xy;
        float camScale = pipe_frame.camera_transform_ndc.z;

        pos = (pos - camPos) * camScale;
        scl = scl * camScale;
    }

    vec2 ndc = pos + inPos * scl;

    gl_Position = vec4(ndc, 0, 1.0);

    vCameraTransform_ndc = pipe_frame.camera_transform_ndc;
    vRectTransform_ndc = obj.transform_ndc;

    vSpaceBasis = obj.space_basis;
    vApplyFragCam = obj.apply_camera_to_frag;

    vColor = obj.color;
    vBackgroundColor = obj.background_color;
}