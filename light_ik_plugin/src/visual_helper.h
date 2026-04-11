#pragma once

#include "helpers.h"
#include "joint_constraints.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>

namespace godot
{

class ImmediateMesh;
class StandardMaterial3D;


class VisualHelper : public MeshInstance3D
{
    GDCLASS(VisualHelper, MeshInstance3D)
protected:
    static void _bind_methods() {};

public:
    VisualHelper();
    ~VisualHelper();

public: // godot overrides
    void _ready() override;
    void _process(double delta) override;

    struct ChainVisualData
    {
        std::vector<Transform3D> chain;
        Transform3D start;
        Vector3 target;
        bool hasTarget = false;
    };
    void ResetChainData()                                   { m_chainVisualData.clear();        }
    void AddChain(const ChainVisualData& visualData);

    
    struct BoneInfo
    {
        Transform3D position;
        Vector3 minAngles{0,0,0};
        Vector3 maxAngles{0,0,0};
        real_t  flexibility = 1;
        bool    constrained = false;
    };
    void ResetBoneConstraintsData()                         { m_boneConstraintData.clear();     }
    void AddBoneConstraint(const BoneInfo& constraintData);

    void SetRootMarkerRadius(float markerRadius)            { m_radiusRoot = markerRadius;      }
    float GetRootMarkerRadius() const                       { return m_radiusRoot;              }

    void SetConstraintMarkerRadius(float markerRadius)      { m_radiusJoint = markerRadius;     }
    float GetConstraintMarkerRadius() const                 { return m_radiusJoint;             }

    void Enable(bool enabled)                               { m_enabled = enabled;              }
private:

    void DrawStartMarker(const Transform3D& point);
    void DrawEndMarker(const Transform3D& position, const Transform3D& orientation);
    void DrawTargetMarker(const Transform3D& position);

    void DrawDashedLine(const Vector3& from, const Vector3& to, Ref<StandardMaterial3D>& material);
    void DrawLine(const std::vector<Transform3D>& points, Ref<StandardMaterial3D>& material);
    void DrawLineShape(const std::vector<Vector3>& shape, const Transform3D& position, float scale, Ref<StandardMaterial3D>& material);
    void DrawShape(const std::vector<Vector3>& shape, const Transform3D& position, float scale, Ref<StandardMaterial3D>& material, Mesh::PrimitiveType primitive);

    void DrawArc(const Transform3D& center, float radius, float minAngle, float maxAngle, Vector3 axis, Vector3 direction, Ref<StandardMaterial3D>& material);

    void MakeMaterial(Ref<StandardMaterial3D>& material, Color color);

    Ref<ImmediateMesh>              m_helpersGeometry;
    
    Ref<StandardMaterial3D>         m_targetLineMaterial;
    Ref<StandardMaterial3D>         m_chainLineMaterial;
    Ref<StandardMaterial3D>         m_startMarkerMaterial;
    Ref<StandardMaterial3D>         m_endMarkerMaterial;
    Ref<StandardMaterial3D>         m_targetMarkerMaterial;
    Ref<StandardMaterial3D>         m_jointMarkerMaterials[3];

    bool                            m_enabled       = false;
    float                           m_radiusJoint   = 0.2f;
    float                           m_radiusRoot    = 0.25f;
    
    std::vector<ChainVisualData>    m_chainVisualData;
    std::vector<BoneInfo>           m_boneConstraintData;

    const size_t                    m_pointsPerMarker = 32;
};

}
