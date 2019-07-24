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

include(gccclangshareddefaults)

set(_PXR_CXX_FLAGS "${_PXR_GCC_CLANG_SHARED_CXX_FLAGS}")

# clang annoyingly warns about the -pthread option if it's only linking.
if(CMAKE_USE_PTHREADS_INIT)
    _disable_warning("unused-command-line-argument")
endif()
