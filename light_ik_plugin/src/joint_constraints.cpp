#include "joint_constraints.h"
#include "glm/glm.hpp"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>
namespace godot
{
void JointConstraints::_bind_methods()
{
    DECLARE_UNSCOPED_PROPERTY(JointConstraints, min_angle, Variant::VECTOR3);
    DECLARE_UNSCOPED_PROPERTY(JointConstraints, max_angle, Variant::VECTOR3);
    DECLARE_UNSCOPED_PROPERTY(JointConstraints, stiffness, Variant::FLOAT);
}

void JointConstraints::set_min_angle(const Vector3& min_angle) 
{ 
    m_dirtyFlag = m_dirtyFlag || (m_angleMin != min_angle);
    m_angleMin = min_angle; 
}  

Vector3 JointConstraints::get_min_angle() const 
{
    return m_angleMin;
}

void JointConstraints::set_max_angle(const Vector3& max_angle)
{
    m_dirtyFlag = m_dirtyFlag || (m_angleMax != max_angle);
    m_angleMax = max_angle; 
}

Vector3 JointConstraints::get_max_angle() const 
{
    return m_angleMax;
}

void JointConstraints::set_stiffness(const double& stiffness) 
{ 
    double clampedStiffness = glm::clamp(stiffness, 0.0, 0.99);
    m_dirtyFlag = m_dirtyFlag || (m_stiffness != clampedStiffness);
    m_stiffness = clampedStiffness;
}

double JointConstraints::get_stiffness() const 
{
    return m_stiffness;
}

bool JointConstraints::IsDirty()
{
    bool dirty = m_dirtyFlag;
    m_dirtyFlag = false;
    return dirty;
}

}