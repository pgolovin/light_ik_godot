#include "light_ik_plugin.h"
#include "joint_constraints.h"
#include "visual_helper.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/skeleton3d.hpp>

#include <stack>

namespace godot
{
void LightIKPlugin::_bind_methods()
{
    ADD_GROUP("IK Settings", "skeleton_");

    DECLARE_PROPERTY(LightIKPlugin, root_bone, (Variant::STRING), skeleton);
    DECLARE_PROPERTY(LightIKPlugin, tip_bone,  (Variant::STRING), skeleton);

    ClassDB::bind_method(D_METHOD("get_target"), &LightIKPlugin::get_target);
    ClassDB::bind_method(D_METHOD("set_target", "skeleton_target"), &LightIKPlugin::set_target);
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "skeleton_target", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Node3D"), "set_target", "get_target");

    ADD_GROUP("Constraints", "constraints_");

    ClassDB::bind_method(D_METHOD("get_constraints_array"), &LightIKPlugin::get_constraints_array);
    ClassDB::bind_method(D_METHOD("set_constraints_array", "constraints_array"), &LightIKPlugin::set_constraints_array);
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "constraints_constraints_array", PROPERTY_HINT_TYPE_STRING, 
            String::num(Variant::OBJECT) + "/" + String::num(PROPERTY_HINT_RESOURCE_TYPE) + ":JointConstraints"), "set_constraints_array", "get_constraints_array");

    ADD_GROUP("Helpers", "help_");
    DECLARE_PROPERTY(LightIKPlugin, simulate,  (Variant::BOOL), help);
    DECLARE_PROPERTY(LightIKPlugin, show_helpers,  (Variant::BOOL), help);
}

void LightIKPlugin::set_simulate(const bool& animate) 
{
    m_simulate = animate;
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

void LightIKPlugin::set_root_bone(const String& root_bone_name) 
{
    m_rootBoneName = root_bone_name;
    // prevent construction of the chain constraints untill the skeleton is ready
    if (is_node_ready() && get_skeleton())
    {
        m_rootBone = get_skeleton()->find_bone(root_bone_name);
        // if root bone was not found in the branch of cuurent tip bone, it must be reseted
        if (!UpdateBoneChain())
        {
            UtilityFunctions::push_error("Root and Tip bone are not in the same chain");
            m_tipBoneName  = "";
            m_tipBone      = -1;
        }
        MakeConstraints();

        notify_property_list_changed();
    }
}
String LightIKPlugin::get_root_bone() const 
{
    return m_rootBoneName;
}

void LightIKPlugin::set_tip_bone(const String& tip_bone_name) 
{
    m_tipBoneName = tip_bone_name;

    // prevent construction of the chain constraints untill the skeleton is ready
    if (is_node_ready() && get_skeleton())
    {
        m_tipBone = get_skeleton()->find_bone(tip_bone_name);

        // if root bone was not found in the branch of cuurent tip bone, it must be reseted
        if (!UpdateBoneChain())
        {
            UtilityFunctions::push_error("Root and Tip bone are not in the same chain");
            m_rootBoneName  = "";
            m_rootBone      = -1;
        }
        MakeConstraints();

        notify_property_list_changed();
    }
}
String LightIKPlugin::get_tip_bone() const 
{
    return m_tipBoneName;
}

void LightIKPlugin::set_constraints_array(const TypedArray<JointConstraints>& array) 
{
    m_constraintsArray = array;
    UpdateVisualHelperData();
}
TypedArray<JointConstraints> LightIKPlugin::get_constraints_array() const 
{
    return m_constraintsArray; 
}

void LightIKPlugin::set_target(const NodePath& target_path) 
{
    m_targetPath = target_path;
}
NodePath LightIKPlugin::get_target() const 
{
    return m_targetPath;
}

LightIKPlugin::LightIKPlugin()
    : m_helper(memnew(VisualHelper))
{
    add_child(m_helper);
}

LightIKPlugin::~LightIKPlugin()
{

}

void LightIKPlugin::_ready()
{
    // on object initialization the skeleton doesn'o't exists, 
    // so parameters should be updated at the moment the object is constructed
    assert (get_skeleton());
    if (m_rootBoneName != "")
    {

        m_rootBone = get_skeleton()->find_bone(m_rootBoneName);
    }
    if (m_tipBoneName != "")
    {
        m_tipBone = get_skeleton()->find_bone(m_tipBoneName);
    }
    if (UpdateBoneChain())
    {
        UpdateVisualHelperData();
    }
}

void LightIKPlugin::_process(double delta)
{
    UpdateEditorData();
}

void LightIKPlugin::_validate_property(godot::PropertyInfo& info)
{
    if (info.name == String("skeleton_root_bone"))
    {
        ValidateRootBone(info);
    }
    else if(info.name == String("skeleton_tip_bone"))
    {
        ValidateTipBone(info);
    }
}

void LightIKPlugin::ValidateRootBone(PropertyInfo& info)
{
    info.hint = PROPERTY_HINT_ENUM;
    if (!get_skeleton())
    {
        return;
    }
    if (-1 == m_tipBone)
    {
        // if tip is not selected allow to chose any bone for root
        info.hint_string = get_skeleton()->get_concatenated_bone_names();
        return;
    }

    // list all parent bones from current tip to skeleton root
    int32_t bone = m_tipBone;
    info.hint_string = "";
    while (bone >= 0)
    {
        // inverse addition to the list to keep the right order of the bones, from root to current
        info.hint_string = get_skeleton()->get_bone_name(bone) + "," + info.hint_string;
        bone = get_skeleton()->get_bone_parent(bone);
    }
}

void LightIKPlugin::ValidateTipBone(PropertyInfo& info)
{
    if (!get_skeleton())
    {
        return;
    }

    info.hint = PROPERTY_HINT_ENUM;
    if (-1 == m_rootBone)
    {
        // if root is not selected allow to chose any bone for tip
        info.hint_string = get_skeleton()->get_concatenated_bone_names();
        return;
    }
    
    std::stack<int32_t> boneStack;
    // allow to chose any child bone from the selected root
    boneStack.emplace(m_rootBone);
    // create  the list of all child bones from current to all branch leaves
    // avoid recursion. create the stack of bones and process it.
    
    while (!boneStack.empty())
    {
        int32_t rootBone = boneStack.top();
        boneStack.pop();
        // concatenate names of child bones into the single list
        info.hint_string += get_skeleton()->get_bone_name(rootBone) + ",";

        // put all child bones to stack to process them later
        const auto& bones = get_skeleton()->get_bone_children(rootBone);
        for (int32_t bone : bones)
        {
            boneStack.emplace(bone);
        }
    }
}

bool LightIKPlugin::UpdateBoneChain()
{
    bool found = false;
    int32_t bone = m_tipBone;
    m_boneChain.clear();
    // go through all bones in backward direction from tip to root to form the bone chain
    while (bone >= 0)
    {
            // create the actual bone chain for simulation from tip to begin bone
        m_boneChain.emplace(m_boneChain.begin(), bone);
        if (bone == m_rootBone)
        {
            // ok, we found both tip and bone, so chain is complete
            found = true;
            break;
        }
        bone = get_skeleton()->get_bone_parent(bone);
    }
    // ok chain is incomplete, so cleanup all trash that we had put into the chain array
    if (!found)
    {
        m_boneChain.clear();
    }
    return found;
}

void LightIKPlugin::MakeConstraints()
{
    if (m_boneChain.empty())
    {
        UtilityFunctions::push_error("Bones ", m_rootBoneName, " and ", m_tipBoneName, " are not on the same branch.");
        return;
    }
    m_constraintsArray.resize(m_boneChain.size());
    UpdateVisualHelperData();
}

void LightIKPlugin::UpdateVisualHelperData()
{
    std::vector<Transform3D> bones;
    
    int32_t bone = m_tipBone;
    // collect position and original direction of all bones in the chain
    for (int32_t boneId : m_boneChain)
    {
        bones.emplace_back(get_skeleton()->get_bone_global_pose(boneId));
    }

    // send the array to visualizer
    m_helper->SetConstraintsInfo(m_constraintsArray, bones);
}

void LightIKPlugin::UpdateEditorData()
{
    bool updateRequired = false;
    for (size_t c = 0; c < m_constraintsArray.size(); ++c)
    {
        JointConstraints* constraint = Object::cast_to<JointConstraints>(m_constraintsArray[c]);
        updateRequired = updateRequired || (constraint && constraint->IsDirty());
    }
    if (updateRequired)
    {
        UpdateVisualHelperData();
    }
}

}