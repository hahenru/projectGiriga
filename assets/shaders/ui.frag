#version 460 core

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 TexCoord;

// 明確綁定 Uniform 與 Texture Unit
layout (location = 2) uniform vec4 uiColor;
layout (location = 3) uniform int renderMode; // 0 = 純色, 1 = 字體單通道 Alpha, 2 = 全彩 RGBA 貼圖
layout (binding = 0) uniform sampler2D textTexture; // 直接綁定 Texture Unit 0

void main() {
    if (renderMode == 1) {
        float alpha = texture(textTexture, TexCoord).r;
        FragColor = vec4(uiColor.rgb, alpha * uiColor.a);
    } else if (renderMode == 2) {
        FragColor = texture(textTexture, TexCoord) * uiColor;
    } else {
        FragColor = uiColor;
    }
}
