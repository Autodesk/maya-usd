#==============================================================================
# Configuration hdMaya 
#==============================================================================

set(TARGET_NAME hdMaya)

option(HDMAYA_BUILD_TESTS "Build the maya-to-hydra tests" OFF)

set(MTOH_CXX_STD "11"
    CACHE
    STRING
    "Which C++ standard / version to use, as an integer - ie, 14 means use c++14."
)

if(CMAKE_WANT_UFE_BUILD)
    if(UFE_FOUND)
        message(STATUS "Building with UFE ${UFE_VERSION} features enabled.")
        include_directories(${UFE_INCLUDE_DIR})
        add_definitions(-DWANT_UFE_BUILD)
    else()
        message(STATUS "UFE not found. UFE features will be disabled.")
    endif()
endif()

set(SRC
    adapters/adapter.cpp
    adapters/adapterDebugCodes.cpp
    adapters/adapterRegistry.cpp
    adapters/aiSkydomeLightAdapter.cpp
    adapters/areaLightAdapter.cpp
    adapters/dagAdapter.cpp
    adapters/directionalLightAdapter.cpp
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
    
    debugCodes.cpp
    utils.cpp)

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

if (UFE_FOUND)
    target_link_libraries(${TARGET_NAME} PUBLIC ${UFE_LIBRARY})
endif()

if (MTOH_CXX_STD MATCHES "^11$")
    target_compile_features(${TARGET_NAME} PUBLIC cxx_std_11)
elseif (MTOH_CXX_STD MATCHES "^14$")
    target_compile_features(${TARGET_NAME} PUBLIC cxx_std_14)
endif ()

target_compile_definitions(${TARGET_NAME} 
    PRIVATE 
        "HDMAYA_EXPORT=1"
)

if (IS_WINDOWS)
    # Prevent windows.h from defining min and max macros.
    target_compile_definitions(${TARGET_NAME} PUBLIC "NOMINMAX=1")
elseif (IS_MACOSX)
    target_compile_definitions(${TARGET_NAME} PUBLIC "OSMac_")
endif ()

set(libFileName ${CMAKE_SHARED_LIBRARY_PREFIX}${TARGET_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
set(PLUG_INFO_PATH "plugInfo.json")
set(PLUG_INFO_LIBRARY_PATH "../../../${libFileName}")
set(PLUG_INFO_RESOURCE_PATH "resources")

configure_file(
    ${PLUG_INFO_PATH}
    ${CMAKE_CURRENT_BINARY_DIR}/${PLUG_INFO_PATH}
)

#==============================================================================
# Install
#==============================================================================
install(TARGETS ${TARGET_NAME}
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    RUNTIME
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${PLUG_INFO_PATH}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/usd/hdMaya/resources)

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
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/hdMaya/adapters)

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
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/hdMaya/delegates)

install(
    FILES
        api.h
        utils.h
        debugCodes.h
        ${HDMAYA_H}

    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/hdMaya)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> DESTINATION ${CMAKE_INSTALL_PREFIX}/lib OPTIONAL)
endif()
