#include <hdmaya/delegates/testDelegate.h>

#include <pxr/base/tf/envSetting.h>

#include <hdmaya/delegates/delegateRegistry.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HDMAYA_TEST_DELEGATE_FILE, "",
                      "Path for HdMayaTestDelegate to load");

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (HdMayaTestDelegate)
);

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaDelegateRegistry, HdMayaTestDelegate) {
    if (!TfGetEnvSetting(HDMAYA_TEST_DELEGATE_FILE).empty()) {
        HdMayaDelegateRegistry::RegisterDelegate(
            _tokens->HdMayaTestDelegate,
            [](HdRenderIndex* parentIndex, const SdfPath& id) -> HdMayaDelegatePtr {
                return std::static_pointer_cast<HdMayaDelegate>(std::make_shared<HdMayaTestDelegate>(parentIndex, id));
            });
    }
}

HdMayaTestDelegate::HdMayaTestDelegate(HdRenderIndex* renderIndex,
                                       const SdfPath& delegateID) {
    _delegate.reset(new UsdImagingDelegate(renderIndex, delegateID));
}

void
HdMayaTestDelegate::Populate() {
    _stage = UsdStage::Open(TfGetEnvSetting(HDMAYA_TEST_DELEGATE_FILE));
    _delegate->Populate(_stage->GetPseudoRoot());
}

PXR_NAMESPACE_CLOSE_SCOPE
