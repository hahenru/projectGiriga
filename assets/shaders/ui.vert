#version 450

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

layout(push_constant) uniform UIPush {
    vec4 rect;    // xy = 左上角像素座標 (x, y), zw = 寬高 (width, height)
    vec4 color;   // rgba
    vec4 params;  // x = renderMode (0=純色, 1=字體單通道R, 2=RGBA貼圖), y = time, zw = screenSize (w, h)
} pc;

layout(location = 0) out vec2 vTexCoord;

void main() {
    vec2 pixelPos = pc.rect.xy + aPos * pc.rect.zw;
    // 轉換至 Vulkan NDC：左上 (-1, -1)，右下 (+1, +1)
    vec2 ndc = (pixelPos / pc.params.zw) * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
    vTexCoord = aTexCoord;
}