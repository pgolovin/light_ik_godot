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
constexpr bool settingEnableDebugging = true;
constexpr bool settingAllowRuntimeModification = true;

class VisualHelper;

class LightIKPlugin : public SkeletonModifier3D
{
    GDCLASS(LightIKPlugin, SkeletonModifier3D)

    DEFINE_PROPERTY(int,    iterations_count);
    DEFINE_PROPERTY(bool,   simulate);

    DEFINE_PROPERTY(bool,   show_helpers);
    DEFINE_PROPERTY(float,  marker_radius);
    DEFINE_PROPERTY(float,  constraint_radius);

    DEFINE_PROPERTY(TypedArray<BoneChain>, bone_chains);
    DEFINE_PROPERTY(TypedArray<JointConstraints>, constraints_array);


public:
    LightIKPlugin();
    ~LightIKPlugin();

    void _ready() override;
    void _process(double delta) override;
    void _process_modification() override;

protected:
    static void _bind_methods();
    
private:
    // Build and process skeleton
    void UpdateSkeletonParameters();

    int                     m_iterationsCount           = 1;
    bool                    m_simulate                  = false;
    std::unique_ptr<LightIK::LightIK> m_controllerIK    = nullptr;

    // Build and process chains of all types
    void BuildChains();
    void BuildTargetChain(ChainIKTarget& chain, uint32_t index);
    void BuildLinkChain(ChainIKBoneLink& link, uint32_t index);
    std::vector<LightIK::BoneDesc> BuildRootChain(int32_t tipBone, real_t leafBoneLength);
    void UpdateChainsVisualData();

    struct NodeTarget
    {
        Node3D* target = nullptr;
        LightIK::TargetPosition* pos;
    };
    TypedArray<BoneChain>   m_boneChains;
    std::list<NodeTarget>   m_targets;

    // Build and process constraints data
    void BuildConstraints();
    void UpdateConstraintsVisualData();
    TypedArray<JointConstraints> m_constraintsArray;

    // DEBUG visualization data
    void AddChainLine(std::vector<LightIK::BoneDesc> chain, int32_t startIndex, int32_t targetIndex, size_t chainId);
    struct ChainDebugInfo
    {
        std::vector<int32_t> indices;
        int32_t startIndex  = -1;
        int32_t targetIndex = -1;
        size_t chainId      = 0;
    };
    std::vector<ChainDebugInfo> m_debugChains;
    VisualHelper*           m_helper;
    bool                    m_showHelpers   = false;

};

}