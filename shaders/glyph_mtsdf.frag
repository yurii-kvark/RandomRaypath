#version 450
layout(location = 0) in vec2 vUV;
layout(location = 1) flat in int vDisplayEnable;
layout(location = 2) in vec4 vFillColor;
layout(location = 3) in vec4 vOutlineColor;
layout(location = 4) in vec4 vBackgroundColor;
layout(location = 5) in float vOutlineSize_ndc;

layout(location = 0) out vec4 outColor;
layout(set = 0, binding = 2) uniform sampler2D atlasSampler;

float median3(vec3 v) {
    return max(min(v.r, v.g), min(max(v.r, v.g), v.b));
}

vec4 premul(vec3 rgb, float a) {
    return vec4(rgb * a, a);
}

void main() {
    if (vDisplayEnable == 0) {
        outColor = vec4(0.0);
        return;
    }

    vec4 t = texture(atlasSampler, vUV);

    float d_msdf = median3(t.rgb);
    float d_sdf  = t.a;
    float d = max(d_msdf, d_sdf);
    float w = max(fwidth(d), 1e-4);

    float fillAlpha  = smoothstep(0.5 - w, 0.5 + w, d);
    float outerAlpha = smoothstep(0.5 - w, 0.5 + w, d + vOutlineSize_ndc);
    float tFill = (outerAlpha > 1e-6) ? clamp(fillAlpha / outerAlpha, 0.0, 1.0) : 0.0;

    vec3 rgb = mix(vOutlineColor.rgb, vFillColor.rgb, tFill);
    float a  = outerAlpha * mix(vOutlineColor.a, vFillColor.a, tFill);

    vec4 bg   = premul(vBackgroundColor.rgb, vBackgroundColor.a);
    vec4 text = premul(rgb, a);

    outColor = text + bg * (1.0 - text.a);
}