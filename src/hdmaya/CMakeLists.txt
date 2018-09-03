set(SRC
    adapters/adapter.cpp
    adapters/adapterDebugCodes.cpp
    adapters/adapterRegistry.cpp
    adapters/areaLightAdapter.cpp
    adapters/dagAdapter.cpp
    adapters/imagePlaneAdapter.cpp
    adapters/imagePlaneMaterialAdapter.cpp
    adapters/lightAdapter.cpp
    adapters/materialAdapter.cpp
    adapters/materialNetworkConverter.cpp
    adapters/mayaAttrs.cpp
    adapters/meshAdapter.cpp
    adapters/nurbsCurveAdapter.cpp
    adapters/pointLightAdapter.cpp
    adapters/shapeAdapter.cpp
    adapters/spotLightAdapter.cpp
    adapters/tokens.cpp

    delegates/delegate.cpp
    delegates/delegateCtx.cpp
    delegates/delegateDebugCodes.cpp
    delegates/delegateRegistry.cpp
    delegates/sceneDelegate.cpp
    delegates/testDelegate.cpp)

add_library(hdmaya SHARED ${SRC})
target_link_libraries(hdmaya PUBLIC ${OPENGL_gl_LIBRARY} ${MAYA_LIBS})
target_link_libraries(hdmaya PUBLIC ${Boost_LIBRARIES} ${GLEW_LIBRARY})
target_link_libraries(hdmaya PUBLIC arch gf plug tf vt)
target_link_libraries(hdmaya PUBLIC glf hd hdSt hdx hf pxOsd)
target_link_libraries(hdmaya PUBLIC usd sdf usdGeom usdLux)
target_link_libraries(hdmaya PUBLIC usdImaging usdImagingGL)
target_link_libraries(hdmaya PUBLIC usdMaya)
target_link_libraries(hdmaya PUBLIC ${TBB_LIBRARIES})
target_link_libraries(hdmaya PUBLIC ${PYTHON_LIBRARIES})
target_include_directories(hdmaya PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_include_directories(hdmaya PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(hdmaya SYSTEM PUBLIC ${MAYA_INCLUDE_DIRS} ${GLEW_INCLUDE_DIR})
target_include_directories(hdmaya SYSTEM PUBLIC ${USD_INCLUDE_DIR} ${USD_MAYA_INCLUDE_DIR})
target_include_directories(hdmaya SYSTEM PUBLIC ${Boost_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
target_include_directories(hdmaya SYSTEM PUBLIC ${TBB_INCLUDE_DIRS})
target_compile_features(hdmaya PUBLIC cxx_std_11)
target_compile_definitions(hdmaya PRIVATE "HDMAYA_EXPORT=1")
set_target_properties(hdmaya PROPERTIES INSTALL_RPATH_USE_LINK_PATH ON)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Prevent windows.h from defining min and max macros.
    target_compile_definitions(hdmaya PUBLIC "NOMINMAX=1")
endif ()

if (LUMA_USD_BUILD)
    target_compile_definitions(hdmaya PRIVATE LUMA_USD_BUILD)
endif ()

if (USD_HDST_SHADOWS_BUILD)
    target_compile_definitions(hdmaya PRIVATE USD_HDST_SHADOWS_BUILD)
endif ()

if (USD_HDST_UDIM_BUILD)
    target_compile_definitions(hdmaya PRIVATE USD_HDST_UDIM_BUILD)
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

set(PLUG_INFO_PATH "plugInfo.json")

configure_file(
    ${PLUG_INFO_PATH}
    ${CMAKE_CURRENT_BINARY_DIR}/${PLUG_INFO_PATH}
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${PLUG_INFO_PATH}
    DESTINATION lib)

install(
    TARGETS hdmaya
    DESTINATION lib)

install(
    FILES
        adapters/adapter.h
        adapters/adapterDebugCodes.h
        adapters/adapterRegistry.h
        adapters/constantShadowMatrix.h
        adapters/dagAdapter.h
        adapters/lightAdapter.h
        adapters/materialAdapter.h
        adapters/materialNetworkConverter.h
        adapters/mayaAttrs.h
        adapters/shapeAdapter.h
        adapters/tokens.h
    DESTINATION include/hdmaya/adapters)

install(
    FILES
        delegates/delegate.h
        delegates/delegateCtx.h
        delegates/delegateDebugCodes.h
        delegates/delegateRegistry.h
        delegates/params.h
    DESTINATION include/hdmaya/delegates)

install(
    FILES
        api.h
        utils.h
    DESTINATION include/hdmaya)
