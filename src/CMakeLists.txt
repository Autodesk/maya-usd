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

set(HDMAYA_SRC
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
    hdmaya/delegates/testDelegate.cpp)

set(HDVIEW_SRC
    plugin/cmd.cpp
    plugin/plugin.cpp
    plugin/renderOverride.cpp)

add_library(hdmaya SHARED ${HDMAYA_SRC})
target_link_libraries(hdmaya PUBLIC ${OPENGL_gl_LIBRARY} ${MAYA_LIBS} ${Boost_LIBRARIES} ${GLEW_LIBRARY})
target_link_libraries(hdmaya PUBLIC usd usdGeom glf gf hd hdSt hdx usdImaging usdImagingGL usdMaya)
target_include_directories(hdmaya PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(hdmaya SYSTEM PUBLIC ${MAYA_INCLUDE_DIRS} ${GLEW_INCLUDE_DIR})
target_include_directories(hdmaya SYSTEM PUBLIC ${USD_INCLUDE_DIR} ${USD_MAYA_INCLUDE_DIR})
target_include_directories(hdmaya SYSTEM PUBLIC ${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
target_compile_features(hdmaya PUBLIC cxx_std_11)
target_compile_definitions(hdmaya PRIVATE "HDMAYA_EXPORT=1")
set_target_properties(hdmaya PROPERTIES INSTALL_RPATH_USE_LINK_PATH ON)

add_library(hdview SHARED ${HDVIEW_SRC})
target_link_libraries(hdview PRIVATE hdmaya)
set_target_properties(hdview PROPERTIES PREFIX "")
set_target_properties(hdview PROPERTIES INSTALL_RPATH_USE_LINK_PATH ON)
set_target_properties(hdview PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")

if (LUMA_USD_BUILD)
    target_compile_definitions(hdmaya PRIVATE LUMA_USD_BUILD)
    target_compile_definitions(hdview PRIVATE LUMA_USD_BUILD)
endif ()

install(TARGETS hdmaya
        DESTINATION lib)

install(TARGETS hdview
        DESTINATION plug-ins)

install(FILES
        hdmaya/adapters/adapter.h
        hdmaya/adapters/adapterDebugCodes.h
        hdmaya/adapters/adapterRegistry.h
        hdmaya/adapters/constantShadowMatrix.h
        hdmaya/adapters/dagAdapter.h
        hdmaya/adapters/lightAdapter.h
        DESTINATION include/hdmaya/adapters)

install(FILES
        hdmaya/delegates/delegate.h
        hdmaya/delegates/delegateCtx.h
        hdmaya/delegates/delegateRegistry.h
        hdmaya/delegates/params.h
        DESTINATION include/hdmaya/delegates)

install(FILES
        hdmaya/api.h
        hdmaya/utils.h
        DESTINATION include/hdmaya)

# file(GLOB TEMPLATES scripts/*.py)
#
# install(FILES ${TEMPLATES}
#         DESTINATION scripts)