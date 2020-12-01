//
// Copyright 2016 Pixar
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
    TF_DEBUG_ENVIRONMENT_SYMBOL(PXRUSDMAYA_REGISTRY, "UsdMaya registration for usd types.");
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        PXRUSDMAYA_DIAGNOSTICS, "Debugging of the the diagnostics batching system in UsdMaya.");
    TF_DEBUG_ENVIRONMENT_SYMBOL(PXRUSDMAYA_TRANSLATORS, "Debugging of translators.");
    TF_DEBUG_ENVIRONMENT_SYMBOL(USDMAYA_PROXYSHAPEBASE, "Base proxy shape evaluation");
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        USDMAYA_PROXYACCESSOR, "Debugging of the evaluation for mixed data models.");
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        USDMAYA_PLUG_INFO_VERSION, "Debugging of the mayaUsd plug info version check.");
}

PXR_NAMESPACE_CLOSE_SCOPE
