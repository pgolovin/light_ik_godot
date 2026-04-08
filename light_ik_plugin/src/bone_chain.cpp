#include "bone_chain.h"
#include "light_ik_plugin.h"
#include "glm/glm.hpp"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>

#include <stack>

namespace godot
{

bool BoneChain::IsDirty()
{
    bool dirty  = m_dirty;
    m_dirty     = false;
    return dirty;
}

void BoneChain::_bind_methods()
{
    DECLARE_UNSCOPED_PROPERTY(BoneChain, root_bone, (Variant::STRING));
    DECLARE_UNSCOPED_PROPERTY(BoneChain, tip_bone,  (Variant::STRING));
}

void BoneChain::set_root_bone(const String& root_bone_name) 
{
    SetDirty(m_rootBoneName != root_bone_name);
    m_rootBoneName = root_bone_name;
    notify_property_list_changed();
}

String BoneChain::get_root_bone() const 
{
    return m_rootBoneName;
}

void BoneChain::set_tip_bone(const String& tip_bone_name) 
{
    SetDirty(m_tipBoneName != tip_bone_name);
    m_tipBoneName = tip_bone_name;
    notify_property_list_changed();
}

String BoneChain::get_tip_bone() const 
{
    return m_tipBoneName;
}

void BoneChain::_ready(Skeleton3D* skeleton)
{
    m_skeleton  = skeleton;
}

void BoneChain::_validate_property(godot::PropertyInfo& info)
{
    if(!m_skeleton)
    {
        return;
    }

    if (info.name == String("root_bone"))
    {
        ValidateRootBone(info);
    }
    else if(info.name == String("tip_bone"))
    {
        ValidateTipBone(info);
    }
}
////////////////////////////////////////////// Editor functions

void BoneChain::ValidateRootBone(PropertyInfo& info)
{
    info.hint = PROPERTY_HINT_ENUM;

    assert(m_skeleton);
    
    // if tip is not selected allow to chose any bone for root
    int tipBone = m_skeleton->find_bone(m_tipBoneName);
    if (-1 == tipBone)
    {
        info.hint_string = m_skeleton->get_concatenated_bone_names();
        return;
    }

    // list all parent bones from current tip (including) to skeleton root
    int32_t bone = tipBone;
    info.hint_string = "";
    while (bone >= 0)
    {
        // inverse addition to the list to keep the right order of the bones, from root to current
        info.hint_string = m_skeleton->get_bone_name(bone) + "," + info.hint_string;
        bone = m_skeleton->get_bone_parent(bone);
    }
}

void BoneChain::ValidateTipBone(PropertyInfo& info)
{
    info.hint = PROPERTY_HINT_ENUM;

    assert(m_skeleton);

    int rootBone = m_skeleton->find_bone(m_rootBoneName);
    if (-1 == rootBone)
    {
        // if root is not selected allow to chose any bone for tip
        info.hint_string = m_skeleton->get_concatenated_bone_names();
        return;
    }
    
    std::stack<int32_t> boneStack;
    // allow to chose any child bone from the selected root, including the root itself, to make look-at bones
    
    boneStack.emplace(rootBone);
    // create the list of all child bones from current to all branch leaves
    // avoid recursion. create the stack of bones and process it.
    
    while (!boneStack.empty())
    {
        int32_t rootBone = boneStack.top();
        boneStack.pop();
        // concatenate names of child bones into the single list
        info.hint_string += m_skeleton->get_bone_name(rootBone) + ",";

        // put all child bones to stack to process them later
        const auto& bones = m_skeleton->get_bone_children(rootBone);
        for (int32_t bone : bones)
        {
            boneStack.emplace(bone);
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////
/// Standard IK chain 
////////////////////////////////////////////////////////////////////////////////////////////

void ChainIKTarget::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_target"), &ChainIKTarget::get_target);
    ClassDB::bind_method(D_METHOD("set_target", "target"), &ChainIKTarget::set_target);
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "target", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Node3D"), "set_target", "get_target");
}

void ChainIKTarget::set_target(const NodePath& target_path) 
{
    SetDirty(m_targetPath != target_path);
    m_targetPath = target_path;
}

NodePath ChainIKTarget::get_target() const 
{
    return m_targetPath;
}

////////////////////////////////////////////////////////////////////////////////////////////
/// IK chain that can target any bone in the skeleton
////////////////////////////////////////////////////////////////////////////////////////////

void ChainIKBoneLink::_bind_methods()
{
    DECLARE_UNSCOPED_PROPERTY(ChainIKBoneLink, target_bone, Variant::STRING);
}

void ChainIKBoneLink::set_target_bone(const String& target_bone_name) 
{
    SetDirty(m_targetBoneName != target_bone_name);
    m_targetBoneName = target_bone_name;
    notify_property_list_changed();
}

String ChainIKBoneLink::get_target_bone() const 
{
    return m_targetBoneName;
}

void ChainIKBoneLink::_validate_property(godot::PropertyInfo& info)
{
    if(!m_skeleton)
    {
        return;
    }

    if (info.name == String("target_bone"))
    {
        info.hint = PROPERTY_HINT_ENUM;
        info.hint_string = m_skeleton->get_concatenated_bone_names();
    }
}

}