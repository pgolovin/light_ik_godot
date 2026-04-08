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
    ADD_GROUP("Helpers", "help_");
    DECLARE_PROPERTY(LightIKPlugin, simulate,           (Variant::BOOL), help);
    DECLARE_PROPERTY(LightIKPlugin, iterations_count,   (Variant::INT),  help);
    DECLARE_PROPERTY(LightIKPlugin, show_helpers,       (Variant::BOOL), help);

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

void LightIKPlugin::set_show_helpers(const bool& animate) 
{
    m_showHelpers = animate;
}
bool LightIKPlugin::get_show_helpers() const 
{
    return m_showHelpers; 
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
            // reset chain dirty state
            chain->IsDirty();

            // attach target to the bone
            Node3D* targetNode = get_node<Node3D>(chain->GetTargetPath());
            // build chain of bones
            int32_t chainTipBone    = get_skeleton()->find_bone(chain->GetTipBone());
            int32_t chainStartBone  = get_skeleton()->find_bone(chain->GetRootBone());
            
            // if parameters are invalid, no need to build this chain
            if (!targetNode || chainTipBone < 0 || chainStartBone < 0 )
            {
                UtilityFunctions::push_error("The ", i, "th chain parameters are invalid");
                continue;
            }

            auto target = m_targets.emplace_back( NodeTarget{targetNode, &m_controllerIK->CreateTarget()});
            auto rootChain          = BuildRootChain(chainTipBone);

            m_controllerIK->CreateIKChain(rootChain, chainStartBone, *target.pos);    
            continue;
        } 
    
        ChainIKBoneLink* link = Object::cast_to<ChainIKBoneLink>(m_boneChains[i]);
        if (link)
        {
            // reset link dirty state
            link->IsDirty();

            // find bones in the skeleton to build the link
            int32_t chainTipBone    = get_skeleton()->find_bone(link->GetTipBone());
            int32_t chainStartBone  = get_skeleton()->find_bone(link->GetRootBone());
            int32_t chainTargetBone = get_skeleton()->find_bone(link->GetTargetBone());

            // if parameters are invalid, no need to build this chain
            if (chainTargetBone < 0 || chainTipBone < 0 || chainStartBone < 0 )
            {
                UtilityFunctions::push_error("The ", i, "th chain parameters are invalid");
                continue;            }

            auto rootChain          = BuildRootChain(chainTipBone);
            auto targetChain        = BuildRootChain(chainTargetBone);

            m_controllerIK->CreatePassiveChain(targetChain);
            m_controllerIK->CreateIKLink(rootChain, chainStartBone, chainTargetBone);

            continue;
        }

        UtilityFunctions::push_error("The ", i, "th chain is not set");
    }
}

void LightIKPlugin::BuildConstraints()
{
    for (size_t i = 0; i < m_constraintsArray.size(); ++i)
    {
        JointConstraints* constraint = Object::cast_to<JointConstraints>(m_constraintsArray[i]);
        
    }

    // if (UpdateBoneChain())
    // {
    //     UpdateIKData();
    //     UpdateVisualHelperData();
    // }
}

void LightIKPlugin::_process_modification()
{
    if (!is_inside_tree() || !is_node_ready() || !m_simulate || !m_controllerIK)
    {
        return;
    }

    Transform3D skeletonPosition = get_skeleton()->get_global_transform();

    for (auto& target : m_targets)
    {
        Vector3 localPosition = skeletonPosition.affine_inverse().xform(target.target->get_global_transform().origin);
        target.pos->SetPosition(ToLightIKVector(localPosition));
    }

    m_controllerIK->Update(m_iterationsCount);

    auto& deltas = m_controllerIK->GetDeltaRotations();
    for (int32_t index = 0; index < deltas.size(); ++index)
    {
        if (deltas[index])
        {
            get_skeleton()->set_bone_pose_rotation(index, FromLightIKQuaternion(*deltas[index]));
        }
    }
}

void LightIKPlugin::_process(double delta)
{
    if (godot::Engine::get_singleton()->is_editor_hint())
    {
        Transform3D skeletonPosition = get_skeleton()->get_global_transform();

        for (auto& target : m_targets)
        {
            Vector3 localPosition = skeletonPosition.affine_inverse().xform(target.target->get_global_transform().origin);
            target.pos->SetPosition(ToLightIKVector(localPosition));
        }
        
        UpdateEditorData(true);
    }
}

std::vector<LightIK::BoneDesc> LightIKPlugin::BuildRootChain(int32_t tipBone)
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
        rootChain.emplace_back(LightIK::BoneDesc{rotation, 1, tipBone});
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

// void LightIKPlugin::_validate_property(godot::PropertyInfo& info)
// {
//     // if (info.name == String("skeleton_root_bone"))
//     // {
//     //     ValidateRootBone(info);
//     // }
//     // else if(info.name == String("skeleton_tip_bone"))
//     // {
//     //     ValidateTipBone(info);
//     // }
// }

////////////////////////////////////////////// Editor functions

// void LightIKPlugin::ValidateRootBone(PropertyInfo& info)
// {
//     info.hint = PROPERTY_HINT_ENUM;
//     if (!get_skeleton())
//     {
//         return;
//     }
//     if (-1 == m_tipBone)
//     {
//         // if tip is not selected allow to chose any bone for root
//         info.hint_string = get_skeleton()->get_concatenated_bone_names();
//         return;
//     }

//     assert(m_tipBone != m_rootBone);
//     // list all parent bones from current tip (excluding) to skeleton root
//     int32_t bone = get_skeleton()->get_bone_parent(m_tipBone);
//     info.hint_string = "";
//     while (bone >= 0)
//     {
//         // inverse addition to the list to keep the right order of the bones, from root to current
//         info.hint_string = get_skeleton()->get_bone_name(bone) + "," + info.hint_string;
//         bone = get_skeleton()->get_bone_parent(bone);
//     }
// }

// void LightIKPlugin::ValidateTipBone(PropertyInfo& info)
// {
//     if (!get_skeleton())
//     {
//         return;
//     }

//     info.hint = PROPERTY_HINT_ENUM;
//     if (-1 == m_rootBone)
//     {
//         // if root is not selected allow to chose any bone for tip
//         info.hint_string = get_skeleton()->get_concatenated_bone_names();
//         return;
//     }
    
//     assert(m_tipBone != m_rootBone);
//     std::stack<int32_t> boneStack;
//     // allow to chose any child bone from the selected root
//     const auto& bones = get_skeleton()->get_bone_children(m_rootBone);
//     for (int32_t bone : bones)
//     {
//         boneStack.emplace(bone);
//     }
//     // create  the list of all child bones from current to all branch leaves
//     // avoid recursion. create the stack of bones and process it.
    
//     while (!boneStack.empty())
//     {
//         int32_t rootBone = boneStack.top();
//         boneStack.pop();
//         // concatenate names of child bones into the single list
//         info.hint_string += get_skeleton()->get_bone_name(rootBone) + ",";

//         // put all child bones to stack to process them later
//         const auto& bones = get_skeleton()->get_bone_children(rootBone);
//         for (int32_t bone : bones)
//         {
//             boneStack.emplace(bone);
//         }
//     }
// }

// bool LightIKPlugin::UpdateBoneChain()
// {
//     bool found = false;
//     int32_t bone = m_tipBone;
//     m_boneChain.clear();
//     // go through all bones in backward direction from tip to root to form the bone chain
//     while (bone >= 0)
//     {
//             // create the actual bone chain for simulation from tip to begin bone
//         m_boneChain.emplace(m_boneChain.begin(), bone);
//         if (bone == m_rootBone)
//         {
//             // ok, we found both tip and bone, so chain is complete
//             found = true;
//             break;
//         }
//         bone = get_skeleton()->get_bone_parent(bone);
//     }
//     // ok chain is incomplete, so cleanup all trash that we had put into the chain array
//     if (!found)
//     {
//         m_boneChain.clear();
//     }
//     return found;
// }

// void LightIKPlugin::MakeConstraints()
// {
//     if (m_boneChain.empty())
//     {
//         UtilityFunctions::push_error("Bones ", m_rootBoneName, " and ", m_tipBoneName, " are not on the same branch.");
//         return;
//     }
//     // the last bone is the tip. it is not used for calculations
//     m_constraintsArray.resize(m_boneChain.size() - 1);
//     UpdateIKData();
//     UpdateVisualHelperData();
// }

// void LightIKPlugin::UpdateIKData()
// {
//     m_lightIKCore.Reset();
//     if (m_boneChain.empty())
//     {
//         return;
//     }
//     Vector3 base    = get_skeleton()->get_bone_global_pose(m_boneChain.front()).origin;
//     auto q          = get_skeleton()->get_bone_pose_rotation(m_boneChain.front());

//     m_lightIKCore.SetRootPosition(LightIK::Vector(base.x, base.y, base.z));
//     for (size_t b = 1; b < m_boneChain.size(); ++b)
//     {        
//         q               = get_skeleton()->get_bone_pose_rotation(m_boneChain[b - 1]);
//         Vector3 origin  = get_skeleton()->get_bone_global_pose(m_boneChain[b]).origin;
//         real_t length   = (origin - base).length();
//         m_lightIKCore.AddBone(length, LightIK::Quaternion{ (LightIK::real)q.w, (LightIK::real)q.x, (LightIK::real)q.y, (LightIK::real)q.z});
//         base            = origin;

//         JointConstraints* constraint = Object::cast_to<JointConstraints>(m_constraintsArray[b - 1]);
//         if (constraint)
//         {
//             LightIK::Constraints c{constraint->get_flexibility()};
//             m_lightIKCore.SetConstraint(b - 1, std::move(c));
//         }

//     }
//     m_lightIKCore.CompleteChain();
// }

void LightIKPlugin::UpdateVisualHelperData()
{
    // std::vector<Transform3D> bones;
    
    // int32_t bone = m_tipBone;
    // // collect position and original direction of all bones in the chain
    // for (int32_t boneId : m_boneChain)
    // {
    //     bones.emplace_back(get_skeleton()->get_bone_global_pose(boneId));
    // }
    // m_helper->SetConstraintsInfo(m_constraintsArray, bones);
}

void LightIKPlugin::UpdateEditorData(bool)
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

    std::list<VisualHelper::TipTarget> tt;
    for (size_t i = 0; i < m_controllerIK->GetSolversCount(); ++i)
    {
        tt.emplace_back(VisualHelper::TipTarget{FromLightIKVector(m_controllerIK->GetTipPosition(i)), FromLightIKVector(m_controllerIK->GetTargetPosition(i))});
    }
    m_helper->UpdateTipTargetInfo(tt);

    // Transform3D targetPosition = m_target ? m_target->get_global_transform() : Transform3D();
    // m_helper->SetTargetPosition(get_skeleton()->get_global_transform(), targetPosition);

    // // if array data was modified, request the update of constraints visualization
    // if (constraintsUpdated)
    // {
    //     for (size_t i = 0; i < m_constraintsArray.size(); ++i)
    //     {
    //         JointConstraints* constraint = Object::cast_to<JointConstraints>(m_constraintsArray[i]);
    //         if (constraint)
    //         {
    //             LightIK::Constraints c{constraint->get_flexibility()};
    //             m_lightIKCore.SetConstraint(i, std::move(c));
    //         }
    //     }

    //     UpdateVisualHelperData();
    // }
}

}