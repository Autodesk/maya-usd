set(TARGET_NAME hdmaya)

if (LUMA_USD_BUILD)
    set(HDMAYA_LUMA_BUILD_INT 1)
else ()
    set(HDMAYA_LUMA_BUILD_INT 0)
endif ()

set(HDMAYA_H "${CMAKE_CURRENT_BINARY_DIR}/hdmaya.h")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/hdmaya.h.in"
    ${HDMAYA_H}
    @ONLY
)

set(SRC
    "${HDMAYA_H}"
    adapters/adapter.cpp
    adapters/adapterDebugCodes.cpp
    adapters/adapterRegistry.cpp
    adapters/aiSkydomeLightAdapter.cpp
    adapters/areaLightAdapter.cpp
    adapters/dagAdapter.cpp
    adapters/directionalLightAdapter.cpp
    adapters/imagePlaneAdapter.cpp
    adapters/imagePlaneMaterialAdapter.cpp
    adapters/lightAdapter.cpp
    adapters/proxyAdapter.cpp
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
    delegates/proxyDelegate.cpp
	delegates/proxyUsdImagingDelegate.cpp
    delegates/testDelegate.cpp
	
	debugCodes.cpp)

add_library(${TARGET_NAME} SHARED ${SRC})

target_link_libraries(${TARGET_NAME} PUBLIC 
    ${OPENGL_gl_LIBRARY} 
    ${MAYA_LIBRARIES}
    ${Boost_LIBRARIES} 
    ${GLEW_LIBRARY}
    arch 
    gf 
    plug
    tf
    vt
    glf
    hd
    hdSt
    hdx
    hf
    pxOsd
    usd
    sdf
    sdr
    usdGeom
    usdLux
    usdHydra
    usdImaging
    usdImagingGL
    ${LIBRARY_NAME}
    ${TBB_LIBRARIES}
    ${PYTHON_LIBRARIES}
)

if (USD_001905_BUILD)
    target_link_libraries(${TARGET_NAME} PUBLIC hio)
endif ()

target_include_directories(${TARGET_NAME} 
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        "${CMAKE_CURRENT_BINARY_DIR}/.."
    SYSTEM PUBLIC
        ${MAYA_INCLUDE_DIRS} 
        ${GLEW_INCLUDE_DIR}
        ${Boost_INCLUDE_DIRS}
        ${PYTHON_INCLUDE_DIRS}
        ${TBB_INCLUDE_DIRS}
        ${CMAKE_BINARY_DIR}
)

if (HDMAYA_UFE_BUILD)
    target_link_libraries(${TARGET_NAME} PUBLIC ${UFE_LIBRARY})
endif()

if (MTOH_CXX_STD MATCHES "^11$")
    target_compile_features(${TARGET_NAME} PUBLIC cxx_std_11)
elseif (MTOH_CXX_STD MATCHES "^14$")
    target_compile_features(${TARGET_NAME} PUBLIC cxx_std_14)
endif ()

target_compile_definitions(${TARGET_NAME} PRIVATE "HDMAYA_EXPORT=1")
set_target_properties(${TARGET_NAME} PROPERTIES INSTALL_RPATH_USE_LINK_PATH ON)
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_target_properties(${TARGET_NAME} PROPERTIES INSTALL_NAME_DIR "@loader_path/../lib")
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Prevent windows.h from defining min and max macros.
    target_compile_definitions(${TARGET_NAME} PUBLIC "NOMINMAX=1")
elseif (APPLE)
    target_compile_definitions(${TARGET_NAME} PUBLIC "OSMac_")
endif ()

if (LUMA_USD_BUILD)
    target_compile_definitions(${TARGET_NAME} PRIVATE LUMA_USD_BUILD)
endif ()

# Wanted to do this:
#   get_target_property(LIB_FULLPATH hdmaya LOCATION)
#   get_filename_component(LIB_NAME ${LIB_FULLPATH} NAME)
#   set(PLUG_INFO_LIBRARY_PATH ${LIB_NAME})
# But apparently you can't look up LOCATION in cmake-2.8.0... nor can you use
# the generator expression "$<TARGET_FILE_NAME:hdmaya>" outside of special
# commands, like add_custom_command...
# So just building "manually" with PREFIX / SUFFIX
get_target_property(LIB_PREFIX ${TARGET_NAME} PREFIX)
if (LIB_PREFIX STREQUAL LIB_PREFIX-NOTFOUND OR LIB_PREFIX STREQUAL NOTFOUND)
    set(LIB_PREFIX ${CMAKE_SHARED_LIBRARY_PREFIX})
endif (LIB_PREFIX STREQUAL LIB_PREFIX-NOTFOUND OR LIB_PREFIX STREQUAL NOTFOUND)

get_target_property(LIB_SUFFIX ${TARGET_NAME} SUFFIX)
if (LIB_SUFFIX STREQUAL LIB_SUFFIX-NOTFOUND OR LIB_SUFFIX STREQUAL NOTFOUND)
    set(LIB_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
endif (LIB_SUFFIX STREQUAL LIB_SUFFIX-NOTFOUND OR LIB_SUFFIX STREQUAL NOTFOUND)

set(PLUG_INFO_LIBRARY_PATH ${LIB_PREFIX}hdmaya${LIB_SUFFIX})

set(PLUG_INFO_PATH "plugInfo.json")

configure_file(
    ${PLUG_INFO_PATH}
    ${CMAKE_CURRENT_BINARY_DIR}/${PLUG_INFO_PATH}
)

#==============================================================================
# Install
#==============================================================================
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${PLUG_INFO_PATH}
    DESTINATION lib/usd/hdmaya/resources)

install(
    TARGETS ${TARGET_NAME}
    DESTINATION lib)

install(
    FILES
        adapters/adapter.h
        adapters/adapterDebugCodes.h
        adapters/adapterRegistry.h
        adapters/constantShadowMatrix.h
        adapters/dagAdapter.h
        adapters/lightAdapter.h
        adapters/proxyAdapter.h
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
        delegates/proxyDelegate.h
		delegates/proxyUsdImagingDelegate.h
        delegates/sceneDelegate.h
    DESTINATION include/hdmaya/delegates)

install(
    FILES
        api.h
        utils.h
		debugCodes.h
        ${HDMAYA_H}

    DESTINATION include/hdmaya)

if(MSVC)
    install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> DESTINATION ${CMAKE_INSTALL_PREFIX}/lib OPTIONAL)
endif()