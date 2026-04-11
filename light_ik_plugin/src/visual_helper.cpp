#include "visual_helper.h"

#include "light_ik/light_ik.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/quaternion.hpp>

#include <godot_cpp/classes/immediate_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>

#include <glm/glm.hpp>
#include <glm/ext/scalar_constants.hpp>

#include <array>

namespace godot
{

inline Vector3 grad2rad(const Vector3 angles)
{
    return angles * (Math_PI / 180.);
}

VisualHelper::VisualHelper()
{
    m_helpersGeometry.instantiate();
    set_cast_shadows_setting(GeometryInstance3D::ShadowCastingSetting::SHADOW_CASTING_SETTING_OFF);

    MakeMaterial(m_targetLineMaterial,      Color::hex(0xFFaa55FF));
    MakeMaterial(m_chainLineMaterial,       Color::hex(0x22FF22FF));
    MakeMaterial(m_startMarkerMaterial,     Color::hex(0xFF22FFFF));
    MakeMaterial(m_endMarkerMaterial,       Color::hex(0x22FF22FF));
    MakeMaterial(m_targetMarkerMaterial,    Color::hex(0xFF2222FF));

    MakeMaterial(m_jointMarkerMaterials[0], Color::hex(0xFF2222FF));
    MakeMaterial(m_jointMarkerMaterials[1], Color::hex(0x22FF22FF));
    MakeMaterial(m_jointMarkerMaterials[2], Color::hex(0x2222FFFF));
}

VisualHelper::~VisualHelper()
{
}

void VisualHelper::_ready()
{
    set_mesh(m_helpersGeometry);
}

void VisualHelper::_process(double delta)
{
    if (!is_node_ready())
    {
        return;
    }
    m_helpersGeometry->clear_surfaces();

    if (m_enabled)
    {
        // visualize skeleton structure
        for (const auto& chain : m_chainVisualData)
        {
            assert(chain.chain.size() > 1);
            // Draw the chain
            DrawLine(chain.chain, m_chainLineMaterial);

            // Add major chain markers to the beginning and the end of the chain
            DrawStartMarker(chain.start);
            DrawEndMarker(chain.chain.back(), chain.chain[chain.chain.size() - 2]);

            // If chain is targeting to another bone, show target marker
            DrawTargetMarker(Transform3D(chain.chain.back().basis, chain.target));

            // Draw line between the tip and the target of the chain
            DrawDashedLine(chain.chain.back().origin, chain.target, m_targetLineMaterial);
        }

        // visualize joint constraints
        for (const auto& constraint : m_boneConstraintData)
        {
            float radius = constraint.flexibility * m_radiusJoint;
            Vector3 minAngles = grad2rad(constraint.minAngles);
            Vector3 maxAngles = grad2rad(constraint.maxAngles);

            DrawArc(constraint.position, radius, minAngles.x, maxAngles.x, Vector3(1, 0, 0), Vector3(0, 1, 0), m_jointMarkerMaterials[0]);
            DrawArc(constraint.position, radius, minAngles.y, maxAngles.y, Vector3(0, 1, 0), Vector3(0, 0, 1), m_jointMarkerMaterials[1]);
            DrawArc(constraint.position, radius, minAngles.z, maxAngles.z, Vector3(0, 0, 1), Vector3(0, 1, 0), m_jointMarkerMaterials[2]);
        }
    }
}

void VisualHelper::AddChain(const ChainVisualData& visualData)
{
    m_chainVisualData.emplace_back(visualData);
}

void VisualHelper::AddBoneConstraint(const BoneInfo& constraintData) 
{
    m_boneConstraintData.emplace_back(constraintData);
}

void VisualHelper::DrawStartMarker(const Transform3D& point)
{
    static std::vector<Vector3> circularVertices;
    if (circularVertices.empty())
    {
        for (size_t i = 0; i < m_pointsPerMarker; ++i)
        {
            circularVertices.emplace_back(Vector3{sinf(2 * Math_PI * i/(float)m_pointsPerMarker), 0, cosf(2 * Math_PI * i/(float)m_pointsPerMarker)});
        }
        circularVertices.push_back(circularVertices.front());
    }

    DrawLineShape(circularVertices, point, m_radiusRoot, m_startMarkerMaterial);
}

void VisualHelper::DrawEndMarker(const Transform3D& position, const Transform3D& orientation)
{
    static std::vector<Vector3> arrowVertices = {Vector3{0, -1, 0}, {0.5, -1, 0}, {0, 0, 0}, {-0.5, -1, 0}, {0, -1, 0}, {0, -1, 0.5}, {0, 0, 0}, {0, -1, -0.5}, {0,-1,0}};

    //the transform of the tip consists of position of the last joint and orientation of pre-last bone
    Transform3D tipPosition = Transform3D(orientation.basis, position.origin);
    DrawLineShape(arrowVertices, tipPosition, m_radiusRoot, m_endMarkerMaterial);
}

void VisualHelper::DrawTargetMarker(const Transform3D& point)
{
    static std::vector<Vector3> rectangleVertices = {Vector3{0, 0, 0}, {1, 0, 0}, {0, 0, 0}, {-1, 0, 0}, 
                                                            {0, 0, 0}, {0, 1, 0}, {0, 0, 0}, {0, -1, 0},
                                                            {0, 0, 0}, {0, 0, 1}, {0, 0, 0}, {0, 0, -1}};

    DrawLineShape(rectangleVertices, point, m_radiusRoot/2.0, m_targetMarkerMaterial);
}

void VisualHelper::DrawDashedLine(const Vector3& from, const Vector3& to, Ref<StandardMaterial3D>& material)
{
    // setup the material
    Vector3 direction = to - from;
    float distance = direction.length();
    direction.normalize();

    // draw the dashed line, keep in mind that number of dashes cannot be infinite, so for the sake of performance, give it an upper limit
    constexpr size_t DashedLineMaxSteps = 25;
    constexpr float DashedLineDashSize = 0.2f;
    size_t steps = glm::min(DashedLineMaxSteps, (size_t)(distance / DashedLineDashSize + 1));
    float dashSize = distance / (steps - 0.5f);
    m_helpersGeometry->surface_begin(Mesh::PrimitiveType::PRIMITIVE_LINES, material);
    for (size_t i = 0; i < steps; ++i)
    {
        m_helpersGeometry->surface_add_vertex(from + direction * dashSize * i);
        m_helpersGeometry->surface_add_vertex(from + direction * glm::min(dashSize * i + dashSize/2.f, distance));
    }
    m_helpersGeometry->surface_end();
}

void VisualHelper::DrawLine(const std::vector<Transform3D>& line, Ref<StandardMaterial3D>& material)
{
    m_helpersGeometry->surface_begin(Mesh::PrimitiveType::PRIMITIVE_LINE_STRIP, material);
    for (const auto& vertex : line)
    {
        m_helpersGeometry->surface_add_vertex(vertex.origin);
    }
    m_helpersGeometry->surface_end();
}

void VisualHelper::DrawLineShape(const std::vector<Vector3>& shape, const Transform3D& position, float scale, Ref<StandardMaterial3D>& material)
{
    m_helpersGeometry->surface_begin(Mesh::PrimitiveType::PRIMITIVE_LINE_STRIP, material);
    for (const auto& vertex : shape)
    {
        m_helpersGeometry->surface_add_vertex(position.xform(scale * vertex));
    }
    m_helpersGeometry->surface_end();
}

void VisualHelper::DrawArc(const Transform3D& center, float radius, float minAngle, float maxAngle, Vector3 axis, Vector3 direction, Ref<StandardMaterial3D>& material)
{
    Vector3 minAxis         = direction.rotated(axis, minAngle);
    float step              = (maxAngle - minAngle)/m_pointsPerMarker;
    std::vector<Vector3> arc = {Vector3(0,0,0), minAxis};
    for (size_t i = 0; i < m_pointsPerMarker; ++i)
    {
        Vector3 point = minAxis.rotated(axis, step * i);
        arc.emplace_back(point);
        arc.emplace_back(Vector3(0,0,0));
    }

    DrawShape(arc, center, radius, material, Mesh::PrimitiveType::PRIMITIVE_TRIANGLE_STRIP);
}

void VisualHelper::DrawShape(const std::vector<Vector3>& shape, const Transform3D& position, float scale, Ref<StandardMaterial3D>& material, Mesh::PrimitiveType primitive)
{
    m_helpersGeometry->surface_begin(primitive, material);
    for (const auto& vertex : shape)
    {
        m_helpersGeometry->surface_add_vertex(position.xform(scale * vertex));
    }
    m_helpersGeometry->surface_end();
}

void VisualHelper::MakeMaterial(Ref<StandardMaterial3D>& material, Color color)
{
    material.instantiate();
    material->set_albedo(color);
    material->set_shading_mode(BaseMaterial3D::ShadingMode::SHADING_MODE_UNSHADED);
    material->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
}

}