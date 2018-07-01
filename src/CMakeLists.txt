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
    hdmaya/adapters/adapter.cpp
    hdmaya/adapters/adapterDebugCodes.cpp
    hdmaya/adapters/adapterRegistry.cpp
    hdmaya/adapters/areaLightAdapter.cpp
    hdmaya/adapters/dagAdapter.cpp
    hdmaya/adapters/lightAdapter.cpp
    hdmaya/adapters/meshAdapter.cpp
    hdmaya/adapters/pointLightAdapter.cpp
    hdmaya/adapters/spotLightAdapter.cpp

    hdmaya/delegates/delegate.cpp
    hdmaya/delegates/delegateCtx.cpp
    hdmaya/delegates/delegateRegistry.cpp
    hdmaya/delegates/sceneDelegate.cpp
    hdmaya/delegates/testDelegate.cpp

    plugin/cmd.cpp
    plugin/plugin.cpp
    plugin/renderOverride.cpp
    )

add_library(${PLUGIN} SHARED ${SRC} ${HDR})
set_target_properties(${PLUGIN} PROPERTIES PREFIX "")
target_link_libraries(${PLUGIN} PRIVATE ${OPENGL_gl_LIBRARY} ${MAYA_LIBS} ${Boost_LIBRARIES} ${GLEW_LIBRARY})
target_link_libraries(${PLUGIN} PRIVATE usd usdGeom glf gf hd hdSt hdx usdImaging usdImagingGL usdMaya)
target_include_directories(${PLUGIN} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
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

install(FILES
        hdmaya/adapters/adapter.h
        hdmaya/adapters/adapterDebugCodes.h
        hdmaya/adapters/adapterRegistry.h
        hdmaya/adapters/constantShadowMatrix.h
        hdmaya/adapters/dagAdapter.h
        hdmaya/adapters/lightAdapter.h
        DESTINATION include/adapter)

install(FILES
        hdmaya/delegates/delegate.h
        hdmaya/delegates/delegateCtx.h
        hdmaya/delegates/delegateRegistry.h
        DESTINATION include)

install(FILES
        hdmaya/utils.h
        DESTINATION include)

# file(GLOB TEMPLATES scripts/*.py)
#
# install(FILES ${TEMPLATES}
#         DESTINATION scripts)