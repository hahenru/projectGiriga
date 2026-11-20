#version 450

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 0) uniform sampler2D uTexture;

layout(push_constant) uniform UIPush {
    vec4 rect;
    vec4 color;
    vec4 params; // x = renderMode (0=純色, 1=字體單通道R, 2=RGBA貼圖)
} pc;

void main() {
    int mode = int(pc.params.x + 0.5);
    if (mode == 1) {
        float alpha = texture(uTexture, vTexCoord).r;
        FragColor = vec4(pc.color.rgb, alpha * pc.color.a);
    } else if (mode == 2) {
        FragColor = texture(uTexture, vTexCoord) * pc.color;
    } else {
        FragColor = pc.color;
    }
}