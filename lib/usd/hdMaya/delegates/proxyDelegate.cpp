//
// Copyright 2019 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "proxyDelegate.h"

#include "../adapters/proxyAdapter.h"
#include "../debugCodes.h"

#include "delegateRegistry.h"

#include "../../../nodes/proxyShapeBase.h"

#include <pxr/base/tf/envSetting.h>
#include <pxr/base/tf/type.h>

#include <maya/MDGMessage.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MNodeClass.h>
#include <maya/MObject.h>
#include <maya/MSceneMessage.h>

#include <atomic>
#include <mutex>
#include <unordered_set>

#if WANT_UFE_BUILD
#include <ufe/rtid.h>
#include <ufe/runTimeMgr.h>
#endif // WANT_UFE_BUILD

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(_tokens, (HdMayaProxyDelegate));

TF_REGISTRY_FUNCTION(TfType) {
    TF_DEBUG(HDMAYA_AL_PLUGIN)
        .Msg("Calling TfType::Define for HdMayaProxyDelegate\n");
    TfType::Define<HdMayaProxyDelegate, TfType::Bases<HdMayaDelegate> >();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaDelegateRegistry, HdMayaProxyDelegate) {
    TF_DEBUG(HDMAYA_AL_PLUGIN)
        .Msg("Calling RegisterDelegate for HdMayaProxyDelegate\n");
    HdMayaDelegateRegistry::RegisterDelegate(
        _tokens->HdMayaProxyDelegate, HdMayaProxyDelegate::Creator);
}

namespace {

#if WANT_UFE_BUILD
constexpr auto USD_UFE_RUNTIME_NAME = "USD";
static UFE_NS::Rtid usdUfeRtid = 0;
#endif // WANT_UFE_BUILD

// Don't know if this variable would be accessed from multiple threads, but
// plugin load/unload is infrequent enough that performance isn't an issue, and
// I prefer to default to thread-safety for global variables.
std::atomic<bool> isALPluginLoaded(false);

// Not sure if we actually need a mutex guarding _allAdapters, but
// I'd rather be safe....
std::mutex _allAdaptersMutex;
std::unordered_set<HdMayaProxyAdapter*> _allAdapters;

bool IsALPluginLoaded() {
    auto nodeClass = MNodeClass(MayaUsdProxyShapeBase::typeId);
    // if not loaded yet, typeName() will be an empty string
    return nodeClass.typeName() == MayaUsdProxyShapeBase::typeName;
}

void PluginCallback(const MStringArray& strs, void* clientData) {
    // Considered having separate plugin loaded/unloaded callbacks, but that
    // would mean checking for the plugin "name", which seems somewhat
    // unreliable - it's just the name of the built library, which seems too
    // easy to alter.
    // Instead, we check if the node is registered.

    TF_DEBUG(HDMAYA_AL_CALLBACKS)
        .Msg(
            "HdMayaProxyDelegate - PluginCallback - %s - %s\n",
            strs.length() > 0 ? strs[0].asChar() : "<none>",
            strs.length() > 1 ? strs[1].asChar() : "<none");

    bool isCurrentlyLoaded = IsALPluginLoaded();
    bool wasLoaded = isALPluginLoaded.exchange(isCurrentlyLoaded);
    if (wasLoaded != isCurrentlyLoaded) {
        if (TfDebug::IsEnabled(HDMAYA_AL_CALLBACKS)) {
            if (isCurrentlyLoaded) {
                TfDebug::Helper().Msg("ALUSDMayaPlugin loaded!\n");
            } else {
                TfDebug::Helper().Msg("ALUSDMayaPlugin unloaded!\n");
            }
        }
        // AL plugin was either loaded or unloaded - either way, need to reset
        // the renderOverride to either add / remove our AL delegate
        HdMayaDelegateRegistry::SignalDelegatesChanged();
    }
}

void SetupPluginCallbacks() {
    MStatus status;

    isALPluginLoaded.store(IsALPluginLoaded());

    // Set up callback to notify of plugin load
    TF_DEBUG(HDMAYA_AL_CALLBACKS)
        .Msg("HdMayaProxyDelegate - creating PluginLoaded callback\n");
    MSceneMessage::addStringArrayCallback(
        MSceneMessage::kAfterPluginLoad, PluginCallback, nullptr, &status);
    TF_VERIFY(status, "Could not set pluginLoaded callback");

    // Set up callback to notify of plugin unload
    TF_DEBUG(HDMAYA_AL_CALLBACKS)
        .Msg("HdMayaProxyDelegate - creating PluginUnloaded callback\n");
    MSceneMessage::addStringArrayCallback(
        MSceneMessage::kAfterPluginUnload, PluginCallback, nullptr, &status);
    TF_VERIFY(status, "Could not set pluginUnloaded callback");
}

} // namespace

HdMayaProxyDelegate::HdMayaProxyDelegate(const InitData& initData)
    : HdMayaDelegate(initData) {
    TF_DEBUG(HDMAYA_AL_PROXY_DELEGATE)
        .Msg(
            "HdMayaProxyDelegate - creating with delegateID %s\n",
            GetMayaDelegateID().GetText());

    MStatus status;

#if WANT_UFE_BUILD
    if (usdUfeRtid == 0) {
        try {
            usdUfeRtid =
                UFE_NS::RunTimeMgr::instance().getId(USD_UFE_RUNTIME_NAME);
        }
        // This shoudl catch ufe's InvalidRunTimeName exception, but they don't
        // expose that!
        catch (...) {
            TF_WARN("USD UFE Runtime plugin not loaded!\n");
        }
    }
#endif // WANT_UFE_BUILD
}

HdMayaProxyDelegate::~HdMayaProxyDelegate() {
    TF_DEBUG(HDMAYA_AL_PROXY_DELEGATE)
        .Msg(
            "HdMayaProxyDelegate - destroying with delegateID %s\n",
            GetMayaDelegateID().GetText());
}

HdMayaDelegatePtr HdMayaProxyDelegate::Creator(const InitData& initData) {
    static std::once_flag setupPluginCallbacksOnce;
    std::call_once(setupPluginCallbacksOnce, SetupPluginCallbacks);

    if (!isALPluginLoaded.load()) { return nullptr; }
    return std::static_pointer_cast<HdMayaDelegate>(
        std::make_shared<HdMayaProxyDelegate>(initData));
}

void HdMayaProxyDelegate::AddAdapter(HdMayaProxyAdapter* adapter) {
    std::lock_guard<std::mutex> lock(_allAdaptersMutex);
    _allAdapters.insert(adapter);
}

void HdMayaProxyDelegate::RemoveAdapter(HdMayaProxyAdapter* adapter) {
    std::lock_guard<std::mutex> lock(_allAdaptersMutex);
    _allAdapters.erase(adapter);
}

void HdMayaProxyDelegate::Populate() {
    // Does nothing - delegate exists only for PreFrame and
    // PopulateSelectedPaths
}

void HdMayaProxyDelegate::PreFrame(const MHWRender::MDrawContext& context) {
    std::lock_guard<std::mutex> lock(_allAdaptersMutex);
    for (auto adapter : _allAdapters) { adapter->PreFrame(); }
}

#if WANT_UFE_BUILD

void HdMayaProxyDelegate::PopulateSelectedPaths(
    const UFE_NS::Selection& ufeSelection, SdfPathVector& selectedSdfPaths,
    const HdSelectionSharedPtr& selection) {
    MStatus status;
    MObject proxyMObj;
    MFnDagNode proxyMFnDag;

    TF_DEBUG(HDMAYA_AL_SELECTION)
        .Msg(
            "HdMayaProxyDelegate::PopulateSelectedPaths (ufe version) - ufe "
            "sel size: %lu\n",
            ufeSelection.size());

    // We get the maya selection for the whole-proxy-selected check, since
    // it is a subset of the ufe selection
    MSelectionList mayaSel;
    MGlobal::getActiveSelectionList(mayaSel);

    std::unordered_map<std::string, HdMayaProxyAdapter*> proxyPathToAdapter;

    {
        // new context for the _allAdaptersMutex lock
        std::lock_guard<std::mutex> lock(_allAdaptersMutex);
        for (auto adapter : _allAdapters) {
            // First, we check to see if the entire proxy shape is selected
            bool wholeProxySelected = false;
            const MDagPath& dagPath = adapter->GetDagPath();
            // Loop over all parents
            MDagPath parentDag = dagPath;
            while (parentDag.length()) {
                if (mayaSel.hasItem(parentDag)) {
                    // The whole proxy is selected - HdMayaProxyAdapter's
                    // PopulateSelectedPaths will handle this case. we can
                    // skip this shape...
                    TF_DEBUG(HDMAYA_AL_SELECTION)
                        .Msg(
                            "proxy node %s was selected\n",
                            parentDag.fullPathName().asChar());
                    wholeProxySelected = true;
                    break;
                }
                parentDag.pop();
            }
            if (!wholeProxySelected) {
                TF_DEBUG(HDMAYA_AL_SELECTION)
                    .Msg(
                        "HdMayaProxyDelegate::PopulateSelectedPaths - adding "
                        "proxy to lookup: %s\n",
                        dagPath.fullPathName().asChar());
                proxyPathToAdapter.emplace(
                    dagPath.fullPathName().asChar(), adapter);
            }
        }
    }

    for (auto item : ufeSelection) {
        if (item->runTimeId() != usdUfeRtid) { continue; }
        auto& pathSegments = item->path().getSegments();
        if (pathSegments.size() != 2) {
            TF_WARN(
                "Found invalid usd-ufe path (had %lu segments - should have "
                "2): %s\n",
                item->path().size(), item->path().string().c_str());
            continue;
        }
        // We popHead for maya pathSegment because it always starts with
        // "|world", which makes it non-standard...
        auto mayaPathSegment = pathSegments[0].popHead();
        auto& usdPathSegment = pathSegments[1];

        auto findResult = proxyPathToAdapter.find(mayaPathSegment.string());

        TF_DEBUG(HDMAYA_AL_SELECTION)
            .Msg(
                "HdMayaProxyDelegate::PopulateSelectedPaths - looking up "
                "proxy: %s\n",
                mayaPathSegment.string().c_str());

        if (findResult == proxyPathToAdapter.cend()) { continue; }

        auto proxyAdapter = findResult->second;

        selectedSdfPaths.push_back(proxyAdapter->ConvertCachePathToIndexPath(
            SdfPath(usdPathSegment.string())));
        selection->AddRprim(
            HdSelection::HighlightModeSelect, selectedSdfPaths.back());
        TF_DEBUG(HDMAYA_AL_SELECTION)
            .Msg(
                "HdMayaProxyDelegate::PopulateSelectedPaths - selecting %s\n",
                selectedSdfPaths.back().GetText());
    }
}

bool HdMayaProxyDelegate::SupportsUfeSelection() { return usdUfeRtid != 0; }

#endif // WANT_UFE_BUILD

PXR_NAMESPACE_CLOSE_SCOPE
