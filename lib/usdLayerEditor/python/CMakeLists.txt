#
# Copyright 2026 Autodesk
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

# Python extension library for UsdLayerEditor library bindings. Pixar's
# TF_WRAP_MODULE macro expects the library name to be prefixed with _, but
# adds an underscore to the package name when creating the Python extension
# module initialization function (see Pixar's pyModule.h).
#
# This CMakeLists is invoked from lib/usdUfe/usd-layer-editor/CMakeLists.txt
# when BUILD_NEW_LAYER_EDITOR is ON. Python, boost.python, UFE and USD are
# already found/defined by the parent maya-usd build — do not call
# find_package here.

set(PACKAGE_NAME UsdLayerEditor)

if(IS_WINDOWS AND MAYAUSD_DEFINE_DEBUG_PYTHON_FLAG)
    # On Windows when compiling with debug python the library must be named with _d.
    set(PYTHON_TARGET_NAME "_${PACKAGE_NAME}_d")
else()
    set(PYTHON_TARGET_NAME "_${PACKAGE_NAME}")
endif()

add_library(${PYTHON_TARGET_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PYTHON_TARGET_NAME}
    PRIVATE
        module.cpp
        wrapUsdLayerEditor.cpp
)

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
target_compile_definitions(${PYTHON_TARGET_NAME}
    PRIVATE
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
        MFB_PACKAGE_NAME=${PACKAGE_NAME}
        MFB_ALT_PACKAGE_NAME=${PACKAGE_NAME}
        MFB_PACKAGE_MODULE=${PACKAGE_NAME}
)

mayaUsd_compile_config(${PYTHON_TARGET_NAME})

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${PYTHON_TARGET_NAME}
    PRIVATE
        UsdLayerEditorLib
        # UsdLayerEditorLib's public headers (layerEditorWidgetManager.h) expose
        # Qt types (QPointer), so the binding TU also needs Qt::Core headers.
        Qt::Core
)

# -----------------------------------------------------------------------------
# properties
# -----------------------------------------------------------------------------
set_python_module_property(${PYTHON_TARGET_NAME})

# -----------------------------------------------------------------------------
# run-time search paths
# -----------------------------------------------------------------------------
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "lib/python/${PACKAGE_NAME}")
    # back to <prefix>/lib so we can find UsdLayerEditorLib and usdUfe
    mayaUsd_add_rpath(rpath "../../..")
    if(IS_MACOSX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../../../../Maya.app/Contents/MacOS")
    endif()
    if(DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
        if (IS_LINUX)
            mayaUsd_add_rpath(rpath "../../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib64")
        endif()
    elseif(DEFINED PXR_USD_LOCATION)
        mayaUsd_add_rpath(rpath "${PXR_USD_LOCATION}/lib")
    endif()
    mayaUsd_install_rpath(rpath ${PYTHON_TARGET_NAME})
endif()

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
set(PYLIB_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/lib/python/${PACKAGE_NAME})

install(TARGETS ${PYTHON_TARGET_NAME}
    LIBRARY
        DESTINATION ${PYLIB_INSTALL_PREFIX}
    ARCHIVE
        DESTINATION ${PYLIB_INSTALL_PREFIX}
    RUNTIME
        DESTINATION ${PYLIB_INSTALL_PREFIX}
)

install(FILES __init__.py DESTINATION ${PYLIB_INSTALL_PREFIX})

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${PYTHON_TARGET_NAME}>
            DESTINATION ${PYLIB_INSTALL_PREFIX} OPTIONAL)
endif()
