#pragma once

#include "helpers.h"

#include "light_ik/light_ik.h"
#include "bone_chain.h"

#include <godot_cpp/classes/skeleton_modifier3d.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/node_path.hpp>

#include <list>

namespace godot
{

class VisualHelper;

class LightIKPlugin : public SkeletonModifier3D
{
    GDCLASS(LightIKPlugin, SkeletonModifier3D)

    DEFINE_PROPERTY(int, iterations_count);
    DEFINE_PROPERTY(bool, simulate);
    DEFINE_PROPERTY(bool, show_helpers);
    DEFINE_PROPERTY(TypedArray<BoneChain>, bone_chains);
    DEFINE_PROPERTY(TypedArray<JointConstraints>, constraints_array);

public:
    LightIKPlugin();
    ~LightIKPlugin();

    void _ready() override;
    void _process(double delta) override;
    void _process_modification() override;
    //void _validate_property(godot::PropertyInfo& info);

    void UpdateEditorData(bool updateRequired);
protected:
    static void _bind_methods();

private:
    bool UpdateBoneChain();    
    void MakeConstraints();
    void UpdateIKData();

    void BuildChains();
    void BuildConstraints();

    std::vector<LightIK::BoneDesc> BuildRootChain(int32_t tipBone);

    void UpdateVisualHelperData();

    std::unique_ptr<LightIK::LightIK> m_controllerIK = nullptr;
    TypedArray<JointConstraints> m_constraintsArray;

    struct NodeTarget
    {
        Node3D* target = nullptr;
        LightIK::TargetPosition* pos;
    };

    TypedArray<BoneChain>   m_boneChains;
    std::list<NodeTarget>              m_targets;
    std::list<LightIK::TargetBone*>    m_links;

    real_t                  m_time = 0;
    VisualHelper*           m_helper;
    int                     m_iterationsCount = 1;
    bool                    m_simulate      = true;
    bool                    m_printed       = true;
    bool                    m_showHelpers   = false;
};

}