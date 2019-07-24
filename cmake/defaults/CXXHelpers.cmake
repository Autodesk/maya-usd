#
# Copyright 2016 Pixar
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
function(_add_define definition)
    list(APPEND _PXR_CXX_DEFINITIONS "-D${definition}")
    set(_PXR_CXX_DEFINITIONS ${_PXR_CXX_DEFINITIONS} PARENT_SCOPE)
endfunction()

function(_disable_warning flag)
    if(MSVC)
        list(APPEND _PXR_CXX_WARNING_FLAGS "/wd${flag}")
    else()
        list(APPEND _PXR_CXX_WARNING_FLAGS "-Wno-${flag}")
    endif()
    set(_PXR_CXX_WARNING_FLAGS ${_PXR_CXX_WARNING_FLAGS} PARENT_SCOPE)
endfunction()
