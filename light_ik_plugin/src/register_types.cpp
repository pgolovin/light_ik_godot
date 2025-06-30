#include "register_types.h"
#include <gdextension_interface.h>

#include "light_ik_plugin.h"
#include "joint_constraints.h"
#include "visual_helper.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

const godot::ModuleInitializationLevel pluginLevel = MODULE_INITIALIZATION_LEVEL_SCENE;

void initialize_example_module(ModuleInitializationLevel p_level) 
{
 
    if (p_level != pluginLevel) 
    {
        return;
    }

    GDREGISTER_CLASS(LightIKPlugin);
    GDREGISTER_CLASS(JointConstraints);
    GDREGISTER_INTERNAL_CLASS(VisualHelper);
}

void uninitialize_example_module(ModuleInitializationLevel p_level) 
{
    if (p_level != pluginLevel) 
    {
        return;
    }
}

extern "C" {
// Initialization.
GDExtensionBool GDE_EXPORT light_ik_library_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address, 
    const GDExtensionClassLibraryPtr p_library, 
    GDExtensionInitialization *r_initialization) 
{
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_example_module);
    init_obj.register_terminator(uninitialize_example_module);
    init_obj.set_minimum_library_initialization_level(pluginLevel);   

    return init_obj.init();
}

}