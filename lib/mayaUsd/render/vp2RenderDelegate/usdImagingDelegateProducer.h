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
#ifndef HD_VP2_USD_IMAGING_DELEGATE_PRODUCER
#define HD_VP2_USD_IMAGING_DELEGATE_PRODUCER

#include "usdProducer.h"

#include <pxr/pxr.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

/*! \brief  VP2 USD producer backed by the Hydra 1.0 UsdImagingDelegate.

    This is the default producer and preserves the behaviour VP2 has always had.

    \class  HdVP2UsdImagingDelegateProducer
*/
class HdVP2UsdImagingDelegateProducer final : public HdVP2UsdProducer
{
public:
    HdVP2UsdImagingDelegateProducer() = default;
    ~HdVP2UsdImagingDelegateProducer() override = default;

    void Initialize(HdRenderIndex* renderIndex, const SdfPath& producerId) override;
    bool Populate(
        const UsdStageRefPtr& stage,
        const SdfPath&        rootPath,
        const SdfPathVector&  excludedPaths) override;

    void ApplyPendingUpdates() override;
    bool FlushDeferredUpdates() override;

    void        SetTime(const UsdTimeCode& timeCode) override;
    UsdTimeCode GetTime() const override;

    void       SetRootTransform(const GfMatrix4d& transform) override;
    GfMatrix4d GetRootTransform() const override;

    void SetRootVisibility(bool visible) override;
    bool GetRootVisibility() const override;

    void SetRefineLevelFallback(int level) override;
    int  GetRefineLevelFallback() const override;

    SdfPath ToIndexPath(const SdfPath& usdPath) const override;

    SdfPath GetScenePrimPath(
        const SdfPath&      rprimId,
        int                 instanceIndex,
        HdInstancerContext* instancerContext) const override;

    SdfPathVector GetScenePrimPaths(
        const SdfPath&          rprimId,
        const std::vector<int>& instanceIndexes) const override;

    void PopulateSelection(
        const SdfPath&              usdPath,
        int                         instanceIndex,
        const HdSelectionSharedPtr& result) override;

    void ClearSelection() override;

    void MarkRprimDirty(const SdfPath& indexPath, HdDirtyBits bits) override;
    void MarkSprimDirty(const SdfPath& indexPath, HdDirtyBits bits) override;

    HdSceneDelegate*    GetSceneDelegate() const override;
    UsdImagingDelegate* GetUsdImagingDelegate() const override;

private:
    HdRenderIndex*                      _renderIndex { nullptr };
    std::unique_ptr<UsdImagingDelegate> _delegate;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
