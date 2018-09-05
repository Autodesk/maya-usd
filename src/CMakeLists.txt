# In case we need boost, USD still needs the dynamic ones.
# This is the default value.
# set(Boost_USE_STATIC_LIBS OFF)

find_package(Maya)
find_package(OpenGL REQUIRED)
find_package(USD REQUIRED)
find_package(Boost COMPONENTS python REQUIRED)
find_package(PythonLibs REQUIRED)
find_package(GLEW REQUIRED)
find_package(TBB REQUIRED)

link_directories(${MAYA_LIBRARY_DIR})
link_directories(${USD_LIBRARY_DIR})
link_directories(${USD_MAYA_LIBRARY_DIR})

set(MAYA_LIBS OpenMaya Foundation OpenMayaAnim OpenMayaFX OpenMayaUI OpenMayaRender)

set(HDMAYA_SRC
    hdmaya/adapters/mayaAttrs.cpp
    hdmaya/adapters/adapter.cpp
    hdmaya/adapters/adapterDebugCodes.cpp
    hdmaya/adapters/adapterRegistry.cpp
    hdmaya/adapters/areaLightAdapter.cpp
    hdmaya/adapters/dagAdapter.cpp
    hdmaya/adapters/imagePlaneAdapter.cpp
    hdmaya/adapters/imagePlaneMaterialAdapter.cpp
    hdmaya/adapters/lightAdapter.cpp
    hdmaya/adapters/materialAdapter.cpp
    hdmaya/adapters/meshAdapter.cpp
    hdmaya/adapters/nurbsCurveAdapter.cpp
    hdmaya/adapters/pointLightAdapter.cpp
    hdmaya/adapters/shapeAdapter.cpp
    hdmaya/adapters/spotLightAdapter.cpp

    hdmaya/delegates/delegate.cpp
    hdmaya/delegates/delegateCtx.cpp
    hdmaya/delegates/delegateDebugCodes.cpp
    hdmaya/delegates/delegateRegistry.cpp
    hdmaya/delegates/sceneDelegate.cpp
    hdmaya/delegates/testDelegate.cpp)

set(HDVIEW_SRC
    hdmaya/adapters/mayaAttrs.cpp
    plugin/plugin.cpp
    plugin/renderOverride.cpp
    plugin/usdPreviewSurface.cpp
    plugin/viewCommand.cpp)

add_library(hdmaya SHARED ${HDMAYA_SRC})
target_link_libraries(hdmaya PUBLIC ${OPENGL_gl_LIBRARY} ${MAYA_LIBS} ${Boost_LIBRARIES} ${GLEW_LIBRARY})
target_link_libraries(hdmaya PUBLIC usd usdGeom glf gf hd hdSt hdx usdImaging usdImagingGL sdf usdMaya)
target_include_directories(hdmaya PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(hdmaya SYSTEM PUBLIC ${MAYA_INCLUDE_DIRS} ${GLEW_INCLUDE_DIR})
target_include_directories(hdmaya SYSTEM PUBLIC ${USD_INCLUDE_DIR} ${USD_MAYA_INCLUDE_DIR})
target_include_directories(hdmaya SYSTEM PUBLIC ${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
target_include_directories(hdmaya SYSTEM PUBLIC ${TBB_INCLUDE_DIRS})
target_compile_features(hdmaya PUBLIC cxx_std_11)
target_compile_definitions(hdmaya PRIVATE "HDMAYA_EXPORT=1")
set_target_properties(hdmaya PROPERTIES INSTALL_RPATH_USE_LINK_PATH ON)

add_library(hdview SHARED ${HDVIEW_SRC})
target_link_libraries(hdview PRIVATE hdmaya)
set_target_properties(hdview PROPERTIES PREFIX "")
set_target_properties(hdview PROPERTIES INSTALL_RPATH_USE_LINK_PATH ON)
set_target_properties(hdview PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")

# M3dView needs this (technically, MNativeWindowHdl.h)
# private atm because M3dView is only used in .cpp right now
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # gcc will NOT set this automatically
    target_compile_definitions(hdview PRIVATE "LINUX")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Not sure if msvcc sets this automatically, but won't hurt to redefine
    target_compile_definitions(hdview PRIVATE "_WIN32")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # Not sure if osx sets this automatically, but won't hurt to redefine
    target_compile_definitions(hdview PRIVATE "OSMac_")
endif (CMAKE_SYSTEM_NAME STREQUAL "Linux")

if (LUMA_USD_BUILD)
    target_compile_definitions(hdmaya PRIVATE LUMA_USD_BUILD)
    target_compile_definitions(hdview PRIVATE LUMA_USD_BUILD)
endif ()

if (USD_HDST_SHADOWS_BUILD)
    target_compile_definitions(hdmaya PRIVATE USD_HDST_SHADOWS_BUILD)
    target_compile_definitions(hdview PRIVATE USD_HDST_SHADOWS_BUILD)
endif ()

if (USD_HDST_UDIM_BUILD)
    target_compile_definitions(hdmaya PRIVATE USD_HDST_UDIM_BUILD)
    target_compile_definitions(hdview PRIVATE USD_HDST_UDIM_BUILD)
endif ()

# Wanted to do this:
#   get_target_property(LIB_FULLPATH hdmaya LOCATION)
#   get_filename_component(LIB_NAME ${LIB_FULLPATH} NAME)
#   set(PLUG_INFO_LIBRARY_PATH ${LIB_NAME})
# But apparently you can't look up LOCATION in cmake-2.8.0... nor can you use
# the generator expression "$<TARGET_FILE_NAME:hdmaya>" outside of special
# commands, like add_custom_command...
# So just building "manually" with PREFIX / SUFFIX
get_target_property(LIB_PREFIX hdmaya PREFIX)
if (LIB_PREFIX STREQUAL LIB_PREFIX-NOTFOUND OR LIB_PREFIX STREQUAL NOTFOUND)
    set(LIB_PREFIX ${CMAKE_SHARED_LIBRARY_PREFIX})
endif (LIB_PREFIX STREQUAL LIB_PREFIX-NOTFOUND OR LIB_PREFIX STREQUAL NOTFOUND)

get_target_property(LIB_SUFFIX hdmaya SUFFIX)
if (LIB_SUFFIX STREQUAL LIB_SUFFIX-NOTFOUND OR LIB_SUFFIX STREQUAL NOTFOUND)
    set(LIB_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
endif (LIB_SUFFIX STREQUAL LIB_SUFFIX-NOTFOUND OR LIB_SUFFIX STREQUAL NOTFOUND)

set(PLUG_INFO_LIBRARY_PATH ${LIB_PREFIX}hdmaya${LIB_SUFFIX})

set(PLUG_INFO_PATH "hdmaya/plugInfo.json")

configure_file(
    ${PLUG_INFO_PATH}
    ${CMAKE_CURRENT_BINARY_DIR}/${PLUG_INFO_PATH}
)

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
        hdmaya/delegates/delegateDebugCodes.h
        hdmaya/delegates/delegateRegistry.h
        hdmaya/delegates/params.h
        DESTINATION include/hdmaya/delegates)

install(FILES
        hdmaya/api.h
        hdmaya/utils.h
        DESTINATION include/hdmaya)
        
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PLUG_INFO_PATH}
        DESTINATION lib)
