#version 450
layout(location=0) in vec2 vUV;
layout(location=1) in vec4 vColor;
layout(location=0) out vec4 outColor;

layout(set = 0, binding = 0) uniform pipe_frame_ubo {
    int time_ms;
    int _pad0;
    int _pad1;
    int _pad2;
    vec4 camera_transform_ndc; // x_px, y_px, scale, 1.0
} pipe_frame;


vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    float time_float = float(pipe_frame.time_ms);
    float time_ticker = time_float / 1000.f;
    float hue = fract(vUV.x + 0.15 * sin(time_ticker));
    float scan = 0.8 + 0.2 * sin(vUV.y * 400.0 + time_ticker * 80.0);
    vec3 rgb = hsv2rgb(vec3(hue, 1.0, 1.0)) * scan;
    outColor = vColor * 0.5 + vec4(rgb, 1.0) * 0.5;
}