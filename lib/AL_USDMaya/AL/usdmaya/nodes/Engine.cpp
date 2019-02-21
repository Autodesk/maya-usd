//
// Copyright 2019 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.//
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
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "AL/usdmaya/nodes/Engine.h"

#include "pxr/imaging/hdx/intersector.h"
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
