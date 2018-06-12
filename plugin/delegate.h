#ifndef __HDMAYA_DELEGATE_H__
#define __HDMAYA_DELEGATE_H__

#include <pxr/pxr.h>

#include <pxr/base/gf/vec4d.h>

#include <pxr/imaging/hd/sceneDelegate.h>

#include <maya/MMatrix.h>

#include "params.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaDelegate : public HdSceneDelegate {
public:
    HdMayaDelegate(
        HdRenderIndex* renderIndex,
        const SdfPath& delegateID);

    virtual ~HdMayaDelegate();

    VtValue Get(SdfPath const& id, TfToken const& key) override;
private:
    using ValueCache = TfHashMap<TfToken, VtValue, TfToken::HashFunctor>;
    using ValueCacheMap = TfHashMap<SdfPath, ValueCache, SdfPath::Hash>;
    ValueCacheMap valueCacheMap;

    template <typename T>
    const T& GetValue(const SdfPath& id, const TfToken& key) {
        auto v = valueCacheMap[id][key];
        TF_VERIFY(v.IsHolding<T>());
        return v.Get<T>();
    }

    template <typename T>
    void SetValue(const SdfPath& id, const TfToken& key, const T& value) {
        valueCacheMap[id][key] = value;
    }
};

typedef std::shared_ptr<HdMayaDelegate> MayaSceneDelegateSharedPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_DELEGATE_H__
