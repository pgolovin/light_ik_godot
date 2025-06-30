
set(PLUGIN_HEADERS
    "src/helpers.h"
    "src/light_ik_plugin.h"
    "src/joint_constraints.h"
    "src/register_types.h"
    "src/visual_helper.h"
)

set(PLUGIN_SRC
    "src/light_ik_plugin.cpp"
    "src/joint_constraints.cpp"
    "src/register_types.cpp"
    "src/visual_helper.cpp"
)
find_package(glm REQUIRED)

add_library(light_ik_plugin SHARED ${PLUGIN_SRC} ${PLUGIN_HEADERS})

target_include_directories(light_ik_plugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE ./src)
target_link_libraries(light_ik_plugin 
                        PRIVATE glm::glm
                        PUBLIC light_ik 
                        PUBLIC godot-cpp)

add_custom_command(TARGET light_ik_plugin COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/plugin/bin/)
add_custom_command(TARGET light_ik_plugin POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src/plugin.gdextension ${PROJECT_SOURCE_DIR}/plugin/bin/)
add_custom_command(TARGET light_ik_plugin POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:light_ik_plugin> ${PROJECT_SOURCE_DIR}/plugin/bin/)