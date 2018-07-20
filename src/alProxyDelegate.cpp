#include "alProxyDelegate.h"

#include <pxr/base/tf/envSetting.h>
#include <pxr/base/tf/type.h>

#include <maya/MDGMessage.h>
#include <maya/MFnDagNode.h>
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

    // Bunch of conversions + sanity checks...
    auto* delegate = static_cast<HdMayaALProxyDelegate*>(userData);
    if (!TF_VERIFY(delegate,
            "StageLoadedCallback called with null userData ptr")) {
        return;
    }
    auto* proxy = dynamic_cast<ProxyShape*>(node);
    if (!TF_VERIFY(proxy,
            "StageLoadedCallback called with null or non-ProxyShape* ptr")) {
        return;
    }

    // Real work done by delegate->createUsdImagingDelegate
    TF_DEBUG(HDMAYA_AL_CALLBACKS).Msg(
            "HdMayaALProxyDelegate - called StageLoadedCallback (ProxyShape: "
            "%s)\n", proxy->name().asChar());
    delegate->CreateUsdImagingDelegate(proxy);
}

void
ProxyShapeDestroyedCallback(void* userData, AL::event::NodeEvents* node) {

    // Bunch of conversions + sanity checks...
    auto* delegate = static_cast<HdMayaALProxyDelegate*>(userData);
    if (!TF_VERIFY(delegate,
            "ProxyShapeDestroyedCallback called with null userData ptr")) {
        return;
    }
    auto* proxy = dynamic_cast<ProxyShape*>(node);
    if (!TF_VERIFY(proxy,
            "ProxyShapeDestroyedCallback called with null or non-ProxyShape* "
            "ptr")) {
        return;
    }

    // Real work done by delegate->removeProxy
    TF_DEBUG(HDMAYA_AL_CALLBACKS).Msg(
            "HdMayaALProxyDelegate - called ProxyShapeDestroyedCallback "
            "(ProxyShape: %s)\n", proxy->name().asChar());
    delegate->RemoveProxy(proxy);
}

void
ProxyShapeAddedCallback(MObject& node, void* clientData) {

    // Bunch of conversions + sanity checks...
    auto delegate = static_cast<HdMayaALProxyDelegate*>(clientData);
    if (!TF_VERIFY(delegate,
            "ProxyShapeAddedCallback called with null HdMayaALProxyDelegate "
            "ptr")) {
        return;
    }

    MStatus status;
    MFnDependencyNode mfnNode(node, &status);
    if (!TF_VERIFY(status, "Error getting MFnDependencyNode")) {
        return;
    }

    TF_DEBUG(HDMAYA_AL_CALLBACKS).Msg(
            "HdMayaALProxyDelegate - called ProxyShapeAddedCallback "
            "(ProxyShape: %s)\n", mfnNode.name().asChar());


    if(!TF_VERIFY(mfnNode.typeId() == ProxyShape::kTypeId,
            "ProxyShapeAddedCallback called on non-%s node",
            ProxyShape::kTypeName.asChar())) {
        return;
    }

    auto* proxy = dynamic_cast<ProxyShape*>(mfnNode.userNode());
    if (!TF_VERIFY(proxy,
            "Error getting ProxyShape* for %s",
            mfnNode.name().asChar())) {
        return;
    }

    // Real work done by delegate->addProxy
    delegate->AddProxy(proxy);
}

void
ProxyShapeRemovedCallback(MObject& node, void* clientData) {

    // Bunch of conversions + sanity checks...
    auto delegate = static_cast<HdMayaALProxyDelegate*>(clientData);
    if (!TF_VERIFY(delegate,
            "ProxyShapeRemovedCallback called with null HdMayaALProxyDelegate "
            "ptr")) {
        return;
    }

    MStatus status;
    MFnDependencyNode mfnNode(node, &status);
    if (!TF_VERIFY(status, "Error getting MFnDependencyNode")) {
        return;
    }


    TF_DEBUG(HDMAYA_AL_CALLBACKS).Msg(
            "HdMayaALProxyDelegate - called ProxyShapeRemovedCallback "
            "(ProxyShape: %s)\n", mfnNode.name().asChar());

    if(!TF_VERIFY(mfnNode.typeId() == ProxyShape::kTypeId,
            "ProxyShapeRemovedCallback called on non-%s node",
            ProxyShape::kTypeName.asChar())) {
        return;
    }

    auto* proxy = dynamic_cast<ProxyShape*>(mfnNode.userNode());
    if (!TF_VERIFY(proxy,
            "Error getting ProxyShape* for %s",
            mfnNode.name().asChar())) {
        return;
    }

    // Real work done by delegate->deleteUsdImagingDelegate
    delegate->DeleteUsdImagingDelegate(proxy);
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
        if(!TF_VERIFY(proxyShape,
                "ProxyShape had no mpx data: %s",
                fn.name().asChar())) {
            continue;
        }

        auto& proxyData = AddProxy(proxyShape);
        auto stage = proxyShape->getUsdStage();
        if(stage)
        {
            CreateUsdImagingDelegate(proxyShape, proxyData);
        }
    }

    // Set up callback to add any new ProxyShapes
    TF_DEBUG(HDMAYA_AL_CALLBACKS).Msg(
            "HdMayaALProxyDelegate - creating ProxyShapeAddedCallback callback "
            "for all ProxyShapes\n");
    _nodeAddedCBId = MDGMessage::addNodeAddedCallback(ProxyShapeAddedCallback,
            ProxyShape::kTypeName, this, &status);
    if (!TF_VERIFY(status, "Could not set nodeAdded callback")) {
        _nodeAddedCBId = 0;
    }

    // Set up callback to remove ProxyShapes from the index
    TF_DEBUG(HDMAYA_AL_CALLBACKS).Msg(
            "HdMayaALProxyDelegate - creating ProxyShapeRemovedCallback callback "
            "for all ProxyShapes\n");
    _nodeRemovedCBId = MDGMessage::addNodeRemovedCallback(ProxyShapeRemovedCallback,
            ProxyShape::kTypeName, this, &status);
    if (!TF_VERIFY(status, "Could not set nodeRemoved callback")) {
        _nodeRemovedCBId = 0;
    }
}

HdMayaALProxyDelegate::~HdMayaALProxyDelegate() {
    TF_DEBUG(HDMAYA_AL_CALLBACKS).Msg(
            "~HdMayaALProxyDelegate - removing all callbacks\n");
    if (_nodeAddedCBId) {
        MMessage::removeCallback(_nodeAddedCBId);
    }
    if (_nodeRemovedCBId){
        MMessage::removeCallback(_nodeRemovedCBId);
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
        PopulateSingleProxy(proxyAndData.first, proxyAndData.second);
    }
}

bool
HdMayaALProxyDelegate::PopulateSingleProxy(
        ProxyShape* proxy,
        HdMayaALProxyData& proxyData) {
    if (!proxyData.delegate) return false;;

    proxyData.delegate->SetRootTransform(GfMatrix4d(proxy->parentTransform().inclusiveMatrix().matrix));

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
        if (PopulateSingleProxy(proxy, proxyData)) {
            proxyData.delegate->ApplyPendingUpdates();
        }
    }
}

void
HdMayaALProxyDelegate::PopulateSelectedPaths(
        MSelectionList mayaSelection,
        SdfPathVector& selectedSdfPaths) {
    MStatus status;
    MObject proxyMObj;
    MFnDagNode proxyMFnDag;
    MDagPathArray proxyDagPaths;

    for (auto& proxyAndData : _proxiesData) {
        auto& proxy = proxyAndData.first;
        auto& proxyData = proxyAndData.second;

        // First, we check to see if the entire proxy shape is selected
        proxyDagPaths.clear();
        proxyMObj = proxy->thisMObject();
        if (!TF_VERIFY(!proxyMObj.isNull())) { continue; }
        if (!TF_VERIFY(proxyMFnDag.setObject(proxyMObj))) { continue; }
        if (!TF_VERIFY(proxyMFnDag.getAllPaths(proxyDagPaths))) { continue; }
        if (!TF_VERIFY(proxyDagPaths.length())) { continue; }

        bool wholeProxySelected = false;
        // Loop over all instances
        for (unsigned int i = 0, n = proxyDagPaths.length(); i < n; ++i) {
            MDagPath& dagPath = proxyDagPaths[i];
            // Loop over all parents
            while (dagPath.length()) {
                if (mayaSelection.hasItem(dagPath)) {
                    TF_DEBUG(HDMAYA_AL_SELECTION).Msg(
                            "proxy node %s was selected\n",
                            dagPath.fullPathName().asChar());
                    wholeProxySelected = true;
                    selectedSdfPaths.push_back(
                            proxyData.delegate->GetDelegateID());
                    break;
                }
                dagPath.pop();
            }
            if (wholeProxySelected) { break; }
        }
        if (wholeProxySelected) { continue; }

        // Ok, we didn't have the entire proxy selected - instead, add in
        // any "subpaths" of the proxy which may be selected

        const auto& paths1 = proxy->selectedPaths();
        const auto& paths2 = proxy->selectionList().paths();

        if (TfDebug::IsEnabled(HDMAYA_AL_SELECTION)) {
            TfDebug::Helper().Msg(
                    "proxy %s has %lu selectedPaths",
                    proxyDagPaths[0].fullPathName().asChar(),
                    paths1.size());
            if (!paths1.empty()) {
                TfDebug::Helper().Msg(
                        " (1st: %s)",
                        paths1.begin()->GetText());
            }
            TfDebug::Helper().Msg(
                    ", and %lu selectionList paths",
                    paths2.size());
            if (!paths2.empty()) {
                TfDebug::Helper().Msg(
                        " (1st: %s)",
                        paths2.begin()->GetText());
            }
            TfDebug::Helper().Msg("\n");

        }

        for (auto& usdPath : paths1) {
            selectedSdfPaths.push_back(
                    proxyData.delegate->GetPathForIndex(usdPath));
        }

        for (auto& usdPath : paths2) {
            selectedSdfPaths.push_back(
                    proxyData.delegate->GetPathForIndex(usdPath));
        }
    }

}

HdMayaALProxyData&
HdMayaALProxyDelegate::AddProxy(ProxyShape* proxy) {
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
        if (!TF_VERIFY(scheduler,
                "Error getting scheduler for %s",
                proxy->name().asChar())) {
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
    CreateUsdImagingDelegate(proxy, proxyData);
    return proxyData;
}

void
HdMayaALProxyDelegate::RemoveProxy(ProxyShape* proxy) {
    // Note that we don't bother unregistering the proxyShapeCallbacks,
    // as this is called when the ProxyShape is about to be destroyed anyway
    _proxiesData.erase(proxy);
}

void
HdMayaALProxyDelegate::CreateUsdImagingDelegate(ProxyShape* proxy) {
    auto proxyDataIter = _proxiesData.find(proxy);
    if (!TF_VERIFY(proxyDataIter != _proxiesData.end(),
            "Proxy not found in delegate: %s",
            proxy->name().asChar())) {
        return;
    }
    CreateUsdImagingDelegate(proxy, proxyDataIter->second);
}

void
HdMayaALProxyDelegate::CreateUsdImagingDelegate(
        ProxyShape* proxy,
        HdMayaALProxyData& proxyData) {
    proxyData.delegate.reset(new UsdImagingDelegate(
            _renderIndex,
            _delegateID.AppendChild(TfToken(
                    TfStringPrintf("ALProxyDelegate_%s_%p",
                            proxy->name().asChar(), proxy)))));
    proxyData.populated = false;
}

void
HdMayaALProxyDelegate::DeleteUsdImagingDelegate(ProxyShape* proxy) {
    auto proxyDataIter = _proxiesData.find(proxy);
    if (!TF_VERIFY(proxyDataIter != _proxiesData.end(),
            "Proxy not found in delegate: %s",
            proxy->name().asChar())) {
        return;
    }
    auto& proxyData = proxyDataIter->second;
    proxyData.delegate.reset(nullptr);
    proxyData.populated = false;
}


PXR_NAMESPACE_CLOSE_SCOPE
