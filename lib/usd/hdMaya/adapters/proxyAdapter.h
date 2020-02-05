#ifndef HDMAYA_AL_PROXY_ADAPTER_H
#define HDMAYA_AL_PROXY_ADAPTER_H

#include "../delegates/proxyUsdImagingDelegate.h"
#include "shapeAdapter.h"

#include <pxr/pxr.h>
#include <pxr/base/tf/weakBase.h>

#include <pxr/usdImaging/usdImaging/delegate.h>

#include "../../../listeners/proxyShapeNotice.h"

PXR_NAMESPACE_OPEN_SCOPE

class MayaUsdProxyShapeBase;

class HdMayaProxyAdapter : public HdMayaShapeAdapter, public TfWeakBase {
public:
    HdMayaProxyAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag);

    ~HdMayaProxyAdapter();

    virtual void MarkDirty(HdDirtyBits dirtyBits) override;

    void Populate() override;

    void PopulateSelectedPaths(
        const MDagPath& selectedDag, SdfPathVector& selectedSdfPaths,
        std::unordered_set<SdfPath, SdfPath::Hash>& selectedMasters,
        const HdSelectionSharedPtr& selection) override;

    bool IsSupported() const override;

    VtValue Get(const TfToken& key) override;

    bool HasType(const TfToken& typeId) const override;

    void CreateUsdImagingDelegate();

    void PreFrame();

    MayaUsdProxyShapeBase* GetProxy() { return _proxy; }

    SdfPath GetPathForInstanceIndex(
        const SdfPath& protoPrimPath, int instanceIndex,
        int* absoluteInstanceIndex, SdfPath* rprimPath = NULL,
        SdfPathVector* instanceContext = NULL) {
        return _usdDelegate->GetPathForInstanceIndex(
            protoPrimPath, instanceIndex, absoluteInstanceIndex, rprimPath,
            instanceContext);
    }

    SdfPath ConvertIndexPathToCachePath(SdfPath const& indexPath) {
        return _usdDelegate->ConvertIndexPathToCachePath(indexPath);
    }

    SdfPath ConvertCachePathToIndexPath(SdfPath const& cachePath) {
        return _usdDelegate->ConvertCachePathToIndexPath(cachePath);
    }

private:
    /// Notice listener method for proxy stage set
    void _OnStageSet(const UsdMayaProxyStageSetNotice& notice);

    MayaUsdProxyShapeBase* _proxy{ nullptr };
    std::unique_ptr<HdMayaProxyUsdImagingDelegate> _usdDelegate;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDMAYA_AL_PROXY_ADAPTER_H
