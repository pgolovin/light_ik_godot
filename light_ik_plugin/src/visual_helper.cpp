#include "visual_helper.h"

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
    if (0 == info.size() || info.size() != bones.size())
    {
        return;
    }

    m_boneInfoArray.resize(info.size());
     // fill the bone constraints information from the constraints array size
    for (size_t c = 0; c < info.size(); ++c)
    {
        JointConstraints* constraint = Object::cast_to<JointConstraints>(info[c]);
        BoneInfo& info = m_boneInfoArray[c];

        info.position       = bones[c].origin;
        info.direction      = bones[c].basis[1];
        info.constrained = (constraint != nullptr);
        // TODO: add partial constraints
        if (constraint)
        {
            info.minAngles = constraint->get_min_angle() / 180.f * glm::pi<float>();
            info.maxAngles = constraint->get_max_angle() / 180.f * glm::pi<float>();
            info.stiffness = constraint->get_stiffness();
        }
        else
        {
            info.minAngles = {-glm::pi<float>(), -glm::pi<float>(), -glm::pi<float>()};
            info.maxAngles = {glm::pi<float>(), glm::pi<float>(), glm::pi<float>()};
            info.stiffness = 0;
        }
    }

    Vector3 origin = m_boneInfoArray.front().position;
   
    m_helpersGeometry->clear_surfaces();
    AddRootBoneMarker();
    for (const auto& constraint : m_boneInfoArray)
    {
        AddConstraint(constraint);
    }
}

void VisualHelper::_ready()
{
    
}

void VisualHelper::_process(double delta)
{
    
}

void VisualHelper::AddRootBoneMarker()
{
    static std::array<Vector3, 4> rectangleVertices = {Vector3{1, 0, 1}, {1, 0, -1}, {-1, 0, -1}, {-1, 0, 1}};

    Ref<StandardMaterial3D> material = memnew(StandardMaterial3D);
    material->set_albedo(Color::hex(0xFF55FFFF));
    material->set_shading_mode(BaseMaterial3D::ShadingMode::SHADING_MODE_UNSHADED);

    m_helpersGeometry->surface_begin(Mesh::PrimitiveType::PRIMITIVE_LINE_STRIP, material);
    for (const auto& vertex : rectangleVertices)
    {
        m_helpersGeometry->surface_add_vertex(m_boneInfoArray.front().position + m_radiusRoot * vertex);
    }
    // close the shape
    m_helpersGeometry->surface_add_vertex(m_boneInfoArray.front().position + m_radiusRoot * rectangleVertices[0]);
    m_helpersGeometry->surface_end();
}

void VisualHelper::AddConstraint(const BoneInfo& constraint)
{
    AddConstraintMarker(constraint.minAngles.x, constraint.maxAngles.x, constraint.position, {1,0,0}, Color::hex(0xFF5555FF));
    AddConstraintMarker(constraint.minAngles.y, constraint.maxAngles.y, constraint.position, {0,1,0}, Color::hex(0x5555FFFF));
    AddConstraintMarker(constraint.minAngles.y, constraint.maxAngles.y, constraint.position, {0,0,1}, Color::hex(0x55FF55FF));
}

void VisualHelper::AddConstraintMarker(float minAngle, float maxAngle, const Vector3& position, const Vector3& axis, Color color)
{
    Ref<StandardMaterial3D> material = memnew(StandardMaterial3D);
    material->set_albedo(color);
    material->set_shading_mode(BaseMaterial3D::ShadingMode::SHADING_MODE_UNSHADED);
    m_helpersGeometry->surface_begin(Mesh::PrimitiveType::PRIMITIVE_LINE_STRIP, material);

    for (size_t i = 0; i < m_pointsPerMarker; ++i)
    {
        float t     = i / (m_pointsPerMarker - 1.f);
        float pos   = minAngle * t + maxAngle * (1 - t);
        Vector3 vertex = m_radiusJoint * Vector3{glm::sin(pos), glm::cos(pos), 0.f};
        // TODO calculate angle
        vertex.rotate(axis, glm::pi<float>()/2.f);
        m_helpersGeometry->surface_add_vertex(position + vertex);
    }
    m_helpersGeometry->surface_end();
}

}