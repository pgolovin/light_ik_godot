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
    
static Vector3 FromLightIKVector(const LightIK::Vector& src)
{
    return Vector3{(real_t)src.x, (real_t)src.y, (real_t)src.z};
}

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

////////////////////////////////////////////// properties definition
void LightIKPlugin::set_simulate(const bool& animate) 
{
    m_simulate = animate;
    if (!m_simulate && get_skeleton())
    {
        get_skeleton()->clear_bones_global_pose_override();
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
    // no need to update target if object is not constructed or path is empty. just drop it to nullptr
    m_target = (!m_targetPath.is_empty() && is_node_ready()) ? get_node<Node3D>(m_targetPath) : nullptr;
}
NodePath LightIKPlugin::get_target() const 
{
    return m_targetPath;
}

LightIKPlugin::LightIKPlugin()
    : m_helper(memnew(VisualHelper))
{

}

LightIKPlugin::~LightIKPlugin()
{

}

////////////////////////////////////////////// godot interface
void LightIKPlugin::_ready()
{
    add_child(m_helper);
    m_helper->reparent(this);
    m_helper->set_owner(this);
    // on object initialization the skeleton doesn't exists, 
    // so parameters should be updated at the moment the object is fully constructed
    assert (get_skeleton());
    if (m_rootBoneName != "")
    {
        m_rootBone = get_skeleton()->find_bone(m_rootBoneName);
    }
    if (m_tipBoneName != "")
    {
        m_tipBone = get_skeleton()->find_bone(m_tipBoneName);
    }
    if (!m_targetPath.is_empty())
    {
        m_target = !m_targetPath.is_empty() ? get_node<Node3D>(m_targetPath) : nullptr;
    }

    if (UpdateBoneChain())
    {
        UpdateIKData();
        UpdateVisualHelperData();
    }
}

void LightIKPlugin::_process_modification()
{
    if (!is_inside_tree())
    {
        return;
    }

    Vector3 targetPosition = m_target ? get_skeleton()->get_global_transform().xform_inv(m_target->get_global_position()) : Vector3{0,0,0};

    m_lightIKCore.SetTargetPosition(LightIK::Vector(targetPosition.x, targetPosition.y, targetPosition.z));

    if (m_simulate && m_lightIKCore.UpdateChainPosition())
    {
        // update bone positions: 
        //TODO: get quaternion in local to parent bone coordinates... 
        std::vector<LightIK::Quaternion> overrides = m_lightIKCore.GetDeltaRotations();

        LightIK::Vector step1 = overrides.front() * LightIK::Vector{0,1,0};

        for (size_t b = 0; b < overrides.size(); ++b)
        {
            const auto& quat = overrides[b];
            Quaternion q{(real_t)quat.x, (real_t)quat.y, (real_t)quat.z, (real_t)quat.w};
            get_skeleton()->set_bone_pose_rotation(m_boneChain[b], q);

            UtilityFunctions::prints("position of bone", b,  get_skeleton()->get_bone_global_pose(m_boneChain[b]).origin);
            UtilityFunctions::prints("                ", b,  q);
        }
    }

    UpdateEditorData(true);
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

////////////////////////////////////////////// Editor functions

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

    assert(m_tipBone != m_rootBone);
    // list all parent bones from current tip (excluding) to skeleton root
    int32_t bone = get_skeleton()->get_bone_parent(m_tipBone);
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
    
    assert(m_tipBone != m_rootBone);
    std::stack<int32_t> boneStack;
    // allow to chose any child bone from the selected root
    const auto& bones = get_skeleton()->get_bone_children(m_rootBone);
    for (int32_t bone : bones)
    {
        boneStack.emplace(bone);
    }
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
    // the last bone is the tip. it is not used for calculations
    m_constraintsArray.resize(m_boneChain.size() - 1);
    UpdateIKData();
    UpdateVisualHelperData();
}

void LightIKPlugin::UpdateIKData()
{
    m_lightIKCore.Reset();
    if (m_boneChain.empty())
    {
        return;
    }
    Vector3 base    = get_skeleton()->get_bone_global_pose(m_boneChain.front()).origin;
    auto q          = get_skeleton()->get_bone_pose_rotation(m_boneChain.front());

    m_lightIKCore.SetRootPosition(LightIK::Vector(base.x, base.y, base.z));
    for (size_t b = 1; b < m_boneChain.size(); ++b)
    {        
        q               = get_skeleton()->get_bone_pose_rotation(m_boneChain[b - 1]);
        Vector3 origin  = get_skeleton()->get_bone_global_pose(m_boneChain[b]).origin;
        real_t length   = (origin - base).length();
        m_lightIKCore.AddBone(length, LightIK::Quaternion{ (LightIK::real)q.w, (LightIK::real)q.x, (LightIK::real)q.y, (LightIK::real)q.z});
        base            = origin;

        JointConstraints* constraint = Object::cast_to<JointConstraints>(m_constraintsArray[b - 1]);
        if (constraint)
        {
            LightIK::Constraints c{constraint->get_flexibility()};
            m_lightIKCore.SetConstraint(b - 1, std::move(c));
        }

    }
    m_lightIKCore.CompleteChain();
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
    m_helper->SetConstraintsInfo(m_constraintsArray, bones);
}

void LightIKPlugin::UpdateEditorData(bool updateRequired)
{
    for (size_t c = 0; c < m_constraintsArray.size(); ++c)
    {
        JointConstraints* constraint = Object::cast_to<JointConstraints>(m_constraintsArray[c]);
        updateRequired = updateRequired || (constraint && constraint->IsDirty());
    }
    
    // check if target position had been changed to draw the direction line
    Transform3D targetPosition = m_target ? m_target->get_global_transform() : Transform3D();
    m_helper->SetTargetPosition(get_skeleton()->get_global_transform(), targetPosition);

    // if array data was modified, request the update of constraints visualization
    if (updateRequired)
    {
        for (size_t i = 0; i < m_constraintsArray.size(); ++i)
        {
            JointConstraints* constraint = Object::cast_to<JointConstraints>(m_constraintsArray[i]);
            if (constraint)
            {
                LightIK::Constraints c{constraint->get_flexibility()};
                m_lightIKCore.SetConstraint(i, std::move(c));
            }
        }

        UpdateVisualHelperData();
    }
}

}