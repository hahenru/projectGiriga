// src/Renderer/UIRenderer.cpp
#include <Renderer/UIRenderer.h>
#include <Renderer/VulkanContext.h>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <array>

UIRenderer::UIRenderer(VulkanContext& vkCtx, VkFormat colorFormat, float screenWidth, float screenHeight)
    : m_vkCtx(vkCtx),
      m_quadMesh(vkCtx),
      m_screenWidth(screenWidth),
      m_screenHeight(screenHeight) {

    createDescriptorResources();
    createPipelines(colorFormat);

    // 建立 1x1 白色預設貼圖，供純色 Quad 綁定滿足 Descriptor 驗證
    const uint32_t whitePixel = 0xFFFFFFFF;
    m_whiteTexture = VulkanTexture(m_vkCtx, m_descriptorPool, m_descriptorSetLayout, &whitePixel, 1, 1, 4);
}

UIRenderer::~UIRenderer() {
    m_vkCtx.waitIdle();
	m_textCache.clear();
    m_textureCache.clear();
    m_whiteTexture = VulkanTexture();

    VkDevice dev = m_vkCtx.getDevice();
    if (m_consoleBgPipeline != VK_NULL_HANDLE) vkDestroyPipeline(dev, m_consoleBgPipeline, nullptr);
    if (m_uiPipeline != VK_NULL_HANDLE) vkDestroyPipeline(dev, m_uiPipeline, nullptr);
    if (m_pipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(dev, m_pipelineLayout, nullptr);
    if (m_descriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(dev, m_descriptorPool, nullptr);
    if (m_descriptorSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(dev, m_descriptorSetLayout, nullptr);
}

void UIRenderer::createDescriptorResources() {
    VkDevice dev = m_vkCtx.getDevice();

    VkDescriptorSetLayoutBinding samplerBinding{};
    samplerBinding.binding = 0;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.descriptorCount = 1;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerBinding;

    if (vkCreateDescriptorSetLayout(dev, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("[UIRenderer] Failed to create VkDescriptorSetLayout");
    }

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 2048;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 2048;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;

    if (vkCreateDescriptorPool(dev, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("[UIRenderer] Failed to create VkDescriptorPool");
    }
}

VkShaderModule UIRenderer::loadShaderModule(const std::string& spvPath) {
    std::ifstream file(spvPath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("[UIRenderer] Failed to open SPIR-V file: " + spvPath);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(fileSize));

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = fileSize;
    createInfo.pCode = buffer.data();

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(m_vkCtx.getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("[UIRenderer] Failed to create VkShaderModule: " + spvPath);
    }
    return shaderModule;
}

void UIRenderer::createPipelines(VkFormat colorFormat) {
    VkDevice dev = m_vkCtx.getDevice();

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(UIPushConstants);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 1;
    layoutInfo.pSetLayouts = &m_descriptorSetLayout;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushRange;

    if (vkCreatePipelineLayout(dev, &layoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("[UIRenderer] Failed to create VkPipelineLayout");
    }

    VkShaderModule uiVert = loadShaderModule("./assets/shaders/ui.vert.spv");
    VkShaderModule uiFrag = loadShaderModule("./assets/shaders/ui.frag.spv");
    m_uiPipeline = buildGraphicsPipeline(uiVert, uiFrag, colorFormat);
    vkDestroyShaderModule(dev, uiFrag, nullptr);

    // Console 背景特效共用同一份 ui.vert.spv
    VkShaderModule consoleFrag = loadShaderModule("./assets/shaders/console_bg.frag.spv");
    m_consoleBgPipeline = buildGraphicsPipeline(uiVert, consoleFrag, colorFormat);
    vkDestroyShaderModule(dev, consoleFrag, nullptr);
    vkDestroyShaderModule(dev, uiVert, nullptr);
}

VkPipeline UIRenderer::buildGraphicsPipeline(VkShaderModule vertModule, VkShaderModule fragModule, VkFormat colorFormat) {
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule;
    stages[0].pName = "main";

    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule;
    stages[1].pName = "main";

    // 頂點格式：vec2 aPos (offset 0), vec2 aTexCoord (offset 8)，stride = 16 bytes
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = 4 * sizeof(float);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 2> attrDescs{};
    attrDescs[0] = { 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 };
    attrDescs[1] = { 1, 0, VK_FORMAT_R32G32_SFLOAT, 2 * sizeof(float) };

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &bindingDesc;
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size());
    vertexInput.pVertexAttributeDescriptions = attrDescs.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.blendEnable = VK_TRUE;
    blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &blendAttachment;

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // Vulkan 1.3 Dynamic Rendering Attachment 宣告
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &colorFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;

    VkPipeline pipeline = VK_NULL_HANDLE;
    if (vkCreateGraphicsPipelines(m_vkCtx.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("[UIRenderer] Failed to create Graphics Pipeline");
    }
    return pipeline;
}

void UIRenderer::setScreenSize(float width, float height) {
    m_screenWidth = width;
    m_screenHeight = height;
}

void UIRenderer::begin(VkCommandBuffer cmd) {
    m_currentCmd = cmd;
    if (m_currentCmd == VK_NULL_HANDLE) return;

    vkCmdBindPipeline(m_currentCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_uiPipeline);

    VkViewport viewport{ 0.0f, 0.0f, m_screenWidth, m_screenHeight, 0.0f, 1.0f };
    vkCmdSetViewport(m_currentCmd, 0, 1, &viewport);
    resetFullScreenScissor();
}

void UIRenderer::end() {
    while (!m_scissorStack.empty()) {
        popClipRect();
    }
    m_currentCmd = VK_NULL_HANDLE;
}

void UIRenderer::pushClipRect(float x, float y, float width, float height) {
    ScissorRect newRect{ x, y, width, height };

    if (!m_scissorStack.empty()) {
        const ScissorRect& cur = m_scissorStack.back();
        float minX = std::max(cur.x, newRect.x);
        float minY = std::max(cur.y, newRect.y);
        float maxX = std::min(cur.x + cur.width, newRect.x + newRect.width);
        float maxY = std::min(cur.y + cur.height, newRect.y + newRect.height);

        newRect.x = minX;
        newRect.y = minY;
        newRect.width = std::max(0.0f, maxX - minX);
        newRect.height = std::max(0.0f, maxY - minY);
    }

    m_scissorStack.push_back(newRect);
    applyScissor(newRect);
}

void UIRenderer::popClipRect() {
    if (m_scissorStack.empty()) return;
    m_scissorStack.pop_back();

    if (m_scissorStack.empty()) {
        resetFullScreenScissor();
    } else {
        applyScissor(m_scissorStack.back());
    }
}

void UIRenderer::applyScissor(const ScissorRect& rect) {
    if (m_currentCmd == VK_NULL_HANDLE) return;
    // Vulkan 原點就在左上角，不需像 OpenGL 翻轉 Y 軸
    int32_t sx = std::max(0, static_cast<int32_t>(rect.x));
    int32_t sy = std::max(0, static_cast<int32_t>(rect.y));
    uint32_t sw = static_cast<uint32_t>(std::max(0.0f, rect.width));
    uint32_t sh = static_cast<uint32_t>(std::max(0.0f, rect.height));

    VkRect2D scissor{ { sx, sy }, { sw, sh } };
    vkCmdSetScissor(m_currentCmd, 0, 1, &scissor);
}

void UIRenderer::resetFullScreenScissor() {
    if (m_currentCmd == VK_NULL_HANDLE) return;
    VkRect2D full{
        { 0, 0 },
        { static_cast<uint32_t>(std::max(1.0f, m_screenWidth)),
          static_cast<uint32_t>(std::max(1.0f, m_screenHeight)) }
    };
    vkCmdSetScissor(m_currentCmd, 0, 1, &full);
}

void UIRenderer::drawQuad(float x, float y, float width, float height,
                          float r, float g, float b, float a) {
    if (m_currentCmd == VK_NULL_HANDLE) return;

    VkDescriptorSet descSet = m_whiteTexture.getDescriptorSet();
    vkCmdBindDescriptorSets(m_currentCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &descSet, 0, nullptr);

    UIPushConstants pc{};
    pc.rect = { x, y, width, height };
    pc.color = { r, g, b, a };
    pc.params = { 0.0f, 0.0f, m_screenWidth, m_screenHeight };

    vkCmdPushConstants(m_currentCmd, m_pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(UIPushConstants), &pc);
    m_quadMesh.draw(m_currentCmd);
}

void UIRenderer::drawTexture(float x, float y, float width, float height,
                             const VulkanTexture& texture,
                             float r, float g, float b, float a, int renderMode) {
    if (m_currentCmd == VK_NULL_HANDLE || !texture.isValid()) return;

    VkDescriptorSet descSet = texture.getDescriptorSet();
    vkCmdBindDescriptorSets(m_currentCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &descSet, 0, nullptr);

    UIPushConstants pc{};
    pc.rect = { x, y, width, height };
    pc.color = { r, g, b, a };
    pc.params = { static_cast<float>(renderMode), 0.0f, m_screenWidth, m_screenHeight };

    vkCmdPushConstants(m_currentCmd, m_pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(UIPushConstants), &pc);
    m_quadMesh.draw(m_currentCmd);
}

const VulkanTexture* UIRenderer::getOrUploadTexture(const ImageResource& image) {
    if (!image.isLoaded()) return nullptr;

    auto& entry = m_textureCache[&image];
    const uint64_t currentRev = image.getRevision();

    if (!entry.texture.isValid() || entry.uploadedRevision != currentRev) {
        entry.texture = VulkanTexture(
            m_vkCtx, m_descriptorPool, m_descriptorSetLayout,
            image.getPixels().data(), image.getWidth(), image.getHeight(), 4
        );
        entry.uploadedRevision = currentRev;
    }

    return &entry.texture;
}

void UIRenderer::drawImage(float x, float y, float width, float height,
                           const ImageResource& image,
                           float r, float g, float b, float a) {
    const VulkanTexture* tex = getOrUploadTexture(image);
    if (!tex) return;
    drawTexture(x, y, width, height, *tex, r, g, b, a, 2);
}

void UIRenderer::drawConsoleBgQuad(float x, float y, float width, float height, float elapsedTime) {
    if (m_currentCmd == VK_NULL_HANDLE) return;

    vkCmdBindPipeline(m_currentCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_consoleBgPipeline);

    VkDescriptorSet descSet = m_whiteTexture.getDescriptorSet();
    vkCmdBindDescriptorSets(m_currentCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &descSet, 0, nullptr);

    UIPushConstants pc{};
    pc.rect = { x, y, width, height };
    pc.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    pc.params = { 0.0f, elapsedTime, m_screenWidth, m_screenHeight };

    vkCmdPushConstants(m_currentCmd, m_pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(UIPushConstants), &pc);
    m_quadMesh.draw(m_currentCmd);

    // 繪製完畢後切回標準 UI 管線
    vkCmdBindPipeline(m_currentCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_uiPipeline);
}

void UIRenderer::drawText(float x, float y, float width, float height,
                          const TextTexture& textTex, const float color[4]) {
    if (!textTex.isValid() || !color) return;

    auto& entry = m_textCache[&textTex];
    const uint64_t currentRev = textTex.getRevision();

    if (!entry.texture.isValid() || entry.uploadedRevision != currentRev) {
        entry.texture = VulkanTexture(
            m_vkCtx, m_descriptorPool, m_descriptorSetLayout,
            textTex.getBitmap().data(), textTex.getWidth(), textTex.getHeight(), 1
        );
        entry.uploadedRevision = currentRev;
    }

    // renderMode = 1 走 ui.frag 的單通道 R8 Alpha 混合分支
    drawTexture(x, y, width, height, entry.texture, color[0], color[1], color[2], color[3], 1);
}