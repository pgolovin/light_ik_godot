#pragma once

#include "helpers.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>

namespace godot
{
struct ConstraintData
{
    String  boneName;
    Vector3 angleMin{0,0,0};
    Vector3 angleMax{0,0,0};
    double  flexibility{1};  
    int rotationOrder = 0;
    int rotationDirection = 1;
};

class JointConstraints : public Resource
{  
    GDCLASS(JointConstraints, Resource)  
    DEFINE_PROPERTY(String,  bone);
    DEFINE_PROPERTY(Vector3, min_angle);
    DEFINE_PROPERTY(Vector3, max_angle);
    DEFINE_PROPERTY(double,  flexibility);

    DEFINE_PROPERTY(int,  rotation_order);
    DEFINE_PROPERTY(int,  rotation_direction);

public:  
    bool IsDirty();
    const ConstraintData& GetConstraintData() const     { return m_constraint; }
    void _validate_property(godot::PropertyInfo& info);
    void _ready(Skeleton3D* skeleton);

protected:  
    static void _bind_methods();
    void ValidateBone(PropertyInfo& info);

    ConstraintData  m_constraint;
    Skeleton3D*     m_skeleton      = nullptr;
    bool            m_dirtyFlag     = false;
    
};

}