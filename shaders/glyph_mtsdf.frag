
#version 450
layout(location = 0) in vec2 vUV;
layout(location = 1) flat in int vDisplayEnable;
layout(location = 2) flat in vec4 vFillColor;
layout(location = 3) flat in vec4 vOutlineColor;
layout(location = 4) flat in vec4 vBackgroundColor;
layout(location = 5) flat in float vOutlineSizePx;

layout(location = 0) out vec4 outColor;
layout(set = 0, binding = 2) uniform sampler2D atlasSampler;

float median3(vec3 v) {
    return max(min(v.r, v.g), min(max(v.r, v.g), v.b));
}

vec4 premul(vec3 rgb, float a) { return vec4(rgb * a, a); }

float screenPxRange(vec2 uv) {
    float pxRange = 8.; // fixed
    vec2 unitRange = vec2(pxRange) / vec2(textureSize(atlasSampler, 0));

    vec2 dx = dFdx(uv);
    vec2 dy = dFdy(uv);

    vec2 screenTexSize = inversesqrt(dx*dx + dy*dy);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

void main() {
    if (vDisplayEnable == 0) { outColor = vec4(0.0); return; }

    vec4 t = texture(atlasSampler, vUV);

    float sd_msdf = median3(t.rgb) - 0.5;
    float sd_sdf  = t.a - 0.5;

    float spr = screenPxRange(vUV);

    float pxDistFill  = spr * sd_msdf;
    float pxDistOuter = spr * sd_sdf + vOutlineSizePx;

    float fillAlpha  = clamp(pxDistFill  + 0.5, 0.0, 1.0);
    float outerAlpha = clamp(pxDistOuter + 0.5, 0.0, 1.0);

    float tFill = (outerAlpha > 1e-6) ? clamp(fillAlpha / outerAlpha, 0.0, 1.0) : 0.0;

    vec3 rgb = mix(vOutlineColor.rgb, vFillColor.rgb, tFill);
    float a  = outerAlpha * mix(vOutlineColor.a, vFillColor.a, tFill);

    vec4 bg   = premul(vBackgroundColor.rgb, vBackgroundColor.a);
    vec4 text = premul(rgb, a);

    outColor = text + bg * (1.0 - text.a);
}