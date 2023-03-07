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
#include "pluginDebugCodes.h"

#include <pxr/base/tf/registryManager.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_RENDEROVERRIDE_DEFAULT_LIGHTING,
        "Print information detection of default lighting for the Maya VP2 "
        "RenderOverride.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_RENDEROVERRIDE_RENDER,
        "Print information about Maya VP2 RenderOverride render call.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_RENDEROVERRIDE_RESOURCES,
        "Print information about Maya VP2 RenderOverride creation, deletion, "
        "and resource usage.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_RENDEROVERRIDE_SELECTION,
        "Print information about selection for the Maya VP2 RenderOverride.");
}

PXR_NAMESPACE_CLOSE_SCOPE
