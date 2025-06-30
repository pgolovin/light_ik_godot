#pragma once

#include "helpers.h"
#include <godot_cpp/classes/resource.hpp>

namespace godot
{

class JointConstraints : public Resource
{  
    GDCLASS(JointConstraints, Resource)  
private:  
    Vector3 m_angleMin{0,0,0};
    Vector3 m_angleMax{0,0,0};
    double m_stiffness = 0;  
    bool   m_dirtyFlag = false;

public:  
    DEFINE_PROPERTY(Vector3, min_angle);
    DEFINE_PROPERTY(Vector3, max_angle);
    DEFINE_PROPERTY(double, stiffness);

    bool IsDirty();
protected:  
    static void _bind_methods();
};

}