#pragma once

#include "helpers.h"

#include "light_ik/light_ik.h"
#include "joint_constraints.h"

#include <godot_cpp/classes/skeleton_modifier3d.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/node_path.hpp>

namespace godot
{

class VisualHelper;

class LightIKPlugin : public SkeletonModifier3D
{
    GDCLASS(LightIKPlugin, SkeletonModifier3D)

    DEFINE_PROPERTY(bool, simulate);
    DEFINE_PROPERTY(String, root_bone);
    DEFINE_PROPERTY(String, tip_bone);
    DEFINE_PROPERTY(NodePath, target);
    DEFINE_PROPERTY(TypedArray<JointConstraints>, constraints_array);
    DEFINE_PROPERTY(bool, show_helpers);

public:
    LightIKPlugin();
    ~LightIKPlugin();

    void _ready() override;
    void _process_modification() override;
    void _validate_property(godot::PropertyInfo& info);

    void UpdateEditorData(bool updateRequired);
protected:
    static void _bind_methods();

private:
    bool UpdateBoneChain();    
    void MakeConstraints();
    void UpdateIKData();

    void ValidateRootBone(PropertyInfo& info);
    void ValidateTipBone(PropertyInfo& info);
    
    void UpdateVisualHelperData();

    String                  m_rootBoneName;
    int32_t                 m_rootBone = -1;

    String                  m_tipBoneName;
    int32_t                 m_tipBone = -1;

    NodePath                m_targetPath;    
    Node3D*                 m_target;
    TypedArray<JointConstraints> m_constraintsArray;
    std::vector<int32_t>    m_boneChain;

    LightIK::LightIK        m_lightIKCore;
    
    real_t                  m_time = 0;
    VisualHelper*           m_helper;
    bool                    m_simulate      = true;
    bool                    m_printed       = true;
    bool                    m_showHelpers   = false;
};

}