#include <alProxyDelegate.h>
#include <pxr/base/tf/envSetting.h>

#include <hdmaya/delegates/delegateRegistry.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HDMAYA_AL_TEST_DELEGATE_FILE, "",
                      "Path for HdMayaProxyDelegate to load");

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (HdMayaALProxyDelegate)
);

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaDelegateRegistry, HdMayaALProxyDelegate) {
    if (!TfGetEnvSetting(HDMAYA_AL_TEST_DELEGATE_FILE).empty()) {
        HdMayaDelegateRegistry::RegisterDelegate(
            _tokens->HdMayaALProxyDelegate,
            [](HdRenderIndex* parentIndex, const SdfPath& id) -> HdMayaDelegatePtr {
                return std::static_pointer_cast<HdMayaALProxyDelegate>(std::make_shared<HdMayaALProxyDelegate>(parentIndex, id));
            });
    }
}

HdMayaALProxyDelegate::HdMayaALProxyDelegate(HdRenderIndex* renderIndex,
                                       const SdfPath& delegateID) {
    _delegate.reset(new UsdImagingDelegate(renderIndex, delegateID));
}

void
HdMayaALProxyDelegate::Populate() {
    _stage = UsdStage::Open(TfGetEnvSetting(HDMAYA_AL_TEST_DELEGATE_FILE));
    _delegate->Populate(_stage->GetPseudoRoot());
}

PXR_NAMESPACE_CLOSE_SCOPE
