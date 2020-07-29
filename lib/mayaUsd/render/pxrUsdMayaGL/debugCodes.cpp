//
// Copyright 2018 Pixar
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

#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/registryManager.h>
#include <pxr/pxr.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        PXRUSDMAYAGL_BATCHED_DRAWING, "Prints out batched drawing event info.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        PXRUSDMAYAGL_BATCHED_SELECTION, "Prints out batched selection event info.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        PXRUSDMAYAGL_INSTANCER_TRACKING,
        "Reports when the instancer imager starts and stops tracking "
        "instancers.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING,
        "Reports on changes in the sets of shape adapters registered with the "
        "batch renderer.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(
        PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE, "Report Maya Hydra shape adapter lifecycle events.");
}

PXR_NAMESPACE_CLOSE_SCOPE
