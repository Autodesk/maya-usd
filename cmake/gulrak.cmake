#
# Copyright 2021 Autodesk
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

include(FetchContent)

set(CONTENT_NAME gulrak)

set(FETCHCONTENT_QUIET OFF)

# GULRAK_SOURCE_DIR : Set this to the directory where you have cloned gulrak filesystem repo, 
#                     if you would like to bypass pulling from Github repository via Internet.
if(DEFINED GULRAK_SOURCE_DIR)
    message(STATUS "**** Building Gulrak From " ${GULRAK_SOURCE_DIR})
    FetchContent_Declare(
        ${CONTENT_NAME}
        URL ${GULRAK_SOURCE_DIR}
    )

    string(TOUPPER ${CONTENT_NAME} UPPERGULARK)
    mark_as_advanced(FETCHCONTENT_SOURCE_DIR_${UPPERGULARK})
    mark_as_advanced(FETCHCONTENT_UPDATES_DISCONNECTED_${UPPERGULARK})

else()
    message(STATUS "**** Building Gulrak From Github Repository.")
    FetchContent_Declare(
        ${CONTENT_NAME}
        GIT_REPOSITORY https://github.com/gulrak/filesystem.git
        GIT_TAG        4e21ab305794f5309a1454b4ae82ab9a0f5e0d25
        USES_TERMINAL_DOWNLOAD TRUE
        GIT_CONFIG     advice.detachedHead=false
    )
endif()

FetchContent_MakeAvailable(${CONTENT_NAME})
