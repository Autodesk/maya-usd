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
option(PXR_STRICT_BUILD_MODE "Turn on additional warnings. Enforce all warnings as errors." ON)
option(PXR_VALIDATE_GENERATED_CODE "Validate script generated code" OFF)
option(PXR_BUILD_TESTS "Build tests" ON)
option(PXR_ENABLE_GL_SUPPORT "Enable OpenGL based components" ON)
option(PXR_ENABLE_PYTHON_SUPPORT "Enable Python based components for USD" ON)
option(PXR_MAYA_TBB_BUG_WORKAROUND "Turn on linker flag (-Wl,-Bsymbolic) to work around a Maya TBB bug" OFF)
option(PXR_ENABLE_NAMESPACES "Enable C++ namespaces." ON)
option(BUILD_SHARED_LIBS "Build shared libraries." ON)
option(PXR_BUILD_MONOLITHIC "Build a monolithic library." OFF)

set(PXR_INSTALL_LOCATION ""
    CACHE
    STRING
    "Intended final location for plugin resource files."
)

set(PXR_OVERRIDE_PLUGINPATH_NAME ""
    CACHE
    STRING
    "Name of the environment variable that will be used to get plugin paths."
)

set(PXR_ALL_LIBS ""
    CACHE
    INTERNAL
    "Aggregation of all built libraries."
)
set(PXR_STATIC_LIBS ""
    CACHE
    INTERNAL
    "Aggregation of all built explicitly static libraries."
)
set(PXR_CORE_LIBS ""
    CACHE
    INTERNAL
    "Aggregation of all built core libraries."
)
set(PXR_OBJECT_LIBS ""
    CACHE
    INTERNAL
    "Aggregation of all core libraries built as OBJECT libraries."
)

option(BUILD_SHARED_LIBS "Build shared libraries." ON)
option(PXR_BUILD_MONOLITHIC "Build a monolithic library." OFF)
set(PXR_MONOLITHIC_IMPORT ""
    CACHE
    STRING
    "Path to cmake file that imports a usd_ms target"
)

set(PXR_EXTRA_PLUGINS ""
    CACHE
    INTERNAL
    "Aggregation of extra plugin directories containing a plugInfo.json.")
