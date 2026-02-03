#version 450
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PC {
    float time;
} pc;



vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    float hue = fract(vUV.x + 0.15 * sin(pc.time));
    float scan = 0.9 + 0.1 * sin(vUV.y * 600.0 + pc.time * 12.0);
    vec3 rgb = hsv2rgb(vec3(hue, 1.0, 1.0)) * scan;
    outColor = vec4(rgb, 1.0);
}