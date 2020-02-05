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

project(testMayaUsdUI)

if (POLICY CMP0074)
    cmake_policy(SET CMP0074 OLD)
endif()

add_executable(${PROJECT_NAME}
	testMayaUsdUI.cpp
)

target_include_directories(${PROJECT_NAME}
	PUBLIC
		${CMAKE_BINARY_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
		# For now including this so we get boost path added.
		usd
		mayaUsdUI
		Qt5::Core
		Qt5::Gui
		Qt5::Widgets
)

if (IS_LINUX)
	# On Linux there are many 'undefined reference to' errors from the OpenMaya* libs.
	# This executable doesn't directly use Maya API and isn't installed. It is simply
	# used for debugging purposes during development.
	set_property(TARGET ${PROJECT_NAME}
		APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--unresolved-symbols=ignore-in-shared-libs")
endif()
