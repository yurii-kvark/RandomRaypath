#version 450
layout(location=0) in vec2 vUV;
layout(location=1) flat in vec4 vColor;
layout(location = 2) flat in vec4 vBackgroundColor;
layout(location = 3) flat in vec2 vGridSize_ndc;
layout(location = 4) flat in vec2 vLineSize_ndc;
layout(location = 5) flat in vec4 vCameraTransform_ndc;
layout(location = 6) flat in vec4 vRectTransform_ndc;
layout(location = 7) flat in int  vSpaceBasis;
layout(location = 8) flat in int vApplyFragCam;

layout(location=0) out vec4 outColor;


float gridLineAlpha(vec2 coord, vec2 gridSize, vec2 lineSize)
{
    vec2 halfLine = 0.5 * lineSize;

    vec2 cell = mod(coord, gridSize);
    vec2 dist = min(cell, gridSize - cell);

    float aaX = length(vec2(dFdx(coord.x), dFdy(coord.x)));
    float aaY = length(vec2(dFdx(coord.y), dFdy(coord.y)));

    float ax = 1.0 - smoothstep(halfLine.x - aaX, halfLine.x + aaX, dist.x);
    float ay = 1.0 - smoothstep(halfLine.y - aaY, halfLine.y + aaY, dist.y);

    return max(ax, ay);
}

void main() {
    vec2 coord = vUV;

    if (vApplyFragCam != 0) {
        vec2  camPos   = vCameraTransform_ndc.xy;
        float camScale = max(vCameraTransform_ndc.z, 1e-6);

        if (vSpaceBasis == 0) {
            coord = coord / camScale + camPos;
        } else {
            coord = (coord - camPos) * camScale;
        }
    }

    float a = gridLineAlpha(coord, vGridSize_ndc, vLineSize_ndc);

    vec4 bg = vBackgroundColor;
    vec4 fg = vColor;

    vec4 mixed = mix(bg, fg, a);
    mixed.a = mix(bg.a, fg.a, a);

    outColor = mixed;
}