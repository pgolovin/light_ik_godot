#include "joint_constraints.h"
#include "glm/glm.hpp"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>
namespace godot
{
void JointConstraints::_bind_methods()
{
    DECLARE_UNSCOPED_PROPERTY(JointConstraints, bone,        Variant::STRING);
    
    DECLARE_UNSCOPED_ENUM_PROPERTY(JointConstraints, rotation_order,        "PassThrough:0,TwistSwing:1,XZY:2,ZXY:3,YXZ:4,YZX:5,XYZ:6");
    DECLARE_UNSCOPED_ENUM_PROPERTY(JointConstraints, rotation_direction,    "CW:-1,CCW:1");

    DECLARE_UNSCOPED_PROPERTY(JointConstraints, min_angle,   Variant::VECTOR3);
    DECLARE_UNSCOPED_PROPERTY(JointConstraints, max_angle,   Variant::VECTOR3);
    DECLARE_UNSCOPED_PROPERTY(JointConstraints, center,      Variant::VECTOR3);
    DECLARE_UNSCOPED_PROPERTY(JointConstraints, flexibility, Variant::FLOAT);
}

void JointConstraints::set_bone(const String& bone_name) 
{ 
    m_dirtyFlag = m_dirtyFlag || (m_constraint.boneName != bone_name);
    m_constraint.boneName = bone_name; 
    notify_property_list_changed();
}  

String JointConstraints::get_bone() const 
{
    return m_constraint.boneName;
}

void JointConstraints::set_min_angle(const Vector3& min_angle) 
{ 
    m_dirtyFlag = m_dirtyFlag || (m_constraint.angleMin != min_angle);
    m_constraint.angleMin = min_angle; 
}  

Vector3 JointConstraints::get_min_angle() const 
{
    return m_constraint.angleMin;
}

void JointConstraints::set_max_angle(const Vector3& max_angle)
{
    m_dirtyFlag = m_dirtyFlag || (m_constraint.angleMax != max_angle);
    m_constraint.angleMax = max_angle; 
}

Vector3 JointConstraints::get_max_angle() const 
{
    return m_constraint.angleMax;
}

Vector3 JointConstraints::get_center() const 
{
    return m_constraint.center;
}

void JointConstraints::set_center(const Vector3& center)
{
    m_dirtyFlag = m_dirtyFlag || (m_constraint.center != center);
    m_constraint.center = center; 
}

void JointConstraints::set_flexibility(const double& stiffness) 
{ 
    double clampedStiffness = glm::clamp(stiffness, 0.0, 1.0);
    m_dirtyFlag = m_dirtyFlag || (m_constraint.flexibility != clampedStiffness);
    m_constraint.flexibility = clampedStiffness;
}

double JointConstraints::get_flexibility() const 
{
    return m_constraint.flexibility;
}

void JointConstraints::set_rotation_order(const int& rotation_order) 
{ 
    m_dirtyFlag = m_dirtyFlag || (m_constraint.rotationOrder != rotation_order);
    m_constraint.rotationOrder = rotation_order;
}

int JointConstraints::get_rotation_order() const 
{
    return m_constraint.rotationOrder;
}

void JointConstraints::set_rotation_direction(const int& rotationDirection) 
{ 
    m_dirtyFlag = m_dirtyFlag || (m_constraint.rotationDirection != rotationDirection);
    m_constraint.rotationDirection = rotationDirection;
}

int JointConstraints::get_rotation_direction() const 
{
    return m_constraint.rotationDirection;
}

bool JointConstraints::IsDirty()
{
    bool dirty = m_dirtyFlag;
    m_dirtyFlag = false;
    return dirty;
}

void JointConstraints::_validate_property(godot::PropertyInfo& info)
{
    if(!m_skeleton)
    {
        return;
    }

    if (info.name == String("bone"))
    {
        ValidateBone(info);
    }
}

void JointConstraints::_ready(Skeleton3D* skeleton)
{
    m_skeleton = skeleton;
}

void JointConstraints::ValidateBone(PropertyInfo& info)
{
    info.hint = PROPERTY_HINT_ENUM;

    // if root is not selected allow to chose any bone for tip
    info.hint_string = m_skeleton->get_concatenated_bone_names();
}

}