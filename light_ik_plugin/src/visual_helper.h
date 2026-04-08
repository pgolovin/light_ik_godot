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
    Transform3D position;
    Vector3 minAngles{0,0,0};
    Vector3 maxAngles{0,0,0};
    float   flexibility = 1;
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
    void SetTargetPosition(const Transform3D& skeletonOrigin, const Transform3D& target);
    size_t AddDebugLine(const std::vector<Vector3>& line, size_t index = 0xFFFFFF);

    struct TipTarget
    {
        Vector3 tip;
        Vector3 target;
    };

    void UpdateTipTargetInfo(const std::list<TipTarget>& tips);

public: // godot overrides
    void _ready() override;
    void _process(double delta) override;

private:

    void AddRootBoneMarker();
    void AddTipMarker();
    void LineStripFromVector(const std::vector<Vector3>& strip, const Transform3D& position, float scale, Color color);
    void AddConstraint(const BoneInfo& constraint);
    void AddConstraintMarker(float minAngle, float maxAngle, const Transform3D& position, float flexibility, const Vector3& axis, Color color);
    void AddDashedLine(const Vector3& from, const Vector3& to);
    void AddDebugLines();

    std::list<TipTarget>    m_tipTargets;

    std::vector<BoneInfo>   m_boneInfoArray;
    Ref<ImmediateMesh>      m_helpersGeometry;

    size_t                  m_pointsPerMarker = 32;
    float                   m_radiusJoint = 0.2f;
    float                   m_radiusRoot = 0.5f;
    Transform3D             m_targetPosition;
    Transform3D             m_skeletonPosition;
    bool                    m_updateRequired{false};

     std::vector<std::vector<Vector3>> m_debugLines;
};

}