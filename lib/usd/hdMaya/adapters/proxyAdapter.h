#ifndef HDMAYA_AL_PROXY_ADAPTER_H
#define HDMAYA_AL_PROXY_ADAPTER_H

#include <hdMaya/adapters/shapeAdapter.h>
#include <hdMaya/delegates/proxyUsdImagingDelegate.h>
#include <mayaUsd/listeners/proxyShapeNotice.h>

#include <pxr/base/tf/weakBase.h>
#include <pxr/pxr.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

PXR_NAMESPACE_OPEN_SCOPE

class MayaUsdProxyShapeBase;

class HdMayaProxyAdapter
    : public HdMayaShapeAdapter
    , public TfWeakBase
{
public:
    HdMayaProxyAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag);

    ~HdMayaProxyAdapter();

    virtual void MarkDirty(HdDirtyBits dirtyBits) override;

    void Populate() override;

    void PopulateSelectedPaths(
        const MDagPath&                             selectedDag,
        SdfPathVector&                              selectedSdfPaths,
        std::unordered_set<SdfPath, SdfPath::Hash>& selectedMasters,
        const HdSelectionSharedPtr&                 selection) override;

    bool IsSupported() const override;

    VtValue Get(const TfToken& key) override;

    bool HasType(const TfToken& typeId) const override;

    void CreateUsdImagingDelegate();

    void PreFrame(const MHWRender::MDrawContext& context);

    MayaUsdProxyShapeBase* GetProxy() { return _proxy; }

#if defined(USD_IMAGING_API_VERSION) && USD_IMAGING_API_VERSION >= 14
    SdfPath GetScenePrimPath(
        const SdfPath&      rprimId,
        int                 instanceIndex,
        HdInstancerContext* instancerContext)
    {
        return _usdDelegate->GetScenePrimPath(rprimId, instanceIndex, instancerContext);
    }
#elif defined(USD_IMAGING_API_VERSION) && USD_IMAGING_API_VERSION >= 13
    SdfPath GetScenePrimPath(const SdfPath& rprimId, int instanceIndex)
    {
        return _usdDelegate->GetScenePrimPath(rprimId, instanceIndex);
    }
#else
    SdfPath GetPathForInstanceIndex(
        const SdfPath& protoPrimPath,
        int            instanceIndex,
        int*           absoluteInstanceIndex,
        SdfPath*       rprimPath = NULL,
        SdfPathVector* instanceContext = NULL)
    {
        return _usdDelegate->GetPathForInstanceIndex(
            protoPrimPath, instanceIndex, absoluteInstanceIndex, rprimPath, instanceContext);
    }
#endif

    SdfPath ConvertIndexPathToCachePath(SdfPath const& indexPath)
    {
        return _usdDelegate->ConvertIndexPathToCachePath(indexPath);
    }

    SdfPath ConvertCachePathToIndexPath(SdfPath const& cachePath)
    {
        return _usdDelegate->ConvertCachePathToIndexPath(cachePath);
    }

    bool PopulateSelection(
        HdSelection::HighlightMode const& highlightMode,
        const SdfPath&                    usdPath,
        int                               instanceIndex,
        HdSelectionSharedPtr const&       result)
    {
        return _usdDelegate->PopulateSelection(highlightMode, usdPath, instanceIndex, result);
    }

    const SdfPath& GetUsdDelegateID() const { return _usdDelegate->GetDelegateID(); }

private:
    /// Notice listener method for proxy stage set
    void _OnStageSet(const MayaUsdProxyStageSetNotice& notice);

    MayaUsdProxyShapeBase*                         _proxy { nullptr };
    std::unique_ptr<HdMayaProxyUsdImagingDelegate> _usdDelegate;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDMAYA_AL_PROXY_ADAPTER_H
