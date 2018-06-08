set(PLUGIN_NAME hd_viewport)

# In case we need boost
set(Boost_USE_STATIC_LIBS ON)

find_package(Maya)
find_package(OpenGL REQUIRED)
find_package(USD REQUIRED)
find_package(Boost COMPONENTS python REQUIRED)
find_package(PythonLibs REQUIRED)
find_package(GLEW REQUIRED)

link_directories(${MAYA_LIBRARY_DIR})
link_directories(${USD_LIBRARY_DIR})

set(MAYA_LIBS OpenMaya Foundation OpenMayaAnim OpenMayaFX OpenMayaUI OpenMayaRender)

file(GLOB SRC *.cpp)
file(GLOB HDR *.h)

add_library(${PLUGIN_NAME} SHARED ${SRC} ${HDR})
set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PLUGIN_NAME} PRIVATE ${OPENGL_gl_LIBRARY} ${MAYA_LIBS} ${Boost_LIBRARIES} ${GLEW_LIBRARY})
target_link_libraries(${PLUGIN_NAME} PRIVATE usd usdGeom glf gf hd hdSt hdx usdImaging usdImagingGL)
target_include_directories(${PLUGIN_NAME} SYSTEM PRIVATE ${MAYA_INCLUDE_DIRS})
target_include_directories(${PLUGIN_NAME} SYSTEM PRIVATE ${USD_INCLUDE_DIR} ${GLEW_INCLUDE_DIR})
target_include_directories(${PLUGIN_NAME} SYSTEM PRIVATE ${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})

install(TARGETS ${PLUGIN_NAME}
        DESTINATION plug-ins)

# file(GLOB TEMPLATES scripts/*.py)
#
# install(FILES ${TEMPLATES}
#         DESTINATION scripts)