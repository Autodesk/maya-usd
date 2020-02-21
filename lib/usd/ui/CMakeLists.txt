#
# =======================================================================
# Copyright 2019 Autodesk, Inc. All rights reserved.
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
# =======================================================================
#

project(mayaUsdUI)

if (POLICY CMP0074)
    cmake_policy(SET CMP0074 OLD)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_library(${PROJECT_NAME} SHARED
	cmds/USDImportDialogCmd.cpp
	factories/TreeModelFactory.cpp
	views/ItemDelegate.cpp
	views/TreeItem.cpp
	views/TreeModel.cpp
	views/USDImportDialog.cpp
	views/USDImportDialog.ui
)

list(APPEND mayaUsdUI_headers
	api.h
	cmds/USDImportDialogCmd.h
	factories/TreeModelFactory.h
	views/IUSDImportView.h
	views/ItemDelegate.h
	views/TreeItem.h
	views/TreeModel.h
	views/USDImportDialog.h
)

# Promote headers
mayaUsd_promoteHeaderList(SUBDIR "ui" HEADERS ${mayaUsdUI_headers})

target_compile_definitions(${PROJECT_NAME}
	PRIVATE
		$<$<BOOL:${IS_MACOSX}>:OSMac_>
		MAYAUSD_UI_EXPORT
)

target_include_directories(${PROJECT_NAME}
	PUBLIC
		${MAYA_INCLUDE_DIRS}
		${PXR_INCLUDE_DIRS}
		${CMAKE_BINARY_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		mayaUsd
		usd
		${MAYA_OpenMaya_LIBRARY}
		${MAYA_OpenMayaUI_LIBRARY}
		Qt5::Core
		Qt5::Gui
		Qt5::Widgets
)

set(MAYAUSDUI_INSTALL_PATH ${CMAKE_INSTALL_PREFIX}/lib)

if(IS_MACOSX OR IS_LINUX)
    # rpath setup
    mayaUsd_init_rpath(rpath "lib")
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
    mayaUsd_add_rpath(rpath ${MAYAUSDUI_INSTALL_PATH})
    mayaUsd_install_rpath(rpath ${PROJECT_NAME})
endif()

install(
	TARGETS ${PROJECT_NAME}
	LIBRARY
		DESTINATION ${MAYAUSDUI_INSTALL_PATH}
	ARCHIVE
		DESTINATION ${MAYAUSDUI_INSTALL_PATH}
	RUNTIME
		DESTINATION ${MAYAUSDUI_INSTALL_PATH}
)
if(IS_WINDOWS)
	install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ${MAYAUSDUI_INSTALL_PATH} OPTIONAL)
endif()

install(FILES ${mayaUsdUI_headers}
	DESTINATION ${CMAKE_INSTALL_PREFIX}/include/mayaUsd/ui/
)

#==============================================================================
# Tests
#==============================================================================
if (BUILD_TESTS)
	# A simple executable used to test the import dialog.
	add_subdirectory(demo)
endif()
