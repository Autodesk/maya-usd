#include <alProxyDelegate.h>

#include <pxr/base/tf/envSetting.h>
#include <pxr/base/tf/type.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MObject.h>

#include "AL/usdmaya/nodes/ProxyShape.h"

#include "hdmaya/delegates/delegateRegistry.h"

#include "debugCodes.h"

using AL::usdmaya::nodes::ProxyShape;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (HdMayaALProxyDelegate)
);

TF_REGISTRY_FUNCTION(TfType)
{
	TF_DEBUG(HDMAYA_AL_PLUGIN).Msg(
			"Calling TfType::Define for HdMayaALProxyDelegate\n");
    TfType::Define<HdMayaALProxyDelegate, TfType::Bases<HdMayaDelegate> >();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaDelegateRegistry, HdMayaALProxyDelegate) {
	TF_DEBUG(HDMAYA_AL_PLUGIN).Msg(
			"Calling RegisterDelegate for HdMayaALProxyDelegate\n");
    HdMayaDelegateRegistry::RegisterDelegate(
        _tokens->HdMayaALProxyDelegate,
        [](HdRenderIndex* parentIndex, const SdfPath& id) -> HdMayaDelegatePtr {
            return std::static_pointer_cast<HdMayaALProxyDelegate>(std::make_shared<HdMayaALProxyDelegate>(parentIndex, id));
        });
}

HdMayaALProxyDelegate::HdMayaALProxyDelegate(HdRenderIndex* renderIndex,
                                       const SdfPath& delegateID) {
    _delegate.reset(new UsdImagingDelegate(renderIndex, delegateID));
}

void
HdMayaALProxyDelegate::Populate() {
	TF_DEBUG(HDMAYA_AL_POPULATE).Msg(
			"HdMayaALProxyDelegate::Populate\n");
    if (!_stage) {
    	TF_DEBUG(HDMAYA_AL_POPULATE).Msg(
    			"HdMayaALProxyDelegate::Populate - stage not set\n");
        MFnDependencyNode fn;

        MItDependencyNodes iter(MFn::kPluginShape);
        for(; !iter.isDone(); iter.next()) {
            MObject mobj = iter.item();
            fn.setObject(mobj);
            if(fn.typeId() != ProxyShape::kTypeId) continue;

            auto proxyShape = static_cast<ProxyShape *>(fn.userNode());
            if(proxyShape == nullptr) {
                MGlobal::displayError(MString("ProxyShape had no mpx data: ") + fn.name());
                continue;
            }

            _stage = proxyShape->getUsdStage();

            if(!_stage)
            {
              MGlobal::displayError(MString("Could not get stage for proxyShape: ") + fn.name());
              continue;
            }

        	TF_DEBUG(HDMAYA_AL_POPULATE).Msg(
        			"HdMayaALProxyDelegate::Populate - setting stage from '%s'\n",
					fn.name().asChar());

            break;
        }
    }
    if (_stage) {
    	_delegate->Populate(_stage->GetPseudoRoot());
    }
}

void
HdMayaALProxyDelegate::PreFrame() {
	_delegate->ApplyPendingUpdates();
}

PXR_NAMESPACE_CLOSE_SCOPE
