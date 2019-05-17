#include <hdmaya/adapters/shapeAdapter.h>

#include <AL/usdmaya/nodes/ProxyShape.h>

#include <pxr/pxr.h>

#include <pxr/usdImaging/usdImaging/delegate.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaALProxyAdapter : public HdMayaShapeAdapter {
public:
    HdMayaALProxyAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag);

    ~HdMayaALProxyAdapter();

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

    AL::usdmaya::nodes::ProxyShape* GetProxy() { return _proxy; }

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
    std::vector<AL::event::CallbackId> _proxyShapeCallbacks;
    AL::usdmaya::nodes::ProxyShape* _proxy = nullptr;
    std::unique_ptr<UsdImagingDelegate> _usdDelegate;
};

PXR_NAMESPACE_CLOSE_SCOPE
