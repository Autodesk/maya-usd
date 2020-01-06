#
# Copyright 2019 Autodesk
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

set(TARGET_NAME mayaUsdPlugin)

#==============================================================================
# packages
#==============================================================================
if(CMAKE_WANT_UFE_BUILD)
    find_package(UFE QUIET)
    if(UFE_FOUND)
        message(STATUS "Building with UFE ${UFE_VERSION} features enabled.")
    else()
        message(STATUS "UFE not found. UFE features will be disabled.")
    endif()
endif()

#==============================================================================
# target
#==============================================================================
add_library(${TARGET_NAME} MODULE)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME}
    PRIVATE
        plugin.cpp
        ProxyShape.cpp
)

# -----------------------------------------------------------------------------
# compile definitions
# -----------------------------------------------------------------------------
target_compile_definitions(${TARGET_NAME}
    PRIVATE
        MAYAUSD_PLUGIN_EXPORT
        $<$<BOOL:${UFE_FOUND}>:WANT_UFE_BUILD>
)

# -----------------------------------------------------------------------------
# include directories
# -----------------------------------------------------------------------------
target_include_directories(${TARGET_NAME}
    PRIVATE
        ${MAYAUSD_INCLUDE_DIR}
        ${PXR_INCLUDE_DIRS}
        $<$<BOOL:${UFE_FOUND}>:${UFE_INCLUDE_DIR}>
)

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${TARGET_NAME}
    PRIVATE
        ${MAYA_LIBRARIES}
        mayaUsd
        ar
        gf
        kind
        plug
        sdf
        tf
        usd
        usdGeom
        usdUtils
)

MAYA_SET_PLUGIN_PROPERTIES(${TARGET_NAME})

# handle rpath
if(IS_MACOSX OR IS_LINUX) 
    mayaUsd_init_rpath(rpath "plugin")
    if(WANT_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../../USD/lib")
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib")
    mayaUsd_install_rpath(rpath ${TARGET_NAME})
endif()

if(IS_LINUX)
    if(WANT_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../../USD/lib64")
    endif()
endif()

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(TARGETS
    ${TARGET_NAME}
    LIBRARY
        DESTINATION ${INSTALL_DIR_SUFFIX}/plugin
    RUNTIME
        DESTINATION ${INSTALL_DIR_SUFFIX}/plugin
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> DESTINATION ${INSTALL_DIR_SUFFIX}/plugin OPTIONAL)
endif()
