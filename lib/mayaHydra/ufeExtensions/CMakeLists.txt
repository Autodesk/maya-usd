set(TARGET_NAME ufeExtensions)

add_library(${TARGET_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME} 
    PRIVATE
		Global.cpp
)

set(HEADERS
    api.h
	Global.h
)

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
target_compile_definitions(${TARGET_NAME} 
    PRIVATE 
        UFEEXTENSIONS_EXPORT
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
)

mayaUsd_compile_config(${TARGET_NAME})

if(NOT BUILD_MAYAUSD_LIBRARY)
    # -----------------------------------------------------------------------------
    # include directories
    # -----------------------------------------------------------------------------
    target_include_directories(${TARGET_NAME} 
        PUBLIC
            ${MAYA_INCLUDE_DIRS}
            ${CMAKE_BINARY_DIR}/include
            $<$<BOOL:${UFE_FOUND}>:${UFE_INCLUDE_DIR}>
    )

    target_compile_definitions(${TARGET_NAME}
        PUBLIC
            PXR_PLUGINPATH_NAME=${PXR_OVERRIDE_PLUGINPATH_NAME}
            $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:TBB_USE_DEBUG>
            $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_DEBUG_PYTHON>
            $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_LINKING_PYTHON>
        PRIVATE
            MFB_PACKAGE_NAME="${PROJECT_NAME}"
            MFB_ALT_PACKAGE_NAME="${PROJECT_NAME}"
            MFB_PACKAGE_MODULE="${PROJECT_NAME}"
            $<$<BOOL:${IS_MACOSX}>:OSMac_>
            $<$<BOOL:${IS_LINUX}>:LINUX>
            $<$<BOOL:${BUILD_MAYAHYDRALIB}>:BUILD_MAYAHYDRALIB>

            # this flag is needed when building maya-usd in Maya
            $<$<BOOL:${IS_WINDOWS}>:WIN32>
    )
endif()

if(DEFINED MAYAHYDRA_VERSION)
    target_compile_definitions(${TARGET_NAME}
        PRIVATE
            MAYAHYDRA_VERSION=${MAYAHYDRA_VERSION}
    )
endif()

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------

target_link_libraries(${TARGET_NAME} 
	PUBLIC
		sdf
		tf
		usd
		${MAYA_LIBRARIES}
	PRIVATE
        $<$<BOOL:${UFE_FOUND}>:${UFE_LIBRARY}>
		$<$<BOOL:${USD_001905_BUILD}>:hio>
		${PYTHON_LIBRARIES} 
)

# -----------------------------------------------------------------------------
# promoted headers
# -----------------------------------------------------------------------------

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

install(TARGETS ${TARGET_NAME}
        LIBRARY
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
        RUNTIME
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)

install(FILES ${HEADERS}
        DESTINATION 
            ${CMAKE_INSTALL_PREFIX}/include/ufeExtensions
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> 
            DESTINATION ${CMAKE_INSTALL_PREFIX}/lib OPTIONAL)
endif()
