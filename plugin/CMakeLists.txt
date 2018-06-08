set(PLUGIN_NAME hd_viewport)

# In case we need boost
set(Boost_USE_STATIC_LIBS ON)

find_package(Maya)
find_package(OpenGL REQUIRED)

link_directories(${MAYA_LIBRARY_DIR})

set(MAYA_LIBS OpenMaya Foundation OpenMayaAnim OpenMayaFX OpenMayaUI OpenMayaRender)

file(GLOB SRC *.cpp)
file(GLOB HDR *.h)

add_library(${PLUGIN_NAME} SHARED ${SRC} ${HDR})
set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PLUGIN_NAME} ${OPENGL_gl_LIBRARY} ${MAYA_LIBS})
target_include_directories(${PLUGIN_NAME} PRIVATE ${MAYA_INCLUDE_DIRS})

install(TARGETS ${PLUGIN_NAME}
        DESTINATION plug-ins)

# file(GLOB TEMPLATES scripts/*.py)
#
# install(FILES ${TEMPLATES}
#         DESTINATION scripts)