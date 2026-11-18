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
#ifndef HDVP2_GAUSSIAN_SPLAT_VIEW_CACHE_H
#define HDVP2_GAUSSIAN_SPLAT_VIEW_CACHE_H

// The view-dependent state of one Gaussian splat Rprim: its shader clone, the data it last
// published, the buffers the pre-draw callback rewrites and the per-view slots. Every change of
// that state goes through one of the event methods below.
//
// Threading: main thread only (commit lambdas, pre-draw callback, Rprim destructor).

#include "gaussianSplatViewCore.h"

#include <mayaUsd/mayaUsd.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/vec2i.h>
#include <pxr/pxr.h>

#include <maya/MDrawContext.h>
#include <maya/MHWGeometry.h>
#include <maya/MShaderManager.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE
class HdVP2ResourceRegistry;
PXR_NAMESPACE_CLOSE_SCOPE

namespace MAYAUSD_NS_DEF {

/*! \brief  View-dependent cache of one splat Rprim.
    \class  GaussianSplatViewCache

    The Rprim holds it through a shared pointer that its commit lambdas capture, so it outlives
    the Rprim until the last pending lambda has run. Those lambdas check IsAlive() first.

    The pre-draw callback is a plain function pointer with no user data: it finds the cache of
    the shader it draws with through Find(). A cache is registered under its shader by
    EnsureShader() and unregistered by OnRprimDestroyed().
*/
class GaussianSplatViewCache
{
public:
    GaussianSplatViewCache() = default;
    GaussianSplatViewCache(const GaussianSplatViewCache&) = delete;
    GaussianSplatViewCache& operator=(const GaussianSplatViewCache&) = delete;

    //! The cache registered for a shader, null if none.
    static GaussianSplatViewCache* Find(const MHWRender::MShaderInstance* shader);

    //! The first call creates the shader clone (see CreateSplatShaderClone) and registers this
    //! cache under it. Later calls return it. A failed creation is not retried. Returns null on
    //! failure.
    MHWRender::MShaderInstance* EnsureShader(const std::string& colorSpace);

    //! The shader clone, null before EnsureShader() or after a failure.
    MHWRender::MShaderInstance* Shader() const { return _shader; }

    //! New data synced by the Rprim. Published when it is not the data already published: every
    //! slot is then stale and the buffers hold no slot. Null is ignored.
    void OnDataSynced(std::shared_ptr<const GaussianSplatViewCore::SplatData> data);

    //! A draw item committed its geometry. The color buffer is the one shared by the items of the
    //! Rprim. The index buffer is added to the ones the draw rewrites when it is new.
    //! `indexResized`: the item sized the index buffer again (identity order), so the buffers
    //! no longer hold any slot.
    void OnBuffersCommitted(
        MHWRender::MVertexBuffer* colorBuffer,
        MHWRender::MIndexBuffer*  indexBuffer,
        bool                      indexResized);

    //! Sets the tint uniforms of the shader clone. Warns and returns false on failure.
    bool SetTint(bool enabled, const float color[4]);

    //! A draw with the shader clone (pre-draw callback). For the beauty pass only: finds or
    //! computes the slot of the view and rewrites the shared buffers when they do not hold it.
    void OnDraw(const MHWRender::MDrawContext& context);

    /*! \brief  The Rprim is being destroyed.

        Unregisters the cache, so that a callback firing afterwards finds nothing. The clone is
        released by a later commit, on the main thread: the draw items of the Rprim are owned by
        HdRprim::_reprs, which is destroyed after the members of the Rprim class (rprim.h), and
        the items are removed from the container by ~HdVP2DrawItem. Releasing here would free a
        shader that a render item still references.
    */
    void OnRprimDestroyed(PXR_NS::HdVP2ResourceRegistry& registry);

    //! False once the Rprim is destroyed: pending commit lambdas must do nothing.
    bool IsAlive() const { return !_destroyed; }

private:
    //! Slots kept per Rprim, the least recently used one is dropped for a new view.
    static constexpr size_t kMaxSlots = 6;

    //! The order and colors of one view, computed for the same camera.
    struct Slot
    {
        std::string           cameraPath;
        PXR_NS::GfVec2i       viewportSize;
        PXR_NS::GfMatrix4d    viewMatrix;
        uint64_t              dataVersion = 0; //!< 0: holds nothing valid
        uint64_t              lastUse = 0;
        uint64_t              uid = 0;
        uint64_t              revision = 0; //!< bumped at each recomputation
        std::vector<uint32_t> order;        //!< splat indices, back to front
        std::vector<float>    colors;       //!< `order.size()*4` floats, same camera as `order`
    };

    //! Inputs of one draw. An empty camera path means the camera could not be determined.
    struct View
    {
        std::string        cameraPath;
        PXR_NS::GfVec2i    viewportSize = PXR_NS::GfVec2i(0);
        PXR_NS::GfMatrix4d viewMatrix = PXR_NS::GfMatrix4d(1.0); //!< world to camera
    };

    //! Reads the view of the draw. Any failing call leaves the camera unknown.
    static View _ReadView(const MHWRender::MDrawContext& context);

    //! The slot to draw for `view`: found or created (evicting the least recently used one),
    //! then recomputed when its data version or view matrix is stale. An unknown camera gets the
    //! most recently used slot that is valid for the current data. Null when there is nothing to
    //! draw.
    const Slot* _SlotForView(const View& view);

    //! The shared buffers hold no slot: the next draw rewrites them.
    void _ForgetBufferedSlot()
    {
        _bufferUid = 0;
        _bufferRevision = 0;
    }

    MHWRender::MShaderInstance* _shader = nullptr;
    bool                        _creationFailed = false;
    std::atomic<bool>           _destroyed { false };

    //! The data last published. Never modified after it is published.
    std::shared_ptr<const GaussianSplatViewCore::SplatData> _data;
    uint64_t                                                _dataVersion = 0;

    uint64_t          _sequence = 0; //!< LRU clock, advanced by every beauty draw
    uint64_t          _nextUid = 1;
    std::vector<Slot> _slots;

    //! The slot (and its revision) that the shared buffers hold. 0 uid: none.
    uint64_t _bufferUid = 0;
    uint64_t _bufferRevision = 0;

    //! The color stream of the Rprim (float4, one entry per splat). Rewritten at the same size.
    MHWRender::MVertexBuffer* _colorBuffer = nullptr;

    //! The index buffers of the draw items that share the shader (uint32, one entry per splat).
    //! Each is rewritten at the same size.
    std::vector<MHWRender::MIndexBuffer*> _indexBuffers;
};

} // namespace MAYAUSD_NS_DEF

#endif // HDVP2_GAUSSIAN_SPLAT_VIEW_CACHE_H
