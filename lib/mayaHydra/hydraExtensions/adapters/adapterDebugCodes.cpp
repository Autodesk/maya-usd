//
// Copyright 2019 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "adapterDebugCodes.h"

#include <pxr/base/tf/registryManager.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_ADAPTER_CALLBACKS, "Print information adding and removal of adapter callbacks.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_ADAPTER_CURVE_PLUG_DIRTY,
        "Print information when a nurbs curve prim is dirtied due to a plug "
        "being dirtied.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_ADAPTER_CURVE_UNHANDLED_PLUG_DIRTY,
        "Print information when a nurbs curve prim is NOT dirtied, even though "
        "a plug was dirtied.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_ADAPTER_DAG_HIERARCHY, "Print information related to dag hierarchy changes.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_ADAPTER_DAG_PLUG_DIRTY, "Print information about the dag node plug dirtying.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_ADAPTER_GET, "Print information about 'Get' calls to the adapter.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_ADAPTER_GET_LIGHT_PARAM_VALUE,
        "Print information about 'LightParamValue' "
        "calls to the light adapters.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_ADAPTER_IMAGEPLANES, "Print information about drawing image planes.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_ADAPTER_LIGHT_SHADOWS, "Print information about shadow rendering.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_ADAPTER_MATERIALS, "Print information about converting materials.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_ADAPTER_MESH_PLUG_DIRTY, "Print information about the mesh plug dirtying handled.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_ADAPTER_MESH_UNHANDLED_PLUG_DIRTY,
        "Print information about unhandled mesh plug dirtying.");
}

PXR_NAMESPACE_CLOSE_SCOPE
