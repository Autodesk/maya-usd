set(PLUGIN hd_viewport)

# In case we need boost
# USD still needs the dynamic ones
# set(Boost_USE_STATIC_LIBS ON)

find_package(Maya)
find_package(OpenGL REQUIRED)
find_package(USD REQUIRED)
find_package(Boost COMPONENTS python REQUIRED)
find_package(PythonLibs REQUIRED)
find_package(GLEW REQUIRED)

link_directories(${MAYA_LIBRARY_DIR})
link_directories(${USD_LIBRARY_DIR})
link_directories(${USD_MAYA_LIBRARY_DIR})

set(MAYA_LIBS OpenMaya Foundation OpenMayaAnim OpenMayaFX OpenMayaUI OpenMayaRender)

set(SRC
    adapters/adapter.cpp
    adapters/adapterDebugCodes.cpp
    adapters/adapterRegistry.cpp
    adapters/areaLightAdapter.cpp
    adapters/dagAdapter.cpp
    adapters/lightAdapter.cpp
    adapters/meshAdapter.cpp
    adapters/pointLightAdapter.cpp
    adapters/spotLightAdapter.cpp

    delegates/delegate.cpp
    delegates/delegateCtx.cpp
    delegates/delegateRegistry.cpp
    delegates/sceneDelegate.cpp
    delegates/testDelegate.cpp

    cmd.cpp
    plugin.cpp
    renderOverride.cpp
    )

add_library(${PLUGIN} SHARED ${SRC} ${HDR})
set_target_properties(${PLUGIN} PROPERTIES PREFIX "")
target_link_libraries(${PLUGIN} PRIVATE ${OPENGL_gl_LIBRARY} ${MAYA_LIBS} ${Boost_LIBRARIES} ${GLEW_LIBRARY})
target_link_libraries(${PLUGIN} PRIVATE usd usdGeom glf gf hd hdSt hdx usdImaging usdImagingGL usdMaya)
target_include_directories(${PLUGIN} SYSTEM PRIVATE ${MAYA_INCLUDE_DIRS} ${GLEW_INCLUDE_DIR})
target_include_directories(${PLUGIN} SYSTEM PRIVATE ${USD_INCLUDE_DIR} ${USD_MAYA_INCLUDE_DIR})
target_include_directories(${PLUGIN} SYSTEM PRIVATE ${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
target_compile_features(${PLUGIN} PRIVATE cxx_std_11)
set_target_properties(${PLUGIN} PROPERTIES INSTALL_RPATH_USE_LINK_PATH ON)
if (LUMA_USD_BUILD)
    target_compile_definitions(${PLUGIN} PRIVATE LUMA_USD_BUILD)
endif ()

install(TARGETS ${PLUGIN}
        DESTINATION plug-ins)

# file(GLOB TEMPLATES scripts/*.py)
#
# install(FILES ${TEMPLATES}
#         DESTINATION scripts)