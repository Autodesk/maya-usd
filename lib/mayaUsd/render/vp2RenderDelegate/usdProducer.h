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
#ifndef HD_VP2_USD_PRODUCER
#define HD_VP2_USD_PRODUCER

#include <pxr/base/gf/matrix4d.h>
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/imaging/hd/selection.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HdRenderIndex;
class UsdImagingDelegate;

/*! \brief  Abstraction over the source of USD data consumed by VP2.

    ProxyRenderDelegate owns the VP2 render delegate, render index, task
    controller and Hydra engine. This interface covers only the remaining piece:
    where the USD scene data comes from.

    Two implementations exist. HdVP2UsdImagingDelegateProducer uses the Hydra 1.0
    UsdImagingDelegate and is the default. HdVP2SceneIndexProducer uses a Hydra
    2.0 scene index chain and is selected by building with
    CMAKE_WANT_MAYAUSD_VP2_USE_SCENE_INDEX=ON. The choice is made in Create(),
    which is the only place the MAYAUSD_VP2_USE_SCENE_INDEX definition is
    consulted.

    \class  HdVP2UsdProducer
*/
class HdVP2UsdProducer
{
public:
    //! \brief  Creates the producer selected at build time. Never returns null.
    static std::unique_ptr<HdVP2UsdProducer> Create();

    virtual ~HdVP2UsdProducer() = default;

    //! \brief  Attaches the producer to a render index.
    //! \param  producerId Path prefix under which the producer's prims appear
    //!         in the render index.
    virtual void Initialize(HdRenderIndex* renderIndex, const SdfPath& producerId) = 0;

    /*! \brief  Populates the render index from a stage, scoped to a subtree.

        \param  stage The stage to populate from. Must not be null.
        \param  rootPath The proxy shape's primPath: "/" for the whole stage.
                Prims keep their stage paths under producerId. Only prims at or
                below rootPath are drawn, plus the materials they bind and the
                prototypes they instance. The prim at rootPath and its ancestors
                contribute an identity transform, so the caller supplies their
                world transform through SetRootTransform. Their visibility,
                purpose and material bindings still apply. rootPath need not
                name an existing prim: if it does not, nothing is drawn, and
                that is still a successful populate, so the call is not retried
                every frame.
        \param  excludedPaths Prims to omit, each together with its subtree.
        \return False only if the producer could not populate at all, such as
                with a null stage. Callers must not latch a populated flag in
                that case, or a single transient failure leaves the proxy shape
                permanently empty.
    */
    virtual bool Populate(
        const UsdStageRefPtr& stage,
        const SdfPath&        rootPath,
        const SdfPathVector&  excludedPaths)
        = 0;

    //! \brief  Flushes any scene edits queued since the last update.
    virtual void ApplyPendingUpdates() = 0;

    /*! \brief  Emits dirty marks withheld because they were raised during
                HdRenderIndex::SyncAll.

        Must be called outside SyncAll. The marks reach the change tracker, but
        nothing syncs them until the next HdEngine::Execute; the caller decides
        whether to run one.

        \return True if anything was emitted.
    */
    virtual bool FlushDeferredUpdates() = 0;

    virtual void        SetTime(const UsdTimeCode& timeCode) = 0;
    virtual UsdTimeCode GetTime() const = 0;

    virtual void       SetRootTransform(const GfMatrix4d& transform) = 0;
    virtual GfMatrix4d GetRootTransform() const = 0;

    virtual void SetRootVisibility(bool visible) = 0;
    virtual bool GetRootVisibility() const = 0;

    virtual void SetRefineLevelFallback(int level) = 0;
    virtual int  GetRefineLevelFallback() const = 0;

    //! \brief  Maps a USD scene path to its path in the render index.
    virtual SdfPath ToIndexPath(const SdfPath& usdPath) const = 0;

    //! \brief  Resolves an rprim and instance index back to the originating USD prim.
    virtual SdfPath GetScenePrimPath(
        const SdfPath&      rprimId,
        int                 instanceIndex,
        HdInstancerContext* instancerContext) const = 0;

    //! \brief  Vectorized GetScenePrimPath.
    virtual SdfPathVector
    GetScenePrimPaths(const SdfPath& rprimId, const std::vector<int>& instanceIndexes) const = 0;

    //! \brief  Adds the rprims corresponding to a USD path into a Hydra selection.
    virtual void PopulateSelection(
        const SdfPath&              usdPath,
        int                         instanceIndex,
        const HdSelectionSharedPtr& result) = 0;

    /*! \brief  Invalidates a prim this producer contributed.

        VP2 dirties prims imperatively for selection highlight, display mode,
        display layers, render tags and material changes. Which mechanism
        actually delivers the invalidation depends on how the producer feeds the
        render index, so it cannot be done by calling HdChangeTracker directly -
        see HdVP2DirtyingSceneIndex for why that silently does nothing for a
        scene index producer.

        \param  indexPath Path of the prim in the render index, not the USD path.
        \param  bits Dirty bits, including MayaUsdRPrim's custom bits.
    */
    virtual void MarkRprimDirty(const SdfPath& indexPath, HdDirtyBits bits) = 0;

    //! \brief  Sprim equivalent of MarkRprimDirty.
    virtual void MarkSprimDirty(const SdfPath& indexPath, HdDirtyBits bits) = 0;

    /*! \brief  The scene delegate feeding this producer's prims.

        Needed for HdSceneDelegate calls that are not routed through Sync, such
        as GetInstancerId and HdRprim::InitRepr.

        Each producer contributes exactly one scene delegate: the
        UsdImagingDelegate itself for the Hydra 1.0 producer, and the single
        emulation adapter delegate for a scene index chain. So no prim id is
        needed to disambiguate.
    */
    virtual HdSceneDelegate* GetSceneDelegate() const = 0;

    /*! \brief  The underlying UsdImagingDelegate, or nullptr if there is none.

        Only for the deprecated ProxyRenderDelegate::GetUsdImagingDelegate
        accessor and for code paths that are specific to the Hydra 1.0 producer.
        Prefer GetSceneDelegate.
    */
    virtual UsdImagingDelegate* GetUsdImagingDelegate() const = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
