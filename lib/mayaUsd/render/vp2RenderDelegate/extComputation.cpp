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
// Modifications copyright (C) 2019 Autodesk
//
#include "extComputation.h"

#include <pxr/base/tf/staticTokens.h>
#include <pxr/imaging/hd/sceneDelegate.h>

PXR_NAMESPACE_OPEN_SCOPE

HdVP2ExtComputation::HdVP2ExtComputation(SdfPath const& id)
    : HdExtComputation(id)
{
}

HdVP2ExtComputation::~HdVP2ExtComputation() { }

void HdVP2ExtComputation::Sync(
    HdSceneDelegate* sceneDelegate,
    HdRenderParam*   renderParam,
    HdDirtyBits*     dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdExtComputation::_Sync(sceneDelegate, renderParam, dirtyBits);

    TF_DEBUG(pxr::HD_EXT_COMPUTATION_UPDATED)
        .Msg(
            "HdVP2ExtComputation::Sync for %s (dirty bits = 0x%x)\n",
            GetId().GetText(),
            *dirtyBits);

    if (!(*dirtyBits & DirtySceneInput)) {
        // No scene inputs to sync. All other computation dirty bits (barring
        // DirtyCompInput) are sync'd in HdExtComputation::_Sync.
        return;
    }

    // Force pre-compute of skinning transforms to work around USD concurrency issue,
    // see https://github.com/PixarAnimationStudios/USD/issues/1742

    // Only do the input evaluation once, the first time the compute is sync.
    // Some of the inputs vary from frame to frame and preparing them here is
    // serial (compute is an sprim) while preparing them from the related rprim
    // sync is parallel.
    if (_evaluatedInputs)
        return;
    for (pxr::TfToken const& inputName : GetSceneInputNames()) {
        sceneDelegate->GetExtComputationInput(GetId(), inputName);
    }

    _evaluatedInputs = true;
}

PXR_NAMESPACE_CLOSE_SCOPE
