#include "light_ik_plugin.h"
#include "joint_constraints.h"
#include "visual_helper.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>

#include <stack>
#include <glm/ext/scalar_constants.hpp>

namespace godot
{
    
static inline Vector3 FromLightIKVector(const LightIK::Vector& src)
{
    return Vector3{(real_t)src.x, (real_t)src.y, (real_t)src.z};
}

static inline LightIK::Vector ToLightIKVector(const Vector3& src)
{
    return LightIK::Vector{(LightIK::real)src.x, (LightIK::real)src.y, (LightIK::real)src.z};
}

static inline LightIK::Quaternion ToLightIKQuaternion(const Quaternion& quat)
{
    return LightIK::Quaternion{(LightIK::real)quat.w, (LightIK::real)quat.x, (LightIK::real)quat.y, (LightIK::real)quat.z};
}

static inline Quaternion FromLightIKQuaternion(const LightIK::Quaternion& quat)
{
    return Quaternion{(real_t)quat.x, (real_t)quat.y, (real_t)quat.z, (real_t)quat.w};
}

void LightIKPlugin::_bind_methods()
{
    DECLARE_UNSCOPED_PROPERTY(LightIKPlugin, simulate,           (Variant::BOOL));
    DECLARE_UNSCOPED_PROPERTY(LightIKPlugin, iterations_count,   (Variant::INT));
    
    ADD_GROUP("Visualization", "helpers_");
    DECLARE_PROPERTY(LightIKPlugin, show_helpers,       (Variant::BOOL), helpers);
    DECLARE_PROPERTY(LightIKPlugin, marker_radius,      (Variant::FLOAT), helpers);
    DECLARE_PROPERTY(LightIKPlugin, constraint_radius,  (Variant::FLOAT), helpers);

    ADD_GROUP("Bone Chains", "chains_");

    ClassDB::bind_method(D_METHOD("get_bone_chains"), &LightIKPlugin::get_bone_chains);
    ClassDB::bind_method(D_METHOD("set_bone_chains", "bone_chains"), &LightIKPlugin::set_bone_chains);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "chains_bone_chains", PROPERTY_HINT_TYPE_STRING, 
            String::num(Variant::OBJECT) + "/" + String::num(PROPERTY_HINT_RESOURCE_TYPE) + ":BoneChain"), "set_bone_chains", "get_bone_chains");

    ADD_GROUP("Constraints", "constraints_");

    ClassDB::bind_method(D_METHOD("get_constraints_array"), &LightIKPlugin::get_constraints_array);
    ClassDB::bind_method(D_METHOD("set_constraints_array", "constraints_array"), &LightIKPlugin::set_constraints_array);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "constraints_constraints_array", PROPERTY_HINT_TYPE_STRING, 
            String::num(Variant::OBJECT) + "/" + String::num(PROPERTY_HINT_RESOURCE_TYPE) + ":JointConstraints"), "set_constraints_array", "get_constraints_array");
}

////////////////////////////////////////////// properties definition
void LightIKPlugin::set_iterations_count(const int& interations) 
{
    m_iterationsCount = interations;
}

int LightIKPlugin::get_iterations_count() const 
{
    return m_iterationsCount; 
}

void LightIKPlugin::set_simulate(const bool& animate) 
{
    m_simulate = animate;
    if (!m_simulate && get_skeleton())
    {
        get_skeleton()->clear_bones_global_pose_override();
        m_controllerIK->ResetPose();
    }
}

bool LightIKPlugin::get_simulate() const 
{
    return m_simulate; 
}

void LightIKPlugin::set_show_helpers(const bool& show) 
{
    m_showHelpers = show;
    m_helper->Enable(show);
}
bool LightIKPlugin::get_show_helpers() const 
{
    return m_showHelpers; 
}

void LightIKPlugin::set_marker_radius(const float& radius) 
{
    m_helper->SetRootMarkerRadius(radius);
}
float LightIKPlugin::get_marker_radius() const 
{
    return m_helper->GetRootMarkerRadius(); 
}

void LightIKPlugin::set_constraint_radius(const float& radius) 
{
    m_helper->SetConstraintMarkerRadius(radius);
}
float LightIKPlugin::get_constraint_radius() const 
{
    return m_helper->GetConstraintMarkerRadius(); 
}

void LightIKPlugin::set_bone_chains(const TypedArray<BoneChain>& array) 
{
    if (!is_node_ready())
    {
        m_boneChains = array;
        return;
    }

    m_boneChains = array;
    for (size_t i = 0; i < m_boneChains.size(); ++i)
    {
        auto chain = Object::cast_to<BoneChain>(m_boneChains[i]);
        if (chain)
        {
            chain->_ready(get_skeleton());
        }
    }

    BuildChains();
}

TypedArray<BoneChain> LightIKPlugin::get_bone_chains() const 
{
    return m_boneChains; 
}


void LightIKPlugin::set_constraints_array(const TypedArray<JointConstraints>& array) 
{
    m_constraintsArray = array;

    if (!is_node_ready())
    {
        return;
    }

    for (size_t i = 0; i < m_constraintsArray.size(); ++i)
    {
        JointConstraints* constraint = Object::cast_to<JointConstraints>(m_constraintsArray[i]);
        if (constraint)
        {
            constraint->_ready(get_skeleton());
        }
    }
}

TypedArray<JointConstraints> LightIKPlugin::get_constraints_array() const 
{
    return m_constraintsArray; 
}

LightIKPlugin::LightIKPlugin()
    : m_helper(memnew(VisualHelper))
{
    add_child(m_helper);
}

LightIKPlugin::~LightIKPlugin()
{

}

////////////////////////////////////////////// godot interface
void LightIKPlugin::_ready()
{
    m_helper->set_owner(this);
    // on object initialization the skeleton doesn't exists, 
    // so parameters should be updated at the moment the object is fully constructed
    assert (get_skeleton());

    m_controllerIK = std::make_unique<LightIK::LightIK>(get_skeleton()->get_bone_count());
    
    for (size_t i = 0; i < m_boneChains.size(); ++i)
    {
        BoneChain* chain = Object::cast_to<BoneChain>(m_boneChains[i]);
        if (chain)
        {
            chain->_ready(get_skeleton());
        }
    }

    BuildChains();

    for (size_t i = 0; i < m_constraintsArray.size(); ++i)
    {
        JointConstraints* constraint = Object::cast_to<JointConstraints>(m_constraintsArray[i]);
        if (constraint)
        {
            constraint->_ready(get_skeleton());
        }
    }

    BuildConstraints();
}

void LightIKPlugin::_process_modification()
{
    if (!is_inside_tree() || !is_node_ready() || !m_simulate || !m_controllerIK)
    {
        return;
    }
    
    // Calculate positions of all external targets. The target position is calculated in skeleton relative coordinates
    Transform3D skeletonPosition = get_skeleton()->get_global_transform();
    for (auto& target : m_targets)
    {
        Vector3 localPosition = skeletonPosition.affine_inverse().xform(target.target->get_global_transform().origin);
        target.pos->SetPosition(ToLightIKVector(localPosition));
    }

    // Process all chains
    m_controllerIK->Update(m_iterationsCount);

    // Update rotations of all bones
    auto& deltas = m_controllerIK->GetDeltaRotations();
    for (int32_t index = 0; index < deltas.size(); ++index)
    {
        if (deltas[index])
        {
            get_skeleton()->set_bone_pose_rotation(index, FromLightIKQuaternion(*deltas[index]));
        }
    }
    
    if constexpr (settingEnableDebugging)
    {
        // update chains visual data if required
        if (m_showHelpers)
        {
            assert(get_skeleton());
            UpdateChainsVisualData();
            UpdateConstraintsVisualData();
        }
    }
}

void LightIKPlugin::_process(double delta)
{
    // Update data of visual helpers
    if constexpr (settingAllowRuntimeModification)
    {
        if (godot::Engine::get_singleton()->is_editor_hint())
        {
            UpdateSkeletonParameters();
        }
    }

    if constexpr (settingEnableDebugging)
    {
        // update chains visual data if required
        if (m_showHelpers && !m_simulate)
        {
            assert(get_skeleton());
            UpdateChainsVisualData();
            UpdateConstraintsVisualData();
        }
    }
}

void LightIKPlugin::BuildChains()
{
    m_targets.clear();
    get_skeleton()->clear_bones_global_pose_override();
    m_controllerIK->Reset();

    for (size_t i = 0; i < m_boneChains.size(); ++i)
    {
        ChainIKTarget* chain = Object::cast_to<ChainIKTarget>(m_boneChains[i]);
        if (chain)
        {
            BuildTargetChain(*chain, i);
            continue;
        } 
    
        ChainIKBoneLink* link = Object::cast_to<ChainIKBoneLink>(m_boneChains[i]);
        if (link)
        {
            BuildLinkChain(*link, i);
            continue;
        }

        UtilityFunctions::push_error("The ", i, "th chain is not set");
    }
}

void LightIKPlugin::BuildTargetChain(ChainIKTarget& chain, uint32_t index)
{
    // reset chain dirty state
    chain.IsDirty();
    
    // build chain of bones
    int32_t chainTipBone    = get_skeleton()->find_bone(chain.GetTipBone());
    int32_t chainStartBone  = get_skeleton()->find_bone(chain.GetRootBone());
    
    // if parameters are invalid, no need to build this chain
    if (chain.GetTargetPath().is_empty() || chainTipBone < 0 || chainStartBone < 0 )
    {
        UtilityFunctions::push_error("The ", index, "th chain cannot be created, parameters are invalid");
        return;
    }
    // attach target to the bone
    Node3D* targetNode = get_node<Node3D>(chain.GetTargetPath());

    auto target             = m_targets.emplace_back( NodeTarget{targetNode, &m_controllerIK->CreateTarget()});
    auto rootChain          = BuildRootChain(chainTipBone, chain.GetLeafBoneLength());

    m_controllerIK->CreateIKChain(rootChain, chainStartBone, *target.pos);

    if constexpr (settingEnableDebugging)
    {
        // Visualize chain information in both editor and player
        AddChainLine(rootChain, chainStartBone, -1, m_controllerIK->GetSolversCount() - 1);
    }
}

void LightIKPlugin::BuildLinkChain(ChainIKBoneLink& link, uint32_t index)
{
    // reset link dirty state
    link.IsDirty();

    // find bones in the skeleton to build the link
    int32_t chainTipBone    = get_skeleton()->find_bone(link.GetTipBone());
    int32_t chainStartBone  = get_skeleton()->find_bone(link.GetRootBone());
    int32_t chainTargetBone = get_skeleton()->find_bone(link.GetTargetBone());

    // if parameters are invalid, no need to build this chain
    if (chainTargetBone < 0 || chainTipBone < 0 || chainStartBone < 0 )
    {
        UtilityFunctions::push_error("The ", index, " link cannot be created, parameters are invalid");
        return;            
    }

    auto rootChain          = BuildRootChain(chainTipBone, link.GetLeafBoneLength());
    auto targetChain        = BuildRootChain(chainTargetBone, 1.0);

    m_controllerIK->CreatePassiveChain(targetChain);
    m_controllerIK->CreateIKLink(rootChain, chainStartBone, chainTargetBone);
    
    if constexpr (settingEnableDebugging)
    {
        // Visualize chain information in both editor and player
        AddChainLine(rootChain, chainStartBone, chainTargetBone, m_controllerIK->GetSolversCount() - 1);
    }
}

void LightIKPlugin::BuildConstraints()
{
    for (size_t i = 0; i < m_constraintsArray.size(); ++i)
    {
        JointConstraints* constraintData = Object::cast_to<JointConstraints>(m_constraintsArray[i]);
        if (constraintData)
        {
            const ConstraintData& data = constraintData->GetConstraintData();
            LightIK::Constraints constraint {
                data.flexibility,
                ToLightIKVector((2.0 * Math_PI) * data.angleMin / 360.0),
                ToLightIKVector((2.0 * Math_PI) * data.angleMax / 360.0),
                LightIK::ConstraintType::Local,
                (LightIK::ConstraintModes)data.rotationOrder,
                (LightIK::ConstraintRotation)data.rotationDirection,
            };
            int32_t boneIndex = get_skeleton()->find_bone(data.boneName);
            if (boneIndex >= 0)
            {
                m_controllerIK->SetConstraint(boneIndex, std::move(constraint));
            }
            else
            {
                UtilityFunctions::push_error("Constraint cannot be set. Bone ", data.boneName, " not found");    
            }
        }
    }
}

std::vector<LightIK::BoneDesc> LightIKPlugin::BuildRootChain(int32_t tipBone, real_t leafBoneLength)
{
    assert(get_skeleton());
    std::vector<LightIK::BoneDesc> rootChain;

    // Build the root chain 
    Vector3 parentPosition = get_skeleton()->get_bone_global_pose(tipBone).origin;

    auto children = get_skeleton()->get_bone_children(tipBone);
    if (children.size())
    {
        // If child available calculate the length of the tip bone using its real parameters
        parentPosition = get_skeleton()->get_bone_global_pose(children[0]).origin;
    }
    else
    {
        // If tip bone is the leaf bone, consider the length of the bone is 1
        LightIK::Quaternion rotation = ToLightIKQuaternion(get_skeleton()->get_bone_pose_rotation(tipBone));
        rootChain.emplace_back(LightIK::BoneDesc{rotation, leafBoneLength, tipBone});
        tipBone = get_skeleton()->get_bone_parent(tipBone);
    }

    while(tipBone >= 0)
    {
        // Collect local rotation of the bone
        LightIK::Quaternion rotation = ToLightIKQuaternion(get_skeleton()->get_bone_pose_rotation(tipBone));
        
        // Calculate the length of the bone by using position of current and previous joint
        Vector3 currentPosition = get_skeleton()->get_bone_global_pose(tipBone).origin;
        real_t length = (currentPosition - parentPosition).length();

        // Add bone to the root chain
        rootChain.emplace_back(LightIK::BoneDesc{rotation, length, tipBone});

        // Proceed to the next bone
        parentPosition = currentPosition;
        tipBone = get_skeleton()->get_bone_parent(tipBone);
    }
    // the array is filled from tip to root, to work properly with arrays, need to reverse them
    // TODO: REALLY?
    std::reverse(rootChain.begin(), rootChain.end());
    return rootChain;
}

void LightIKPlugin::UpdateChainsVisualData()
{
    // Provide the list of transforms that represents bones in a single chain
    m_helper->ResetChainData();
    VisualHelper::ChainVisualData chainData;
    for (const auto& chain : m_debugChains)
    {
        chainData.chain.clear();
        for (int32_t boneId : chain.indices)
        {
            chainData.chain.emplace_back(get_skeleton()->get_bone_global_pose(boneId));
        }
        chainData.chain.emplace_back(Transform3D(chainData.chain.back().basis, FromLightIKVector(m_controllerIK->GetTipPosition(chain.chainId))));
        
        chainData.start     = get_skeleton()->get_bone_global_pose(chain.startIndex);

        chainData.target    = FromLightIKVector(m_controllerIK->GetTargetPosition(chain.chainId));
        m_helper->AddChain(chainData);
    }
}

void LightIKPlugin::UpdateConstraintsVisualData()
{
    // Provide information about bone constraints
    m_helper->ResetBoneConstraintsData();
    for (size_t i = 0; i < m_constraintsArray.size(); ++i)
    {
        JointConstraints* constraintData = Object::cast_to<JointConstraints>(m_constraintsArray[i]);
        
        if (constraintData) 
        {
            const auto& data = constraintData->GetConstraintData();
            int32_t index = get_skeleton()->find_bone(data.boneName);
            if (index < 0)
            {
                continue;
            }
            Basis localBasis;
            int32_t parent = get_skeleton()->get_bone_parent(index);
            Quaternion localRotation = get_skeleton()->get_bone_pose_rotation(index);
            Transform3D bonePosition = get_skeleton()->get_bone_global_pose(index);
            if (parent >= 0)
            {
                localBasis = get_skeleton()->get_bone_global_pose(parent).basis;

                //TODO: dirty workaround, looks like basis of the first bone w/o parent is calculated incorrectly (based on rotation of the skeleton)
                if (get_skeleton()->get_bone_parent(parent) < 0)
                {
                    localBasis = Basis() * localRotation;
                }
            }
            else 
            {
                //TODO: dirty workaround, looks like basis of the first bone w/o parent is calculated incorrectly (based on rotation of the skeleton)
                bonePosition.basis = Basis() * localRotation;
            }
            
            VisualHelper::BoneInfo info{Transform3D(localBasis, bonePosition.origin), bonePosition.basis, localRotation, data.rotationOrder, data.angleMin, data.angleMax, (real_t)data.flexibility, true};
            m_helper->AddBoneConstraint(info);
        }
    }
}

void LightIKPlugin::UpdateSkeletonParameters()
{
    bool chainsUpdated = false;
    for (size_t c = 0; c < m_boneChains.size(); ++c)
    {
        BoneChain* chain = Object::cast_to<BoneChain>(m_boneChains[c]);
        chainsUpdated = chainsUpdated || (chain && chain->IsDirty());
    }

    if (chainsUpdated)
    {
        BuildChains();
    }

    bool constraintsUpdated = false;
    for (size_t c = 0; c < m_constraintsArray.size(); ++c)
    {
        JointConstraints* constraint = Object::cast_to<JointConstraints>(m_constraintsArray[c]);
        constraintsUpdated = constraintsUpdated || (constraint && constraint->IsDirty());
    }
    
    // Check if target position had been changed to draw the direction line
    if (constraintsUpdated)
    {
        BuildConstraints();
    }
}

void LightIKPlugin::AddChainLine(std::vector<LightIK::BoneDesc> chain, int32_t startIndex, int32_t targetIndex, size_t chainId)
{
    auto& newDebugChain = m_debugChains.emplace_back();
    for (const LightIK::BoneDesc& bone : chain)
    {
        newDebugChain.indices.emplace_back(bone.boneIndex);
    }
    newDebugChain.startIndex    = startIndex;
    newDebugChain.targetIndex   = targetIndex;
    newDebugChain.chainId       = chainId;

    assert(newDebugChain.indices.size());
}

}