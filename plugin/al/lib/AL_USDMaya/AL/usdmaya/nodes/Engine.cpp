//
// Copyright 2019 Animal Logic
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
#include "AL/usdmaya/nodes/Engine.h"

#include <pxr/imaging/hd/engine.h>
#include <pxr/imaging/hdx/pickTask.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <maya/MProfiler.h>

#include <vector>

namespace {
const int _enginProfilerCategory = MProfiler::addCategory(
#if MAYA_API_VERSION >= 20190000
    "GLEngine",
    "GLEngine"
#else
    "GLEngine"
#endif
);
} // namespace

namespace AL {
namespace usdmaya {
namespace nodes {

Engine::Engine(const SdfPath& rootPath, const SdfPathVector& excludedPaths)
    : UsdImagingGLEngine(rootPath, excludedPaths)
{
}

bool Engine::TestIntersectionBatch(
    const GfMatrix4d&        viewMatrix,
    const GfMatrix4d&        projectionMatrix,
    const GfMatrix4d&        worldToLocalSpace,
    const SdfPathVector&     paths,
    UsdImagingGLRenderParams params,
    const TfToken&           resolveMode,
    unsigned int             pickResolution,
    HitBatch*                outHit)
{
    MProfilingScope profilerScope(
        _enginProfilerCategory, MProfiler::kColorE_L3, "Test intersection batch");

    _UpdateHydraCollection(&_intersectCollection, paths, params);

    TfTokenVector renderTags;
    _ComputeRenderTags(params, &renderTags);
    _taskController->SetRenderTags(renderTags);

    HdxPickHitVector allHits;

    HdxRenderTaskParams hdParams = _MakeHydraUsdImagingGLRenderParams(params);
    _taskController->SetRenderParams(hdParams);

    HdxPickTaskContextParams pickParams;
    pickParams.resolution = GfVec2i(pickResolution, pickResolution);
    pickParams.resolveMode = resolveMode;
    pickParams.viewMatrix = worldToLocalSpace * viewMatrix;
    pickParams.projectionMatrix = projectionMatrix;
    pickParams.clipPlanes = params.clipPlanes;
    pickParams.collection = _intersectCollection;
    pickParams.outHits = &allHits;
    VtValue vtPickParams(pickParams);

#if defined(USDIMAGINGGL_API_VERSION) && USDIMAGINGGL_API_VERSION >= 6
    HdEngine* hdEngine = _GetHdEngine();
#else
    HdEngine* hdEngine = &_engine;
#endif

    hdEngine->SetTaskContextData(HdxPickTokens->pickParams, vtPickParams);
    auto pickingTasks = _taskController->GetPickingTasks();
    hdEngine->Execute(_taskController->GetRenderIndex(), &pickingTasks);

    if (allHits.size() == 0) {
        return false;
    }

    if (!outHit) {
        return true;
    }

    for (const auto& hit : allHits) {
        SdfPath primPath = hit.objectId;
        int     instanceIndex = hit.instanceIndex;

#if defined(USDIMAGINGGL_API_VERSION) && USDIMAGINGGL_API_VERSION >= 5
        // See similar code in usdImagingGL/engine.cpp...
        primPath = _GetSceneDelegate()->GetScenePrimPath(primPath, instanceIndex);
#elif defined(USDIMAGINGGL_API_VERSION) && USDIMAGINGGL_API_VERSION >= 3
        // See similar code in usdImagingGL/engine.cpp...
        primPath = _delegate->GetScenePrimPath(primPath, instanceIndex);
#else
        SdfPath resolvedPath = GetPrimPathFromInstanceIndex(primPath, instanceIndex);
        if (!resolvedPath.IsEmpty()) {
            primPath = resolvedPath;
        } else {
            primPath = primPath.StripAllVariantSelections();
        }
#endif

        (*outHit)[primPath] = hit.worldSpaceHitPoint;
    }

    return true;
}

} // namespace nodes
} // namespace usdmaya
} // namespace AL
