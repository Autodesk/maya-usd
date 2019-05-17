//
// Copyright 2019 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http:#www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "alProxyDelegate.h"

#include "alProxyAdapter.h"
#include "debugCodes.h"

#include <hdmaya/delegates/delegateRegistry.h>

#include <pxr/base/tf/envSetting.h>
#include <pxr/base/tf/type.h>

#include <maya/MDGMessage.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MNodeClass.h>
#include <maya/MObject.h>

#include <atomic>
#include <mutex>
#include <unordered_set>

#if HDMAYA_UFE_BUILD
#include <ufe/rtid.h>
#include <ufe/runTimeMgr.h>
#endif // HDMAYA_UFE_BUILD

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(_tokens, (HdMayaALProxyDelegate));

TF_REGISTRY_FUNCTION(TfType) {
    TF_DEBUG(HDMAYA_AL_PLUGIN)
        .Msg("Calling TfType::Define for HdMayaALProxyDelegate\n");
    TfType::Define<HdMayaALProxyDelegate, TfType::Bases<HdMayaDelegate> >();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaDelegateRegistry, HdMayaALProxyDelegate) {
    TF_DEBUG(HDMAYA_AL_PLUGIN)
        .Msg("Calling RegisterDelegate for HdMayaALProxyDelegate\n");
    HdMayaDelegateRegistry::RegisterDelegate(
        _tokens->HdMayaALProxyDelegate, HdMayaALProxyDelegate::Creator);
}

namespace {

#if HDMAYA_UFE_BUILD
constexpr auto USD_UFE_RUNTIME_NAME = "USD";
static UFE_NS::Rtid usdUfeRtid = 0;
#endif // HDMAYA_UFE_BUILD

// Don't know if this variable would be accessed from multiple threads, but
// plugin load/unload is infrequent enough that performance isn't an issue, and
// I prefer to default to thread-safety for global variables.
std::atomic<bool> isALPluginLoaded(false);

// Not sure if we actually need a mutex guarding _allAdapters, but
// I'd rather be safe....
std::mutex _allAdaptersMutex;
std::unordered_set<HdMayaALProxyAdapter*> _allAdapters;

bool IsALPluginLoaded() {
    auto nodeClass = MNodeClass(ProxyShape::kTypeId);
    // if not loaded yet, typeName() will be an empty string
    return nodeClass.typeName() == ProxyShape::kTypeName;
}

void PluginCallback(const MStringArray& strs, void* clientData) {
    // Considered having separate plugin loaded/unloaded callbacks, but that
    // would mean checking for the plugin "name", which seems somewhat
    // unreliable - it's just the name of the built library, which seems too
    // easy to alter.
    // Instead, we check if the node is registered.

    TF_DEBUG(HDMAYA_AL_CALLBACKS)
        .Msg(
            "HdMayaALProxyDelegate - PluginCallback - %s - %s\n",
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
        .Msg("HdMayaALProxyDelegate - creating PluginLoaded callback\n");
    MSceneMessage::addStringArrayCallback(
        MSceneMessage::kAfterPluginLoad, PluginCallback, nullptr, &status);
    TF_VERIFY(status, "Could not set pluginLoaded callback");

    // Set up callback to notify of plugin unload
    TF_DEBUG(HDMAYA_AL_CALLBACKS)
        .Msg("HdMayaALProxyDelegate - creating PluginUnloaded callback\n");
    MSceneMessage::addStringArrayCallback(
        MSceneMessage::kAfterPluginUnload, PluginCallback, nullptr, &status);
    TF_VERIFY(status, "Could not set pluginUnloaded callback");
}

} // namespace

HdMayaALProxyDelegate::HdMayaALProxyDelegate(const InitData& initData)
    : HdMayaDelegate(initData) {
    TF_DEBUG(HDMAYA_AL_PROXY_DELEGATE)
        .Msg(
            "HdMayaALProxyDelegate - creating with delegateID %s\n",
            GetMayaDelegateID().GetText());

    MStatus status;

#if HDMAYA_UFE_BUILD
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
#endif // HDMAYA_UFE_BUILD
}

HdMayaALProxyDelegate::~HdMayaALProxyDelegate() {
    TF_DEBUG(HDMAYA_AL_PROXY_DELEGATE)
        .Msg(
            "HdMayaALProxyDelegate - destroying with delegateID %s\n",
            GetMayaDelegateID().GetText());
}

HdMayaDelegatePtr HdMayaALProxyDelegate::Creator(const InitData& initData) {
    static std::once_flag setupPluginCallbacksOnce;
    std::call_once(setupPluginCallbacksOnce, SetupPluginCallbacks);

    if (!isALPluginLoaded.load()) { return nullptr; }
    return std::static_pointer_cast<HdMayaDelegate>(
        std::make_shared<HdMayaALProxyDelegate>(initData));
}

void HdMayaALProxyDelegate::AddAdapter(HdMayaALProxyAdapter* adapter) {
    std::lock_guard<std::mutex> lock(_allAdaptersMutex);
    _allAdapters.insert(adapter);
}

void HdMayaALProxyDelegate::RemoveAdapter(HdMayaALProxyAdapter* adapter) {
    std::lock_guard<std::mutex> lock(_allAdaptersMutex);
    _allAdapters.erase(adapter);
}

void HdMayaALProxyDelegate::Populate() {
    // Does nothing - delegate exists only for PreFrame and
    // PopulateSelectedPaths
}

void HdMayaALProxyDelegate::PreFrame(const MHWRender::MDrawContext& context) {
    std::lock_guard<std::mutex> lock(_allAdaptersMutex);
    for (auto adapter : _allAdapters) { adapter->PreFrame(); }
}

void HdMayaALProxyDelegate::PopulateSelectedPaths(
    const MSelectionList& mayaSelection, SdfPathVector& selectedSdfPaths,
    const HdSelectionSharedPtr& selection) {
    MStatus status;
    MObject proxyMObj;
    MFnDagNode proxyMFnDag;

    std::lock_guard<std::mutex> lock(_allAdaptersMutex);
    for (auto adapter : _allAdapters) {
        auto* proxy = adapter->GetProxy();
        if (!TF_VERIFY(proxy)) { continue; }
        proxyMObj = proxy->thisMObject();
        if (!TF_VERIFY(!proxyMObj.isNull())) { continue; }
        if (!TF_VERIFY(proxyMFnDag.setObject(proxyMObj))) { continue; }

        // First, we check to see if the entire proxy shape is selected
        bool wholeProxySelected = false;
        MDagPath dagPath = adapter->GetDagPath();
        // Loop over all parents
        while (dagPath.length()) {
            if (mayaSelection.hasItem(dagPath)) {
                // The whole proxy is selected - HdMayaAlProxyAdapter's
                // PopulateSelectedPaths will handle this case. we can
                // skip this shape...
                TF_DEBUG(HDMAYA_AL_SELECTION)
                    .Msg(
                        "proxy node %s was selected\n",
                        dagPath.fullPathName().asChar());
                wholeProxySelected = true;
                break;
            }
            dagPath.pop();
        }
        if (wholeProxySelected) { continue; }

        // Ok, we didn't have the entire proxy selected - instead, add in
        // any "subpaths" of the proxy which may be selected

        // Not sure why we need both paths1 and paths2, or what the difference
        // is... but AL's own selection drawing code (in ProxyDrawOverride)
        // makes a combined list from both of these, so we do the same
        const auto& paths1 = proxy->selectedPaths();
        const auto& paths2 = proxy->selectionList().paths();
        selectedSdfPaths.reserve(
            selectedSdfPaths.size() + paths1.size() + paths2.size());

        if (TfDebug::IsEnabled(HDMAYA_AL_SELECTION)) {
            TfDebug::Helper().Msg(
                "proxy %s has %lu selectedPaths",
                dagPath.fullPathName().asChar(), paths1.size());
            if (!paths1.empty()) {
                TfDebug::Helper().Msg(" (1st: %s)", paths1.begin()->GetText());
            }
            TfDebug::Helper().Msg(
                ", and %lu selectionList paths", paths2.size());
            if (!paths2.empty()) {
                TfDebug::Helper().Msg(" (1st: %s)", paths2.begin()->GetText());
            }
            TfDebug::Helper().Msg("\n");
        }

        for (auto& usdPath : paths1) {
            selectedSdfPaths.push_back(
                adapter->ConvertCachePathToIndexPath(usdPath));
            selection->AddRprim(
                HdSelection::HighlightModeSelect, selectedSdfPaths.back());
        }

        for (auto& usdPath : paths2) {
            selectedSdfPaths.push_back(
                adapter->ConvertCachePathToIndexPath(usdPath));
            selection->AddRprim(
                HdSelection::HighlightModeSelect, selectedSdfPaths.back());
        }
    }
}

#if HDMAYA_UFE_BUILD

void HdMayaALProxyDelegate::PopulateSelectedPaths(
    const UFE_NS::Selection& ufeSelection, SdfPathVector& selectedSdfPaths,
    const HdSelectionSharedPtr& selection) {
    MStatus status;
    MObject proxyMObj;
    MFnDagNode proxyMFnDag;

    TF_DEBUG(HDMAYA_AL_SELECTION)
        .Msg(
            "HdMayaALProxyDelegate::PopulateSelectedPaths (ufe version) - ufe "
            "sel size: %lu\n",
            ufeSelection.size());

    // We get the maya selection for the whole-proxy-selected check, since
    // it is a subset of the ufe selection
    MSelectionList mayaSel;
    MGlobal::getActiveSelectionList(mayaSel);

    std::unordered_map<std::string, HdMayaALProxyAdapter*> proxyPathToAdapter;

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
                    // The whole proxy is selected - HdMayaAlProxyAdapter's
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
                        "HdMayaALProxyDelegate::PopulateSelectedPaths - adding "
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
                "HdMayaALProxyDelegate::PopulateSelectedPaths - looking up "
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
                "HdMayaALProxyDelegate::PopulateSelectedPaths - selecting %s\n",
                selectedSdfPaths.back().GetText());
    }
}

bool HdMayaALProxyDelegate::SupportsUfeSelection() { return usdUfeRtid != 0; }

#endif // HDMAYA_UFE_BUILD

PXR_NAMESPACE_CLOSE_SCOPE
