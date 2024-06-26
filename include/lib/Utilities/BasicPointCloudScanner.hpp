#pragma once

#include "evoengine-pch.hpp"
#include "Application.hpp"

#include "CUDAModule.hpp"
#include "IPrivateComponent.hpp"
#include "PointCloud.hpp"
using namespace EvoEngine;
namespace EvoEngine {
    class BasicPointCloudScanner : public IPrivateComponent {
    public:
        float m_rotateAngle = 0.0f;
        glm::vec2 m_size = glm::vec2(8, 4);
        glm::vec2 m_distance = glm::vec2(0.02f, 0.02f);

        std::vector<uint64_t> m_handles;
        std::vector<glm::vec3> m_points;
        std::vector<glm::vec3> m_pointColors;
        void ConstructPointCloud(std::shared_ptr<PointCloud> pointCloud);

        void Scan();

        bool OnInspect(const std::shared_ptr<EditorLayer>& editorLayer) override;

        void Serialize(YAML::Emitter &out) const override;

        void Deserialize(const YAML::Node &in) override;
    };
}