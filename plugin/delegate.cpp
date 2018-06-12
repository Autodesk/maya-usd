#include "delegate.h"

#include <pxr/base/gf/matrix4d.h>

#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hd/camera.h>

#include <pxr/imaging/hdx/tokens.h>
#include <pxr/imaging/hdx/renderSetupTask.h>
#include <pxr/imaging/hdx/renderTask.h>

PXR_NAMESPACE_USING_DIRECTIVE

HdMayaDelegate::HdMayaDelegate(
    HdRenderIndex* renderIndex,
    const SdfPath& delegateID) :
    HdSceneDelegate(renderIndex, delegateID) {
    // Unique ID
}

HdMayaDelegate::~HdMayaDelegate() {

}

VtValue
HdMayaDelegate::Get(SdfPath const& id, const TfToken& key) {
    auto* cache = TfMapLookupPtr(valueCacheMap, id);
    VtValue ret;
    if (cache && TfMapLookup(*cache, key, &ret)) {
        return ret;
    }

    std::cerr << "[HdMayaDelegate] Error accessing " << key << " from the valueCacheMap!" << std::endl;

    return VtValue();
}
