#pragma once

#include "helpers.h"
#include "joint_constraints.h"
#include "light_ik/light_ik.h"
#include <godot_cpp/classes/resource.hpp>

#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/node_path.hpp>

namespace godot
{

class LightIKPlugin;

/// @brief Base class for IK chain resource, the resource is represented as Virtual calss so it cannot be created
class BoneChain : public Resource
{  
    GDCLASS(BoneChain, Resource)  
    DEFINE_PROPERTY(String, root_bone);
    DEFINE_PROPERTY(String, tip_bone);
    
public:  
    void _validate_property(godot::PropertyInfo& info); 
    void _ready(Skeleton3D* solver);
    
    bool IsReady() const                { return m_skeleton;            }
    bool IsDirty();

    const String& GetRootBone() const   { return m_rootBoneName;        }
    const String& GetTipBone()  const   { return m_tipBoneName;         }

protected:  
    void ValidateRootBone(PropertyInfo& info);
    void ValidateTipBone(PropertyInfo& info);
    void SetDirty(bool dirty)           { m_dirty = dirty || m_dirty;   }
    
    static void _bind_methods();

    Skeleton3D*             m_skeleton = nullptr;
    
    String                  m_rootBoneName;
    String                  m_tipBoneName;
    bool                    m_dirty = false;
};

/// @brief standard IK chain that can target any coordinate in the scene
class ChainIKTarget final : public BoneChain
{
    GDCLASS(ChainIKTarget, BoneChain)
    DEFINE_PROPERTY(NodePath, target);
public:
    const NodePath& GetTargetPath() const { return m_targetPath;};

protected:
    static void _bind_methods();
    
    NodePath                m_targetPath;    
};

/// @brief standard IK chain that can target any coordinate in the scene
class ChainIKBoneLink final : public BoneChain
{
    GDCLASS(ChainIKBoneLink, BoneChain)
    DEFINE_PROPERTY(String, target_bone);
public:
    void _validate_property(godot::PropertyInfo& info);
    
    const String& GetTargetBone()  const { return m_targetBoneName; }
protected:
    static void _bind_methods();

    String m_targetBoneName;
};

}