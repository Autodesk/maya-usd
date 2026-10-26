//
// Copyright 2026 Autodesk
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
#ifndef HD_VP2_HOLDOUT_DEPTH_PASS
#define HD_VP2_HOLDOUT_DEPTH_PASS

#include <pxr/pxr.h>

#include <mayaUsd/base/api.h>

#include <maya/MDagPath.h>
#include <maya/MMatrix.h>

namespace MHWRender {
class MRenderItem;
class MVertexBuffer;
class MIndexBuffer;
} // namespace MHWRender

PXR_NAMESPACE_OPEN_SCOPE

//! \brief  Holdout / matte rendering for USD prims in stock Viewport 2.0.
//!
//! A prim tagged with the constant "maya:holdout" primvar becomes a holdout: it
//! is not shaded itself, it occludes CG behind it, and it reveals the camera's
//! background image plane(s) through its silhouette. This is folded into the
//! normal VP2 draw via a pre-scene-render notification -- no separate renderer
//! override -- so it coexists with AA/DOF/SSAO, background, grid, HUD,
//! manipulators, selection and playblast.
//!
//! At begin-scene the pass stamps the holdout geometry's DEPTH (so the scene's
//! own depth test hides CG behind it) and composites the image plane(s) as its
//! color. Multiple image planes are composited back-to-front by their depth,
//! using each EXR's alpha; planes in front of the holdout are left to Maya's
//! native image-plane draw. mesh.cpp detects holdouts, suppresses their beauty
//! draw, and publishes their geometry here.
//!
//! This pass issues raw OpenGL and interprets Maya resource handles as GL names,
//! so it is only installed when the viewport draw API is OpenGL.
namespace HdVP2HoldoutDepthPass {

//! Register/deregister the VP2 pre-scene-render notification. Call from the
//! Maya plugin's initialize/uninitialize. Exported: called from the separate
//! plugin module. No-op when the draw API is not OpenGL.
MAYAUSD_CORE_PUBLIC
void Register();

MAYAUSD_CORE_PUBLIC
void Deregister();

//! Insert or update one render item's geometry for depth stamping. Buffer
//! pointers must stay valid until Unpublish; their GL resource handles are read
//! at draw time. \p key is the owning render item, used as a stable identity.
void Publish(
    const MHWRender::MRenderItem* key,
    MHWRender::MVertexBuffer*     positionBuffer,
    MHWRender::MIndexBuffer*      indexBuffer,
    unsigned int                  indexCount,
    const MMatrix&                worldMatrix,
    const MDagPath&               proxyDagPath,
    bool                          visible);

//! Update a published holdout's prim visibility. Cheap; safe to call on every
//! visibility change (no-op if the item is not currently published).
void SetVisible(const MHWRender::MRenderItem* key, bool visible);

//! Remove a render item from the registry. Call when the draw item is
//! destroyed so we never hold a dangling buffer pointer.
void Unpublish(const MHWRender::MRenderItem* key);

} // namespace HdVP2HoldoutDepthPass

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_VP2_HOLDOUT_DEPTH_PASS