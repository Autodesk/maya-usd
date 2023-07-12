set(TARGET_NAME mayaHydraLib)

add_library(${TARGET_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME} 
    PRIVATE
        debugCodes.cpp
        interfaceImp.cpp
        mayaUtils.cpp
        utils.cpp
)

set(HEADERS
    api.h
    debugCodes.h
    interface.h
    interfaceImp.h
    mayaUtils.h
    utils.h
)

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
target_compile_definitions(${TARGET_NAME} 
    PRIVATE 
        MAYAHYDRALIB_EXPORT
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
        # Copy-pasted from lib/mayaUsd/CMakeLists.txt
)

mayaUsd_compile_config(${TARGET_NAME})

if(NOT BUILD_MAYAUSD_LIBRARY)
    # -----------------------------------------------------------------------------
    # include directories
    # -----------------------------------------------------------------------------
    target_include_directories(${TARGET_NAME} 
        PUBLIC
            ${MAYA_INCLUDE_DIRS}
            ${PYTHON_INCLUDE_DIR}
            ${PXR_INCLUDE_DIRS}
            ${BOOST_INCLUDE_DIR}
            ${CMAKE_BINARY_DIR}/include
        PRIVATE
            $<$<BOOL:${UFE_FOUND}>:${UFE_INCLUDE_DIR}>
    )

    target_compile_definitions(${TARGET_NAME}
        PUBLIC
            PXR_PLUGINPATH_NAME=${PXR_OVERRIDE_PLUGINPATH_NAME}
            $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:TBB_USE_DEBUG>
            $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_DEBUG_PYTHON>
            $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_LINKING_PYTHON>
        PRIVATE
            MAYAUSD_MACROS_EXPORT
            MAYAUSD_CORE_EXPORT
            MFB_PACKAGE_NAME="${PROJECT_NAME}"
            MFB_ALT_PACKAGE_NAME="${PROJECT_NAME}"
            MFB_PACKAGE_MODULE="${PROJECT_NAME}"
            $<$<BOOL:${IS_MACOSX}>:OSMac_>
            $<$<BOOL:${IS_LINUX}>:LINUX>
            $<$<BOOL:${IS_LINUX}>:GL_GLEXT_PROTOTYPES>
            $<$<BOOL:${IS_LINUX}>:GLX_GLXEXT_PROTOTYPES>
            $<$<BOOL:${CMAKE_WANT_MATERIALX_BUILD}>:WANT_MATERIALX_BUILD>
            $<$<BOOL:${BUILD_MAYAHYDRALIB}>:BUILD_MAYAHYDRALIB>

            # this flag is needed when building maya-usd in Maya
            $<$<BOOL:${IS_WINDOWS}>:WIN32>
    )
endif()

if(DEFINED MAYAUSD_VERSION)
    target_compile_definitions(${TARGET_NAME}
        PRIVATE
            MAYAUSD_VERSION=${MAYAUSD_VERSION}
    )
endif()
if(DEFINED MAYAHYDRA_VERSION)
    target_compile_definitions(${TARGET_NAME}
        PRIVATE
            MAYAHYDRA_VERSION=${MAYAHYDRA_VERSION}
    )
endif()
if(DEFINED MAYAHYDRA_CUT_ID)
    target_compile_definitions(${TARGET_NAME}
        PRIVATE
            MAYAHYDRA_CUT_ID=${MAYAHYDRA_CUT_ID}
    )
endif()

    # -----------------------------------------------------------------------------
    # link libraries
    # -----------------------------------------------------------------------------
if(BUILD_MAYAUSD_LIBRARY)
    target_link_libraries(${TARGET_NAME} 
        PUBLIC 
            mayaUsd
        PRIVATE
            $<$<BOOL:${USD_001905_BUILD}>:hio>
    )
else()    
    target_link_libraries(${TARGET_NAME} 
        PUBLIC
            ar
            gf
            hd
            hdx
            js
            kind
            plug
            sdf
            tf
            trace
            usd
            usdGeom
            usdImaging
            usdImagingGL
            usdLux
            usdRender
            usdRi
            usdShade
            usdSkel
            usdUtils
            ufeExtensions
            ${MAYA_LIBRARIES}
        PRIVATE
            $<$<BOOL:${USD_001905_BUILD}>:hio>
            ${PYTHON_LIBRARIES} 
            $<$<BOOL:${UFE_FOUND}>:${UFE_LIBRARY}>
    )
endif()

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------
set(SRCFILE ${CMAKE_CURRENT_SOURCE_DIR}/mayaHydra.h.src)
set(DSTFILE ${CMAKE_BINARY_DIR}/include/mayaHydraLib/mayaHydra.h)
configure_file(${SRCFILE} ${DSTFILE})

mayaUsd_promoteHeaderList( 
    HEADERS
        ${HEADERS}
    BASEDIR
        ${TARGET_NAME}
)

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
set(LIBFILENAME ${CMAKE_SHARED_LIBRARY_PREFIX}${TARGET_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
set(PLUG_INFO_PATH "plugInfo.json")
set(SHADER_DEFS_PATH 
"shaderDefs.usda"
)

set(PLUG_INFO_LIBRARY_PATH "../../../${LIBFILENAME}")
set(PLUG_INFO_RESOURCE_PATH "resources")

configure_file(${PLUG_INFO_PATH} ${CMAKE_CURRENT_BINARY_DIR}/${PLUG_INFO_PATH})

# configure shader files
foreach(X ${SHADER_DEFS_PATH})
    configure_file(${X} ${CMAKE_CURRENT_BINARY_DIR}/${X})
endforeach(X)


install(TARGETS ${TARGET_NAME}
        LIBRARY
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        RUNTIME
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

install(FILES 
            ${CMAKE_CURRENT_BINARY_DIR}/${PLUG_INFO_PATH}
        DESTINATION 
            ${CMAKE_INSTALL_PREFIX}/lib/usd/mayaHydraLib/resources
)

# install shaders
foreach(X ${SHADER_DEFS_PATH})
    install(FILES 
            ${CMAKE_CURRENT_BINARY_DIR}/${X}
        DESTINATION 
            ${CMAKE_INSTALL_PREFIX}/lib/usd/mayaHydraLib/resources
    )
endforeach(X)


install(FILES ${HEADERS}
        DESTINATION 
            ${CMAKE_INSTALL_PREFIX}/include/mayaHydraLib
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> 
            DESTINATION ${CMAKE_INSTALL_PREFIX}/lib OPTIONAL)
endif()

# -----------------------------------------------------------------------------
# subdirectories
# -----------------------------------------------------------------------------
add_subdirectory(adapters)
add_subdirectory(delegates)
add_subdirectory(sceneIndex)
