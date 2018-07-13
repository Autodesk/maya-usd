#include "alProxyDelegate.h"

#include <pxr/base/tf/envSetting.h>
#include <pxr/base/tf/type.h>

#include <maya/MDGMessage.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MObject.h>

#include <hdmaya/delegates/delegateRegistry.h>

#include "debugCodes.h"

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

namespace {

void
StageLoadedCallback(void* userData, AL::event::NodeEvents* node) {
	auto* delegate = static_cast<HdMayaALProxyDelegate*>(userData);
	// TODO: add in TF_UNLIKELY for sanity checks
	if (!delegate) {
		TF_CODING_ERROR("StageLoadedCallback called with null "
				"userData ptr");
		return;
	}
	auto* proxy = dynamic_cast<ProxyShape*>(node);
	if (!proxy) {
		if (!userData) {
			TF_CODING_ERROR("StageLoadedCallback called with null "
					"node ptr");
		}
		else {
			TF_CODING_ERROR("StageLoadedCallback called with node that "
					"not a ProxyShape*");
		}
		return;
	}
	TF_DEBUG(HDMAYA_AL_CALLBACKS).Msg(
			"HdMayaALProxyDelegate - called StageLoadedCallback (ProxyShape: "
			"%s)\n", proxy->name().asChar());
	delegate->createUsdImagingDelegate(proxy);
}

void
ProxyShapeDestroyedCallback(void* userData, AL::event::NodeEvents* node) {
	auto* delegate = static_cast<HdMayaALProxyDelegate*>(userData);
	// TODO: add in TF_UNLIKELY for sanity checks
	if (!delegate) {
		TF_CODING_ERROR("ProxyShapeDestroyedCallback called with null "
				"userData ptr");
		return;
	}
	auto* proxy = dynamic_cast<ProxyShape*>(node);
	if (!proxy) {
		if (!userData) {
			TF_CODING_ERROR("ProxyShapeDestroyedCallback called with null "
					"node ptr");
		}
		else {
			TF_CODING_ERROR("ProxyShapeDestroyedCallback called with node that "
					"not a ProxyShape*");
		}
		return;
	}

	TF_DEBUG(HDMAYA_AL_CALLBACKS).Msg(
			"HdMayaALProxyDelegate - called ProxyShapeDestroyedCallback "
			"(ProxyShape: %s)\n", proxy->name().asChar());
	delegate->removeProxy(proxy);
}

void
ProxyShapeAddedCallback(MObject& node, void* clientData) {
	auto delegate = static_cast<HdMayaALProxyDelegate*>(clientData);
	if (!delegate) {
		TF_CODING_ERROR("ProxyShapeAddedCallback called with null "
				"HdMayaALProxyDelegate ptr");
		return;
	}

	MStatus status;
	MFnDependencyNode mfnNode(node, &status);
	if (!status) {
		TF_CODING_ERROR("Error getting MFnDependencyNode");
		return;
	}

	TF_DEBUG(HDMAYA_AL_CALLBACKS).Msg(
			"HdMayaALProxyDelegate - called ProxyShapeAddedCallback "
			"(ProxyShape: %s)\n", mfnNode.name().asChar());


	if(mfnNode.typeId() != ProxyShape::kTypeId) {
		TF_CODING_ERROR("ProxyShapeAddedCallback called on non-%s node",
				ProxyShape::kTypeName.asChar());
		return;
	}

	auto* proxy = dynamic_cast<ProxyShape*>(mfnNode.userNode());
	if (!proxy) {
		TF_CODING_ERROR("Error getting ProxyShape* for %s",
						mfnNode.name().asChar());
		return;
	}

	delegate->addProxy(proxy);
}

void
ProxyShapeRemovedCallback(MObject& node, void* clientData) {
	auto delegate = static_cast<HdMayaALProxyDelegate*>(clientData);
	if (!delegate) {
		TF_CODING_ERROR("ProxyShapeRemovedCallback called with null "
				"HdMayaALProxyDelegate ptr");
		return;
	}

	MStatus status;
	MFnDependencyNode mfnNode(node, &status);
	if (!status) {
		TF_CODING_ERROR("Error getting MFnDependencyNode");
		return;
	}

	TF_DEBUG(HDMAYA_AL_CALLBACKS).Msg(
			"HdMayaALProxyDelegate - called ProxyShapeRemovedCallback "
			"(ProxyShape: %s)\n", mfnNode.name().asChar());


	if(mfnNode.typeId() != ProxyShape::kTypeId) {
		TF_CODING_ERROR("ProxyShapeRemovedCallback called on non-%s node",
				ProxyShape::kTypeName.asChar());
		return;
	}

	auto* proxy = dynamic_cast<ProxyShape*>(mfnNode.userNode());
	if (!proxy) {
		TF_CODING_ERROR("Error getting ProxyShape* for %s",
						mfnNode.name().asChar());
		return;
	}

	delegate->deleteUsdImagingDelegate(proxy);
}

} // private namespace

HdMayaALProxyDelegate::HdMayaALProxyDelegate(HdRenderIndex* renderIndex,
		const SdfPath& delegateID) :
				_delegateID(delegateID),
				_renderIndex(renderIndex) {
    MStatus status;

	// Add all pre-existing proxy shapes
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

		auto& proxyData = addProxy(proxyShape);
		auto stage = proxyShape->getUsdStage();
		if(stage)
		{
			_createUsdImagingDelegate(proxyShape, proxyData);
		}
	}

    // Set up callback to add any new ProxyShapes
	TF_DEBUG(HDMAYA_AL_CALLBACKS).Msg(
			"HdMayaALProxyDelegate - creating ProxyShapeAddedCallback callback "
			"for all ProxyShapes\n");
    _nodeAddedCBId = MDGMessage::addNodeAddedCallback(ProxyShapeAddedCallback,
    		ProxyShape::kTypeName, this, &status);
    if (!status) {
    	TF_CODING_ERROR("Could not set nodeAdded callback");
    	_nodeAddedCBId = 0;
    }

    // Set up callback to remove ProxyShapes from the index
	TF_DEBUG(HDMAYA_AL_CALLBACKS).Msg(
			"HdMayaALProxyDelegate - creating ProxyShapeRemovedCallback callback "
			"for all ProxyShapes\n");
	_nodeRemovedCBId = MDGMessage::addNodeRemovedCallback(ProxyShapeRemovedCallback,
    		ProxyShape::kTypeName, this, &status);
    if (!status) {
    	TF_CODING_ERROR("Could not set nodeRemoved callback");
    	_nodeRemovedCBId = 0;
    }
}

HdMayaALProxyDelegate::~HdMayaALProxyDelegate() {
	if (_nodeAddedCBId) {
		MMessage::removeCallback(_nodeAddedCBId);
	}

	// If the delegate is destroyed before the proxy shapes, clean up their
	// callbacks
	for (auto& proxyAndData : _proxiesData) {
		auto& proxy = proxyAndData.first;
		auto& proxyData = proxyAndData.second;
		for (auto& callbackId : proxyData.proxyShapeCallbacks) {
			proxy->scheduler()->unregisterCallback(callbackId);
		}
	}
}

void
HdMayaALProxyDelegate::Populate() {
	TF_DEBUG(HDMAYA_AL_POPULATE).Msg(
			"HdMayaALProxyDelegate::Populate\n");
	for (auto& proxyAndData : _proxiesData) {
		_populateSingleProxy(proxyAndData.first, proxyAndData.second);
    }
}

bool
HdMayaALProxyDelegate::_populateSingleProxy(
		ProxyShape* proxy,
		HdMayaALProxyData& proxyData) {
	if (!proxyData.delegate) return false;;

	if (!proxyData.populated) {
		TF_DEBUG(HDMAYA_AL_POPULATE).Msg(
				"HdMayaALProxyDelegate::Populating %s\n",
				proxy->name().asChar());

		auto stage = proxy->getUsdStage();
		if(!stage)
		{
		  MGlobal::displayError(MString(
				  "Could not get stage for proxyShape: ") + proxy->name());
		  return false;
		}
		proxyData.delegate->Populate(stage->GetPseudoRoot());
		proxyData.populated = true;
	}
	return true;
}

void
HdMayaALProxyDelegate::PreFrame() {
	for (auto& proxyAndData : _proxiesData) {
		auto& proxy = proxyAndData.first;
		auto& proxyData = proxyAndData.second;
		if (_populateSingleProxy(proxy, proxyData)) {
			proxyData.delegate->ApplyPendingUpdates();
		}
	}
}

HdMayaALProxyData&
HdMayaALProxyDelegate::addProxy(ProxyShape* proxy) {
	// Our ProxyShapeAdded callback is trigged every time the node is added
	// to the DG, NOT when the C++ ProxyShape object is created; due to the
	// undo queue, it's possible for the same ProxyShape to be added (and
	// removed) from the DG several times throughout it's lifetime.  However,
	// we only call removeProxy() when the C++ ProxyShape object is actually
	// destroyed - so it's possible that the given proxy has already been
	// added to this delegate!
	auto emplaceResult = _proxiesData.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(proxy),
			std::forward_as_tuple());
	auto& proxyData = emplaceResult.first->second;
	if (emplaceResult.second) {
		// Okay, we have an actual insertion! Set up callbacks...
		auto* scheduler = proxy->scheduler();
		if (!scheduler) {
			TF_CODING_ERROR("Error getting scheduler for %s",
							proxy->name().asChar());
			return proxyData;
		}

		TF_DEBUG(HDMAYA_AL_CALLBACKS).Msg(
				"HdMayaALProxyDelegate - creating PreStageLoaded callback "
				"for %s\n", proxy->name().asChar());
		proxyData.proxyShapeCallbacks.push_back(
				scheduler->registerCallback(
						proxy->getId("PreStageLoaded"), // eventId
						"HdMayaALProxyDelegate_onStageLoad", // tag
						StageLoadedCallback, // functionPointer
						10000, // weight
						this // userData
						));

		TF_DEBUG(HDMAYA_AL_CALLBACKS).Msg(
				"HdMayaALProxyDelegate - creating PreDestroyProxyShape "
				"callback for %s\n", proxy->name().asChar());
		proxyData.proxyShapeCallbacks.push_back(
				scheduler->registerCallback(
						proxy->getId("PreDestroyProxyShape"), // eventId
						"HdMayaALProxyDelegate_onProxyDestroy", // tag
						ProxyShapeDestroyedCallback, // functionPointer
						10000, // weight
						this // userData
						));
	}
	_createUsdImagingDelegate(proxy, proxyData);
	return proxyData;
}

void
HdMayaALProxyDelegate::removeProxy(ProxyShape* proxy) {
	// Note that we don't bother unregistering the proxyShapeCallbacks,
	// as this is called when the ProxyShape is about to be destroyed anyway
	_proxiesData.erase(proxy);
}

void
HdMayaALProxyDelegate::createUsdImagingDelegate(ProxyShape* proxy) {
	auto proxyDataIter = _proxiesData.find(proxy);
	if (proxyDataIter == _proxiesData.end()) {
		TF_CODING_ERROR("Proxy not found in delegate: %s",
				proxy->name().asChar());
		return;
	}
	_createUsdImagingDelegate(proxy, proxyDataIter->second);
}

void
HdMayaALProxyDelegate::_createUsdImagingDelegate(
		ProxyShape* proxy,
		HdMayaALProxyData& proxyData) {
	// TODO: check what happens on deletion of old delegate - is renderIndex
	// automatically cleaned up?
	proxyData.delegate.reset(new UsdImagingDelegate(
			_renderIndex,
			_delegateID.AppendChild(TfToken(
					TfStringPrintf("ALProxyDelegate_%s_%p",
							proxy->name().asChar(), proxy)))));
	proxyData.populated = false;
}

void
HdMayaALProxyDelegate::deleteUsdImagingDelegate(ProxyShape* proxy) {
	auto proxyDataIter = _proxiesData.find(proxy);
	if (proxyDataIter == _proxiesData.end()) {
		TF_CODING_ERROR("Proxy not found in delegate: %s",
				proxy->name().asChar());
		return;
	}
	auto& proxyData = proxyDataIter->second;
	proxyData.delegate.reset(nullptr);
	proxyData.populated = false;
}


PXR_NAMESPACE_CLOSE_SCOPE
