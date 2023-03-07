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
#include "debugCodes.h"

#include <pxr/base/tf/registryManager.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_AL_CALLBACKS, "Print info about the various callbacks used by hdMaya_al");
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_AL_PLUGIN, "Print info about the loading of the hdMaya_al plugin");
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_AL_POPULATE, "Print info about populating the delegate from the stage");
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_AL_PROXY_DELEGATE,
        "Print misc info about (ie, creation / destruction) of the delegate "
        "object");
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HDMAYA_AL_SELECTION, "Print info about selecting AL objects in the maya-to-hydra viewport");
}

PXR_NAMESPACE_CLOSE_SCOPE
