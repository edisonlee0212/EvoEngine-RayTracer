//
// Created by lllll on 11/15/2021.
//

#include "RayTracerCamera.hpp"
#include "Optix7.hpp"
#include "RayTracerLayer.hpp"
#include "IHandle.hpp"

#include "Application.hpp"
#include "Scene.hpp"
#include "TransformGraph.hpp"
using namespace EvoEngine;

void RayTracerCamera::Ready(const glm::vec3 &position, const glm::quat &rotation) {
    if (m_cameraProperties.m_frame.m_size != m_frameSize) {
        m_frameSize = glm::max(glm::uvec2(1, 1), m_frameSize);
        m_cameraProperties.Resize(m_frameSize);
        VkExtent3D extent;
        extent.width = m_frameSize.x;
        extent.depth = 1;
        extent.height = m_frameSize.y;
        m_renderTexture->Resize(extent);
    }

    m_cameraProperties.m_image = CudaModule::ImportRenderTexture(m_renderTexture);
    m_cameraProperties.Set(position, rotation);

}

bool RayTracerCamera::OnInspect(const std::shared_ptr<EditorLayer>& editorLayer) {
    if (GetScene()->IsEntityValid(GetOwner())) ImGui::Checkbox("Main Camera", &m_mainCamera);

    m_cameraProperties.OnInspect();
    m_rayProperties.OnInspect();
    if (ImGui::TreeNode("Debug")) {
        static float debugSacle = 0.25f;
        ImGui::DragFloat("Scale", &debugSacle, 0.01f, 0.1f, 1.0f);
        debugSacle = glm::clamp(debugSacle, 0.1f, 1.0f);
        ImGui::Image(m_renderTexture->GetColorImTextureId(),
                ImVec2(m_cameraProperties.m_frame.m_size.x * debugSacle,
                       m_cameraProperties.m_frame.m_size.y * debugSacle),
                ImVec2(0, 1),
                ImVec2(1, 0));
        ImGui::TreePop();
    }
    FileUtils::SaveFile("Export Screenshot", "Texture2D", {".png", ".jpg", ".hdr"},
                        [this](const std::filesystem::path &filePath) {
                            m_renderTexture->Save(filePath);
                        }, false);
    ImGui::Checkbox("Allow auto resize", &m_allowAutoResize);
    if (!m_allowAutoResize) {
        glm::ivec2 resolution = { m_frameSize.x, m_frameSize.y };
        if (ImGui::DragInt2("Resolution", &resolution.x, 1, 1, 4096))
        {
            m_frameSize = { resolution.x, resolution.y };
        }
    }
    return false;
}

void RayTracerCamera::OnCreate() {
    m_frameSize = glm::uvec2(512, 512);
    RenderTextureCreateInfo renderTextureCreateInfo{};
    renderTextureCreateInfo.m_extent.width = m_frameSize.x;
    renderTextureCreateInfo.m_extent.height = m_frameSize.y;
    renderTextureCreateInfo.m_extent.depth = 1;
    m_renderTexture = std::make_unique<RenderTexture>(renderTextureCreateInfo);
    Ready(glm::vec3(0), glm::vec3(0));
}

void RayTracerCamera::OnDestroy() {
    m_cameraProperties.m_frameBufferColor.Free();
    m_cameraProperties.m_frameBufferNormal.Free();
    m_cameraProperties.m_frameBufferAlbedo.Free();
    OPTIX_CHECK(optixDenoiserDestroy(m_cameraProperties.m_denoiser));
    m_cameraProperties.m_denoiserScratch.Free();
    m_cameraProperties.m_denoiserState.Free();
    m_cameraProperties.m_frameBufferColor.Free();
    m_cameraProperties.m_denoiserIntensity.Free();
}

void RayTracerCamera::Deserialize(const YAML::Node &in) {
    if (in["m_mainCamera"]) m_mainCamera = in["m_mainCamera"].as<bool>();

    if (in["m_allowAutoResize"]) m_allowAutoResize = in["m_allowAutoResize"].as<bool>();
    if (in["m_frameSize.x"]) m_frameSize.x = in["m_frameSize.x"].as<int>();
    if (in["m_frameSize.y"]) m_frameSize.y = in["m_frameSize.y"].as<int>();

    if (in["m_rayProperties.m_samples"]) m_rayProperties.m_samples = in["m_rayProperties.m_samples"].as<int>();
    if (in["m_rayProperties.m_bounces"]) m_rayProperties.m_bounces = in["m_rayProperties.m_bounces"].as<int>();

    if (in["m_cameraProperties.m_fov"]) m_cameraProperties.m_fov = in["m_cameraProperties.m_fov"].as<float>();
    if (in["m_cameraProperties.m_gamma"]) m_cameraProperties.m_gamma = in["m_cameraProperties.m_gamma"].as<float>();
    if (in["m_cameraProperties.m_accumulate"]) m_cameraProperties.m_accumulate = in["m_cameraProperties.m_accumulate"].as<bool>();
    if (in["m_cameraProperties.m_denoiserStrength"]) m_cameraProperties.m_denoiserStrength = in["m_cameraProperties.m_denoiserStrength"].as<float>();
    if (in["m_cameraProperties.m_focalLength"]) m_cameraProperties.m_focalLength = in["m_cameraProperties.m_focalLength"].as<float>();
    if (in["m_cameraProperties.m_aperture"]) m_cameraProperties.m_aperture = in["m_cameraProperties.m_aperture"].as<float>();
}

void RayTracerCamera::Serialize(YAML::Emitter &out) const {
    out << YAML::Key << "m_mainCamera" << YAML::Value << m_mainCamera;

    out << YAML::Key << "m_allowAutoResize" << YAML::Value << m_allowAutoResize;
    out << YAML::Key << "m_frameSize.x" << YAML::Value << m_frameSize.x;
    out << YAML::Key << "m_frameSize.y" << YAML::Value << m_frameSize.y;

    out << YAML::Key << "m_rayProperties.m_bounces" << YAML::Value << m_rayProperties.m_bounces;
    out << YAML::Key << "m_rayProperties.m_samples" << YAML::Value << m_rayProperties.m_samples;

    out << YAML::Key << "m_cameraProperties.m_fov" << YAML::Value << m_cameraProperties.m_fov;
    out << YAML::Key << "m_cameraProperties.m_gamma" << YAML::Value << m_cameraProperties.m_gamma;
    out << YAML::Key << "m_cameraProperties.m_accumulate" << YAML::Value << m_cameraProperties.m_accumulate;
    out << YAML::Key << "m_cameraProperties.m_denoiserStrength" << YAML::Value << m_cameraProperties.m_denoiserStrength;
    out << YAML::Key << "m_cameraProperties.m_focalLength" << YAML::Value << m_cameraProperties.m_focalLength;
    out << YAML::Key << "m_cameraProperties.m_aperture" << YAML::Value << m_cameraProperties.m_aperture;
}

RayTracerCamera &RayTracerCamera::operator=(const RayTracerCamera &source) {
    m_mainCamera = source.m_mainCamera;

    m_cameraProperties.m_accumulate = source.m_cameraProperties.m_accumulate;
    m_cameraProperties.m_fov = source.m_cameraProperties.m_fov;
    m_cameraProperties.m_inverseProjectionView = source.m_cameraProperties.m_inverseProjectionView;
    m_cameraProperties.m_horizontal = source.m_cameraProperties.m_horizontal;
    m_cameraProperties.m_outputType = source.m_cameraProperties.m_outputType;
    m_cameraProperties.m_gamma = source.m_cameraProperties.m_gamma;
    m_cameraProperties.m_denoiserStrength = source.m_cameraProperties.m_denoiserStrength;
    m_cameraProperties.m_aperture = source.m_cameraProperties.m_aperture;
    m_cameraProperties.m_focalLength = source.m_cameraProperties.m_focalLength;
    m_cameraProperties.m_modified = true;

    m_cameraProperties.m_frame.m_size = glm::vec2(0, 0);
    m_rayProperties = source.m_rayProperties;
    m_frameSize = source.m_frameSize;
    m_allowAutoResize = source.m_allowAutoResize;
    m_rendered = false;
    return *this;
}

void RayTracerCamera::Render() {
    if (!CudaModule::GetRayTracer()->m_instances.empty()) {
        auto globalTransform = GetScene()->GetDataComponent<GlobalTransform>(GetOwner()).m_value;
        Ready(globalTransform[3], glm::quat_cast(globalTransform));
        m_rendered = CudaModule::GetRayTracer()->RenderToCamera(
                Application::GetLayer<RayTracerLayer>()->m_environmentProperties,
                m_cameraProperties,
                m_rayProperties);
    }
}

void RayTracerCamera::Render(const RayProperties &rayProperties) {
    if (!CudaModule::GetRayTracer()->m_instances.empty()) {
        auto globalTransform = GetScene()->GetDataComponent<GlobalTransform>(GetOwner()).m_value;
        Ready(globalTransform[3], glm::quat_cast(globalTransform));
        m_rendered = CudaModule::GetRayTracer()->RenderToCamera(
                Application::GetLayer<RayTracerLayer>()->m_environmentProperties,
                m_cameraProperties,
                rayProperties);
    }
}

void RayTracerCamera::Render(const RayProperties &rayProperties, const EnvironmentProperties &environmentProperties) {
    if (!CudaModule::GetRayTracer()->m_instances.empty()) {
        auto globalTransform = GetScene()->GetDataComponent<GlobalTransform>(GetOwner()).m_value;
        Ready(globalTransform[3], glm::quat_cast(globalTransform));
        m_rendered = CudaModule::GetRayTracer()->RenderToCamera(
                environmentProperties,
                m_cameraProperties,
                rayProperties);
    }
}

void RayTracerCamera::SetFov(float value) {
    m_cameraProperties.SetFov(value);
}

void RayTracerCamera::SetAperture(float value) {
    m_cameraProperties.SetAperture(value);
}

void RayTracerCamera::SetFocalLength(float value) {
    m_cameraProperties.SetFocalLength(value);
}

void RayTracerCamera::SetDenoiserStrength(float value) {
    m_cameraProperties.SetDenoiserStrength(value);
}

void RayTracerCamera::SetGamma(float value) {
    m_cameraProperties.SetGamma(value);
}

void RayTracerCamera::SetOutputType(OutputType value) {
    m_cameraProperties.SetOutputType(value);
}

void RayTracerCamera::SetAccumulate(bool value) {
    m_cameraProperties.m_accumulate = value;
}

void RayTracerCamera::SetSkybox(const std::shared_ptr<Cubemap>& cubemap)
{
    m_skybox = cubemap;
    auto cudaImage = CudaModule::ImportCubemap(cubemap);
    m_cameraProperties.SetSkybox(cudaImage);
}

void RayTracerCamera::SetMainCamera(bool value) {
    if (GetScene()->IsEntityValid(GetOwner())) m_mainCamera = value;
}

void RayTracerCamera::SetMaxDistance(float value) {
    m_cameraProperties.SetMaxDistance(value);
}

glm::mat4 RayTracerCamera::GetProjection() const {
    return glm::perspective(glm::radians(m_cameraProperties.m_fov * 0.5f), (float)m_frameSize.x / m_frameSize.y, 0.0001f, 100.0f);
}
