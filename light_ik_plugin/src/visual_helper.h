#pragma once

#include "helpers.h"
#include "joint_constraints.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>

namespace godot
{

class ImmediateMesh;
class StandardMaterial3D;

struct BoneInfo
{
    Vector3 position{0,0,0};
    Vector3 direction{0,0,0};
    Vector3 minAngles{0,0,0};
    Vector3 maxAngles{0,0,0};
    float   stiffness = 0;
    bool    constrained = false;
};

class VisualHelper : public MeshInstance3D
{
    GDCLASS(VisualHelper, MeshInstance3D)
protected:
    static void _bind_methods() {};
public:
    VisualHelper();
    ~VisualHelper();

    void SetConstraintsInfo(const TypedArray<JointConstraints>& info, const std::vector<Transform3D>& bones);

public: // godot overrides
    void _ready() override;
    void _process(double delta) override;

private:

    void AddRootBoneMarker();
    void AddConstraint(const BoneInfo& constraint);
    void AddConstraintMarker(float minAngle, float maxAngle, const Vector3& position, const Vector3& axis, Color color);

    std::vector<BoneInfo>   m_boneInfoArray;
    Ref<ImmediateMesh>      m_helpersGeometry = nullptr;

    size_t                  m_pointsPerMarker = 32;
    float                   m_radiusJoint = 0.2f;
    float                   m_radiusRoot = 0.5f;
};

}