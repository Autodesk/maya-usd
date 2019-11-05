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
#include <vector>
#include "AL/usdmaya/nodes/Engine.h"

#if (PXR_MAJOR_VERSION > 0) || (PXR_MINOR_VERSION >= 19 && PXR_PATCH_VERSION >= 7) 
#include "pxr/imaging/hdx/pickTask.h"
#endif
#include "pxr/imaging/hdx/taskController.h"

namespace AL {
namespace usdmaya {
namespace nodes {

Engine::Engine(const SdfPath& rootPath, const SdfPathVector& excludedPaths)
  : UsdImagingGLEngine(rootPath, excludedPaths) {}

bool Engine::TestIntersectionBatch(
  const GfMatrix4d &viewMatrix,
  const GfMatrix4d &projectionMatrix,
  const GfMatrix4d &worldToLocalSpace,
  const SdfPathVector& paths,
  UsdImagingGLRenderParams params,
  unsigned int pickResolution,
  PathTranslatorCallback pathTranslator,
  HitBatch *outHit) {
  if (ARCH_UNLIKELY(_legacyImpl)) {
    return false;
  }

#if (PXR_MAJOR_VERSION > 0) || (PXR_MINOR_VERSION >= 19 && PXR_PATCH_VERSION >= 7) 
  _UpdateHydraCollection(&_intersectCollection, paths, params);

  TfTokenVector renderTags;
  _ComputeRenderTags(params, &renderTags);
  _taskController->SetRenderTags(renderTags);

  HdxPickHitVector allHits;

  HdxRenderTaskParams hdParams = _MakeHydraUsdImagingGLRenderParams(params);
  _taskController->SetRenderParams(hdParams);


  HdxPickTaskContextParams pickParams;
  pickParams.resolution = GfVec2i(pickResolution, pickResolution);
  pickParams.hitMode = HdxPickTokens->hitAll;
  pickParams.resolveMode = HdxPickTokens->resolveUnique;
  pickParams.viewMatrix = worldToLocalSpace * viewMatrix;
  pickParams.projectionMatrix = projectionMatrix;
  pickParams.clipPlanes = params.clipPlanes;
  pickParams.collection = _intersectCollection;
  pickParams.outHits = &allHits;
  VtValue vtPickParams(pickParams);

  _engine.SetTaskContextData(HdxPickTokens->pickParams, vtPickParams);
  auto pickingTasks = _taskController->GetPickingTasks();
  _engine.Execute(_taskController->GetRenderIndex(), &pickingTasks);

  if (allHits.size() == 0) {
    return false;
  }
  #else 

  _UpdateHydraCollection(&_intersectCollection, paths, params, &_renderTags);
  
  HdxIntersector::HitVector allHits;
  HdxIntersector::Params qparams;
  qparams.viewMatrix = worldToLocalSpace * viewMatrix;
  qparams.projectionMatrix = projectionMatrix;
  qparams.alphaThreshold = params.alphaThreshold;
  switch (params.cullStyle) {
    case UsdImagingGLCullStyle::CULL_STYLE_NO_OPINION:
      qparams.cullStyle = HdCullStyleDontCare;
      break;
    case UsdImagingGLCullStyle::CULL_STYLE_NOTHING:
      qparams.cullStyle = HdCullStyleNothing;
      break;
    case UsdImagingGLCullStyle::CULL_STYLE_BACK:
      qparams.cullStyle = HdCullStyleBack;
      break;
    case UsdImagingGLCullStyle::CULL_STYLE_FRONT:
      qparams.cullStyle = HdCullStyleFront;
      break;
    case UsdImagingGLCullStyle::CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED:
      qparams.cullStyle = HdCullStyleBackUnlessDoubleSided;
      break;
    default:
      qparams.cullStyle = HdCullStyleDontCare;
  }
  qparams.renderTags = _renderTags;
  qparams.enableSceneMaterials = params.enableSceneMaterials;

  _taskController->SetPickResolution(pickResolution);
  if (!_taskController->TestIntersection(
      &_engine,
      _intersectCollection,
      qparams,
      HdxIntersectionModeTokens->unique,
      &allHits)) {
    return false;
  }


  #endif

  if (!outHit) {
    return true;
  }

  for (const auto& hit : allHits) {
    const SdfPath primPath = hit.objectId;
    const SdfPath instancerPath = hit.instancerId;
    const int instanceIndex = hit.instanceIndex;

    HitInfo& info = (*outHit)[pathTranslator(primPath, instancerPath,
                                             instanceIndex)];
    info.worldSpaceHitPoint = GfVec3d(hit.worldSpaceHitPoint[0],
                                      hit.worldSpaceHitPoint[1],
                                      hit.worldSpaceHitPoint[2]);
    info.hitInstanceIndex = instanceIndex;
  }

  return true;
}

}
}
}
