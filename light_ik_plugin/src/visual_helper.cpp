#include "visual_helper.h"

#include "light_ik/light_ik.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/immediate_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>

#include <glm/glm.hpp>
#include <glm/ext/scalar_constants.inl>

#include <array>

namespace godot
{

VisualHelper::VisualHelper()
    : m_helpersGeometry(memnew(ImmediateMesh))
    
{
    set_cast_shadows_setting(GeometryInstance3D::ShadowCastingSetting::SHADOW_CASTING_SETTING_OFF);
    set_mesh(m_helpersGeometry);
}

VisualHelper::~VisualHelper()
{
}

void VisualHelper::SetConstraintsInfo(const TypedArray<JointConstraints>& info, const std::vector<Transform3D>& bones)
{
    m_boneInfoArray.clear();
    if (0 == info.size() || info.size() != bones.size() - 1)
    {
        return;
    }

    m_boneInfoArray.resize(info.size());
     // fill the bone constraints information from the constraints array size
    for (size_t c = 0; c < info.size(); ++c)
    {
        JointConstraints* constraint = Object::cast_to<JointConstraints>(info[c]);
        BoneInfo& info = m_boneInfoArray[c];

        info.position       = bones[c];
        info.constrained    = (constraint != nullptr);
        // TODO: add partial constraints
        if (constraint)
        {
            info.minAngles  = constraint->get_min_angle() / 180.f * glm::pi<float>();
            info.maxAngles  = constraint->get_max_angle() / 180.f * glm::pi<float>();
            info.stiffness  = constraint->get_stiffness();
        }
        else
        {
            info.minAngles  = {-glm::pi<float>(), -glm::pi<float>(), -glm::pi<float>()};
            info.maxAngles  = {glm::pi<float>(), glm::pi<float>(), glm::pi<float>()};
            info.stiffness  = 0;
        }
    }
    // Add tip bone
    auto& tip = m_boneInfoArray.emplace_back(BoneInfo());
    tip.position = bones.back();

    m_updateRequired = true;
}

void VisualHelper::SetTargetPosition(const Transform3D& skeletonOrigin, const Transform3D& target)
{
    m_updateRequired    = m_updateRequired || m_targetPosition != target || m_skeletonPosition != skeletonOrigin;
    m_targetPosition    = target;
    m_skeletonPosition  = skeletonOrigin;
}

void VisualHelper::_ready()
{
    
}

void VisualHelper::_process(double delta)
{
    if (!is_node_ready() || !m_updateRequired)
    {
        return;
    }
    // update data only if something really changed.
    m_updateRequired = false;
    m_helpersGeometry->clear_surfaces();
    
    AddRootBoneMarker();
    for (const auto& constraint : m_boneInfoArray)
    {
        AddConstraint(constraint);
    }
    AddTipMarker();
    AddDirectionLine();
}

void VisualHelper::AddRootBoneMarker()
{
    static std::vector<Vector3> rectangleVertices = {Vector3{1, 0, 1}, {1, 0, -1}, {-1, 0, -1}, {-1, 0, 1}, {1, 0, 1}};
    LineStripFromVector(rectangleVertices, m_boneInfoArray.front().position, Color::hex(0xFF22FFFF));
}

void VisualHelper::AddTipMarker()
{
    static std::vector<Vector3> arrowVertices = {Vector3{0, 0, 0}, {0.5, 0, 0}, {0, 1, 0}, {-0.5, 0, 0}, {0, 0, 0}, {0, 0, 0.5}, {0, 1, 0}, {0, 0, -0.5}, {0,0,0}};
    assert(m_boneInfoArray.size() > 1);
    //the transform of the tip consists of position of the last joint and orientation of pre-last bone
    Transform3D tipPosition = Transform3D(m_boneInfoArray[m_boneInfoArray.size() - 2].position.basis, m_boneInfoArray.back().position.origin);
    LineStripFromVector(arrowVertices, tipPosition, Color::hex(0x22FF22FF));
}

void VisualHelper::LineStripFromVector(const std::vector<Vector3>& strip, const Transform3D& position, Color color)
{
    Ref<StandardMaterial3D> material = memnew(StandardMaterial3D);
    material->set_albedo(color);
    material->set_shading_mode(BaseMaterial3D::ShadingMode::SHADING_MODE_UNSHADED);

    m_helpersGeometry->surface_begin(Mesh::PrimitiveType::PRIMITIVE_LINE_STRIP, material);
    for (const auto& vertex : strip)
    {
        m_helpersGeometry->surface_add_vertex(position.xform(m_radiusRoot * vertex));
    }
    m_helpersGeometry->surface_end();
}

void VisualHelper::AddConstraint(const BoneInfo& constraint)
{
    AddConstraintMarker(constraint.minAngles.x, constraint.maxAngles.x, constraint.position, {0,0,0}, Color::hex(0xFF0000FF));
    AddConstraintMarker(constraint.minAngles.y, constraint.maxAngles.y, constraint.position, {0,1,0}, Color::hex(0x55FF55FF));
    AddConstraintMarker(constraint.minAngles.z, constraint.maxAngles.z, constraint.position, {1,0,0}, Color::hex(0x0000FFFF));
}

void VisualHelper::AddConstraintMarker(float minAngle, float maxAngle, const Transform3D& position, const Vector3& axis, Color color)
{
    Ref<StandardMaterial3D> material = memnew(StandardMaterial3D);
    material->set_albedo(color);
    material->set_shading_mode(BaseMaterial3D::ShadingMode::SHADING_MODE_UNSHADED);
    m_helpersGeometry->surface_begin(Mesh::PrimitiveType::PRIMITIVE_LINE_STRIP, material);

    bool needRotation = axis.length_squared() > LightIK::DELTA;

    for (size_t i = 0; i < m_pointsPerMarker; ++i)
    {
        float t         = i / (m_pointsPerMarker - 1.f);
        float pos       = minAngle * t + maxAngle * (1 - t);
        Vector3 vertex  = m_radiusJoint * Vector3{glm::sin(pos), glm::cos(pos), 0.f};
        if (needRotation)
        {
            vertex.rotate(axis, glm::pi<float>()/2.f);
        }
        vertex = position.xform(vertex);
        m_helpersGeometry->surface_add_vertex(vertex);
    }
    m_helpersGeometry->surface_end();
}

void VisualHelper::AddDirectionLine()
{
    if (m_boneInfoArray.empty())
    {
        return;
    }

    // setup the material
    Ref<StandardMaterial3D> material = memnew(StandardMaterial3D);
    material->set_albedo(Color::hex(0xFFaa55FF));
    material->set_shading_mode(BaseMaterial3D::ShadingMode::SHADING_MODE_UNSHADED);

    // calculate the distance from root bone to a target
    Vector3 start = m_boneInfoArray.front().position.origin;
    // the target position must be transferred to a local coordinates of the skeleton
    Vector3 end   = m_skeletonPosition.xform_inv(m_targetPosition.origin);
    Vector3 direction = end - start;
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
        m_helpersGeometry->surface_add_vertex(start + direction * dashSize * i);
        m_helpersGeometry->surface_add_vertex(start + direction * glm::min(dashSize * i + dashSize/2.f, distance));
    }
    m_helpersGeometry->surface_end();
}

}