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

# This file contains a set of flags/settings shared between our 
# GCC and Clang configs. This allows clangdefaults and gccdefaults
# to remain minimal, marking the points where divergence is required.
include(Options)

# Turn on C++11; pxr won't build without it. 
set(_PXR_GCC_CLANG_SHARED_CXX_FLAGS "${_PXR_GCC_CLANG_SHARED_CXX_FLAGS} -std=c++11")

# Enable all warnings.
set(_PXR_GCC_CLANG_SHARED_CXX_FLAGS "${_PXR_GCC_CLANG_SHARED_CXX_FLAGS} -Wall")

# Errors are warnings in strict build mode.
if (${PXR_STRICT_BUILD_MODE})
    set(_PXR_GCC_CLANG_SHARED_CXX_FLAGS "${_PXR_GCC_CLANG_SHARED_CXX_FLAGS} -Werror")
endif()

# We use hash_map, suppress deprecation warning.
_disable_warning("deprecated")
_disable_warning("deprecated-declarations")

# Suppress unused typedef warnings emanating from boost.
if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
    NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.6)
    if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR
        NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.1)
            _disable_warning("unused-local-typedefs")
    endif()
endif()

if (${PXR_MAYA_TBB_BUG_WORKAROUND})
    set(_PXR_GCC_CLANG_SHARED_CXX_FLAGS "${_PXR_GCC_CLANG_SHARED_CXX_FLAGS} -Wl,-Bsymbolic")
endif()

# If using pthreads then tell the compiler.  This should automatically cause
# the linker to pull in the pthread library if necessary so we also clear
# PXR_THREAD_LIBS.
if(CMAKE_USE_PTHREADS_INIT)
    set(_PXR_GCC_CLANG_SHARED_CXX_FLAGS "${_PXR_GCC_CLANG_SHARED_CXX_FLAGS} -pthread")
    set(PXR_THREAD_LIBS "")
endif()
