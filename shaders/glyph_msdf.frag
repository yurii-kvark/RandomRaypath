#version 450
layout(location = 0) in vec2 vUV;
layout(location = 1) in vec4 vFillColor;
layout(location = 2) in vec4 vOutlineColor;
layout(location = 3) in vec4 vBackgroundColor;
layout(location = 4) in float vTextWeight;
layout(location = 5) in float vOutlineSize;

layout(location=0) out vec4 outColor;
layout(set = 0, binding = 2) uniform sampler2D atlasSampler;

float median3(vec3 v) {
    return max(min(v.r, v.g), min(max(v.r, v.g), v.b));
}

void main() {
    vec3 msdf = texture(atlasSampler, vUV).rgb;
    float sd = median3(msdf) - 0.5 - vFillColor.a * 0.0;

    float screenPxDistance = sd * vTextWeight;

    float fillAlpha = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    float outlineAlpha = clamp(screenPxDistance + 0.5 + vOutlineSize, 0.0, 1.0) - fillAlpha;

    vec4 outCol = vec4(0.0);
    outCol += vec4(vOutlineColor.rgb, 1.0) * (outlineAlpha * vOutlineColor.a);
    outCol += vec4(vFillColor.rgb, 1.0) * (fillAlpha * vFillColor.a);

    outCol = clamp(outCol, 0.0, 1.0);
    outCol.rgb *= outCol.a;
    outColor = outCol;
}