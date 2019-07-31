#ifndef __HDMAYA_AL_PROXY_ADAPTER_H__
#define __HDMAYA_AL_PROXY_ADAPTER_H__

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
#ifdef HDMAYA_USD_001907_BUILD
        return _usdDelegate->ConvertIndexPathToCachePath(indexPath);
#else
        return _usdDelegate->GetPathForUsd(indexPath);
#endif
    }

    SdfPath ConvertCachePathToIndexPath(SdfPath const& cachePath) {
#ifdef HDMAYA_USD_001907_BUILD
        return _usdDelegate->ConvertCachePathToIndexPath(cachePath);
#else
        return _usdDelegate->GetPathForIndex(cachePath);
#endif
    }

private:
    /// Notice listener method for proxy stage set
    void _OnStageSet(const UsdMayaProxyStageSetNotice& notice);

    MayaUsdProxyShapeBase* _proxy{ nullptr };
    std::unique_ptr<HdMayaProxyUsdImagingDelegate> _usdDelegate;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_AL_PROXY_ADAPTER_H__
