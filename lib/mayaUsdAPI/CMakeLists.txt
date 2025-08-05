#
# Copyright 2023 Autodesk
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
project(mayaUsdAPI)

add_library(${PROJECT_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME} 
    PRIVATE
        contextOps.cpp
        global.cpp
        proxyStage.cpp
        proxyShapeNotice.cpp
        render.cpp
        ufeHandlers.cpp
        undo.cpp
        utils.cpp
)

set(HEADERS
    api.h
    contextOps.h
    global.h
    proxyStage.h
    ufeHandlers.h
    proxyShapeNotice.h
    render.h
    undo.h
    utils.h
)

if (UFE_CLIPBOARD_SUPPORT)
    target_sources(${PROJECT_NAME}
        PRIVATE
            clipboard.cpp
    )
    list(APPEND HEADERS
        clipboard.h
    )

    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            UFE_CLIPBOARD_SUPPORT=1
    )
endif()

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
target_compile_definitions(${PROJECT_NAME} 
    PUBLIC
        # Needed by Pixar's wrap_python.hpp
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_DEBUG_PYTHON>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_LINKING_PYTHON>
    PRIVATE
        MAYAUSD_API_EXPORT
        MFB_PACKAGE_NAME="${PROJECT_NAME}"
        MFB_ALT_PACKAGE_NAME="${PROJECT_NAME}"
        MFB_PACKAGE_MODULE="${PROJECT_NAME}"
        $<$<BOOL:${CMAKE_WANT_MATERIALX_BUILD}>:WANT_MATERIALX_BUILD>
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
        $<$<BOOL:${IS_LINUX}>:LINUX>
        # this flag is needed when building for Maya
        $<$<BOOL:${IS_WINDOWS}>:WIN32>
)

# When the cmake var is set also set another cmake var for the corresponding
# define. So in mayaUsdAPI.h.src we can configure it correctly. This is used
# when building the MayaUsdAPI project and for any clients that are using it.
if (CMAKE_WANT_MATERIALX_BUILD)
    set(WANT_MATERIALX_BUILD ON)
endif()

set(SRCFILE ${CMAKE_CURRENT_SOURCE_DIR}/mayaUsdAPI.h.src)
set(DSTFILE ${CMAKE_BINARY_DIR}/include/mayaUsdAPI/mayaUsdAPI.h)
if (NOT EXISTS ${DSTFILE})
    message(STATUS "promoting: " ${SRCFILE})
endif()
configure_file(${SRCFILE} ${DSTFILE})

mayaUsd_compile_config(${PROJECT_NAME})

mayaUsd_promoteHeaderList( 
    HEADERS
        ${HEADERS}
    BASEDIR
        ${PROJECT_NAME}
)

# -----------------------------------------------------------------------------
# include directories
# -----------------------------------------------------------------------------
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PXR_INCLUDE_DIRS}
        ${MAYA_INCLUDE_DIRS}
        ${CMAKE_BINARY_DIR}/include
)

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        tf
    PRIVATE
        sdf
        mayaUsd
)

# -----------------------------------------------------------------------------
# run-time search paths
# -----------------------------------------------------------------------------
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "lib")
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    if(IS_LINUX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib64")
    endif()
    if(IS_MACOSX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../Maya.app/Contents/MacOS")
    endif()
    mayaUsd_install_rpath(rpath ${PROJECT_NAME})
endif()

#------------------------------------------------------------------------------
# install
#------------------------------------------------------------------------------
install(FILES  ${HEADERS}
    DESTINATION 
        ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}
)

set(LIBRARY_INSTALL_PATH
    ${CMAKE_INSTALL_PREFIX}/lib
)

install(
    TARGETS ${PROJECT_NAME}
    LIBRARY
    DESTINATION ${LIBRARY_INSTALL_PATH}
    ARCHIVE
    DESTINATION ${LIBRARY_INSTALL_PATH}
    RUNTIME
    DESTINATION ${LIBRARY_INSTALL_PATH}
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> 
        DESTINATION ${LIBRARY_INSTALL_PATH} OPTIONAL
    )
endif()

# We need an empty __init__.py file in each subfolder so it is considered a python module.
install(FILES __init__.py DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME})
install(FILES ${CMAKE_BINARY_DIR}/include/${PROJECT_NAME}/mayaUsdAPI.h
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME})


add_subdirectory(python)
