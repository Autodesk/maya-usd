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

#include <maya/MDagPath.h>
#include <maya/MMatrix.h>

namespace MHWRender {
class MRenderItem;
class MVertexBuffer;
class MIndexBuffer;
} // namespace MHWRender

PXR_NAMESPACE_OPEN_SCOPE

//! \brief  Minimal holdout depth-stamp pass (spike).
//!
//! Registers a pre-scene-render notification on stock Viewport 2.0. Before the
//! opaque scene draw, it stamps the DEPTH of every registered prim's geometry
//! into the shared depth buffer with all color channels masked. Registered
//! prims therefore occlude scene geometry behind them without contributing any
//! color -- a holdout -- folded into stock VP2 with no renderer selection.
//!
//! This increment proves drawing REAL prim geometry depth-only from the render
//! delegate's existing GPU buffers. Holdout detection and beauty-pass exclusion
//! are deliberately not here yet: mesh.cpp currently publishes every shaded
//! (triangle) render item.
namespace HdVP2HoldoutDepthPass {

//! Register/deregister the VP2 pre-scene-render notification. Call from the
//! Maya plugin's initialize/uninitialize.
void Register();
void Deregister();

//! Insert or update one render item's geometry for depth stamping. Buffer
//! pointers must stay valid until Unpublish; their GL resource handles are read
//! at draw time. \p key is the owning render item, used as a stable identity.
void Publish(
    const MHWRender::MRenderItem* key,
    MHWRender::MVertexBuffer*     positionBuffer,
    MHWRender::MIndexBuffer*      indexBuffer,
    unsigned int                 indexCount,
    const MMatrix&               worldMatrix,
    const MDagPath&              proxyDagPath);

//! Remove a render item from the registry. Call when the draw item is
//! destroyed so we never hold a dangling buffer pointer.
void Unpublish(const MHWRender::MRenderItem* key);

} // namespace HdVP2HoldoutDepthPass

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_VP2_HOLDOUT_DEPTH_PASS