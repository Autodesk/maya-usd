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

#include <hdMaya/adapters/proxyAdapter.h>
#include <hdMaya/debugCodes.h>
#include <hdMaya/delegates/delegateRegistry.h>
#include <mayaUsd/nodes/proxyShapeBase.h>

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
#include <ufe/globalSelection.h>
#ifdef UFE_V2_FEATURES_AVAILABLE
#include <ufe/namedSelection.h>
#endif
#include <mayaUsd/ufe/Utils.h>

#include <ufe/observableSelection.h>
#endif // WANT_UFE_BUILD

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (HdMayaProxyDelegate)
);
// clang-format off

TF_REGISTRY_FUNCTION(TfType)
{
    TF_DEBUG(HDMAYA_AL_PLUGIN).Msg("Calling TfType::Define for HdMayaProxyDelegate\n");
    TfType::Define<HdMayaProxyDelegate, TfType::Bases<HdMayaDelegate>>();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaDelegateRegistry, HdMayaProxyDelegate)
{
    TF_DEBUG(HDMAYA_AL_PLUGIN).Msg("Calling RegisterDelegate for HdMayaProxyDelegate\n");
    HdMayaDelegateRegistry::RegisterDelegate(
        _tokens->HdMayaProxyDelegate, HdMayaProxyDelegate::Creator);
}

namespace {

// Don't know if this variable would be accessed from multiple threads, but
// plugin load/unload is infrequent enough that performance isn't an issue, and
// I prefer to default to thread-safety for global variables.
std::atomic<bool> isALPluginLoaded(false);

// Not sure if we actually need a mutex guarding _allAdapters, but
// I'd rather be safe....
std::mutex                              _allAdaptersMutex;
std::unordered_set<HdMayaProxyAdapter*> _allAdapters;

bool IsALPluginLoaded()
{
    auto nodeClass = MNodeClass(MayaUsdProxyShapeBase::typeId);
    // if not loaded yet, typeName() will be an empty string
    return nodeClass.typeName() == MayaUsdProxyShapeBase::typeName;
}

void PluginCallback(const MStringArray& strs, void* clientData)
{
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

void SetupPluginCallbacks()
{
    MStatus status;

    isALPluginLoaded.store(IsALPluginLoaded());

    // Set up callback to notify of plugin load
    TF_DEBUG(HDMAYA_AL_CALLBACKS).Msg("HdMayaProxyDelegate - creating PluginLoaded callback\n");
    MSceneMessage::addStringArrayCallback(
        MSceneMessage::kAfterPluginLoad, PluginCallback, nullptr, &status);
    TF_VERIFY(status, "Could not set pluginLoaded callback");

    // Set up callback to notify of plugin unload
    TF_DEBUG(HDMAYA_AL_CALLBACKS).Msg("HdMayaProxyDelegate - creating PluginUnloaded callback\n");
    MSceneMessage::addStringArrayCallback(
        MSceneMessage::kAfterPluginUnload, PluginCallback, nullptr, &status);
    TF_VERIFY(status, "Could not set pluginUnloaded callback");
}

#ifndef UFE_V2_FEATURES_AVAILABLE
#if (MAYA_API_VERSION >= 20210000) && WANT_UFE_BUILD
MGlobal::ListAdjustment GetListAdjustment()
{
    // Keyboard modifiers can be queried from QApplication::keyboardModifiers()
    // in case running MEL command leads to performance hit. On the other hand
    // the advantage of using MEL command is the platform-agnostic state of the
    // CONTROL key that it provides for aligning to Maya's implementation.
    int modifiers = 0;
    MGlobal::executeCommand("getModifiers", modifiers);

    const bool shiftHeld = (modifiers % 2);
    const bool ctrlHeld = (modifiers / 4 % 2);

    MGlobal::ListAdjustment listAdjustment = MGlobal::kReplaceList;

    if (shiftHeld && ctrlHeld) {
        listAdjustment = MGlobal::kAddToList;
    } else if (ctrlHeld) {
        listAdjustment = MGlobal::kRemoveFromList;
    } else if (shiftHeld) {
        listAdjustment = MGlobal::kXORWithList;
    }

    return listAdjustment;
}
#endif
#endif

} // namespace

HdMayaProxyDelegate::HdMayaProxyDelegate(const InitData& initData)
    : HdMayaDelegate(initData)
{
    TF_DEBUG(HDMAYA_AL_PROXY_DELEGATE)
        .Msg("HdMayaProxyDelegate - creating with delegateID %s\n", GetMayaDelegateID().GetText());
}

HdMayaProxyDelegate::~HdMayaProxyDelegate()
{
    TF_DEBUG(HDMAYA_AL_PROXY_DELEGATE)
        .Msg(
            "HdMayaProxyDelegate - destroying with delegateID %s\n", GetMayaDelegateID().GetText());
}

HdMayaDelegatePtr HdMayaProxyDelegate::Creator(const InitData& initData)
{
    static std::once_flag setupPluginCallbacksOnce;
    std::call_once(setupPluginCallbacksOnce, SetupPluginCallbacks);

    if (!isALPluginLoaded.load()) {
        return nullptr;
    }
    return std::static_pointer_cast<HdMayaDelegate>(
        std::make_shared<HdMayaProxyDelegate>(initData));
}

void HdMayaProxyDelegate::AddAdapter(HdMayaProxyAdapter* adapter)
{
    std::lock_guard<std::mutex> lock(_allAdaptersMutex);
    _allAdapters.insert(adapter);
}

void HdMayaProxyDelegate::RemoveAdapter(HdMayaProxyAdapter* adapter)
{
    std::lock_guard<std::mutex> lock(_allAdaptersMutex);
    _allAdapters.erase(adapter);
}

void HdMayaProxyDelegate::Populate()
{
    // Does nothing - delegate exists only for PreFrame and
    // PopulateSelectedPaths
}

void HdMayaProxyDelegate::PreFrame(const MHWRender::MDrawContext& context)
{
    std::lock_guard<std::mutex> lock(_allAdaptersMutex);
    for (auto adapter : _allAdapters) {
        adapter->PreFrame(context);
    }
}

#if WANT_UFE_BUILD

void HdMayaProxyDelegate::PopulateSelectedPaths(
    const UFE_NS::Selection&    ufeSelection,
    SdfPathVector&              selectedSdfPaths,
    const HdSelectionSharedPtr& selection)
{
    MStatus    status;
    MObject    proxyMObj;
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
            bool            wholeProxySelected = false;
            const MDagPath& dagPath = adapter->GetDagPath();
            // Loop over all parents
            MDagPath parentDag = dagPath;
            while (parentDag.length()) {
                if (mayaSel.hasItem(parentDag)) {
                    // The whole proxy is selected - HdMayaProxyAdapter's
                    // PopulateSelectedPaths will handle this case. we can
                    // skip this shape...
                    TF_DEBUG(HDMAYA_AL_SELECTION)
                        .Msg("proxy node %s was selected\n", parentDag.fullPathName().asChar());
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
                proxyPathToAdapter.emplace(dagPath.fullPathName().asChar(), adapter);
            }
        }
    }

    for (auto item : ufeSelection) {
        if (item->runTimeId() != MayaUsd::ufe::getUsdUfeRuntimeId()) {
            continue;
        }
        auto& pathSegments = item->path().getSegments();
        if (pathSegments.size() != 2) {
            TF_WARN(
                "Found invalid usd-ufe path (had %lu segments - should have "
                "2): %s\n",
                item->path().size(),
                item->path().string().c_str());
            continue;
        }
        // We popHead for maya pathSegment because it always starts with
        // "|world", which makes it non-standard...
        auto  mayaPathSegment = pathSegments[0].popHead();
        auto& usdPathSegment = pathSegments[1];

        auto findResult = proxyPathToAdapter.find(mayaPathSegment.string());

        TF_DEBUG(HDMAYA_AL_SELECTION)
            .Msg(
                "HdMayaProxyDelegate::PopulateSelectedPaths - looking up "
                "proxy: %s\n",
                mayaPathSegment.string().c_str());

        if (findResult == proxyPathToAdapter.cend()) {
            continue;
        }

        auto proxyAdapter = findResult->second;

        const SdfPath usdPath(usdPathSegment.string());
        selectedSdfPaths.push_back(proxyAdapter->ConvertCachePathToIndexPath(usdPath));
        proxyAdapter->PopulateSelection(
            HdSelection::HighlightModeSelect,
            usdPath,
            UsdImagingDelegate::ALL_INSTANCES,
            selection);
        TF_DEBUG(HDMAYA_AL_SELECTION)
            .Msg(
                "HdMayaProxyDelegate::PopulateSelectedPaths - selecting %s\n",
                selectedSdfPaths.back().GetText());
    }
}

bool HdMayaProxyDelegate::SupportsUfeSelection() { return MayaUsd::ufe::getUsdUfeRuntimeId() != 0; }

#endif // WANT_UFE_BUILD

#if MAYA_API_VERSION >= 20210000
void HdMayaProxyDelegate::PopulateSelectionList(
    const HdxPickHitVector&          hits,
    const MHWRender::MSelectionInfo& selectInfo,
    MSelectionList&                  selectionList,
    MPointArray&                     worldSpaceHitPts)
{
    if (selectInfo.pointSnapping()) {
        std::lock_guard<std::mutex> lock(_allAdaptersMutex);

        for (const HdxPickHit& hit : hits) {
            for (auto adapter : _allAdapters) {
                const SdfPath& delegateId = adapter->GetUsdDelegateID();
                if (hit.objectId.HasPrefix(delegateId)) {
                    selectionList.add(adapter->GetDagPath());
                    worldSpaceHitPts.append(
                        hit.worldSpaceHitPoint[0],
                        hit.worldSpaceHitPoint[1],
                        hit.worldSpaceHitPoint[2]);

                    break;
                }
            }
        }
        return;
    }

#if WANT_UFE_BUILD
    auto handler = Ufe::RunTimeMgr::instance().hierarchyHandler(USD_UFE_RUNTIME_ID);
    if (handler == nullptr)
        return;

#ifdef UFE_V2_FEATURES_AVAILABLE
    auto ufeSel = Ufe::NamedSelection::get("MayaSelectTool");
#else
    const MGlobal::ListAdjustment listAdjustment = GetListAdjustment();
#endif

    std::lock_guard<std::mutex> lock(_allAdaptersMutex);

    for (const HdxPickHit& hit : hits) {
        const SdfPath& objectId = hit.objectId;
        const int      instanceIndex = hit.instanceIndex;

        for (auto adapter : _allAdapters) {
            const SdfPath& delegateId = adapter->GetUsdDelegateID();
            if (!objectId.HasPrefix(delegateId)) {
                continue;
            }

            SdfPath usdPath = objectId.ReplacePrefix(delegateId, SdfPath::AbsoluteRootPath());

#if defined(USD_IMAGING_API_VERSION) && USD_IMAGING_API_VERSION >= 14
            usdPath = adapter->GetScenePrimPath(usdPath, instanceIndex, nullptr);
#elif defined(USD_IMAGING_API_VERSION) && USD_IMAGING_API_VERSION >= 13
            usdPath = adapter->GetScenePrimPath(usdPath, instanceIndex);
#else
            if (instanceIndex >= 0) {
                usdPath = adapter->GetPathForInstanceIndex(usdPath, instanceIndex, nullptr);
            }

            usdPath = adapter->ConvertIndexPathToCachePath(usdPath);
#endif

            const Ufe::PathSegment pathSegment(
                usdPath.GetText(), USD_UFE_RUNTIME_ID, USD_UFE_SEPARATOR);
            const Ufe::SceneItem::Ptr& si
                = handler->createItem(adapter->GetProxy()->ufePath() + pathSegment);
            if (!si) {
                TF_WARN("Failed to create UFE scene item for '%s'", objectId.GetText());
                break;
            }

#ifdef UFE_V2_FEATURES_AVAILABLE
            ufeSel->append(si);
#else
            auto globalSelection = Ufe::GlobalSelection::get();

            switch (listAdjustment) {
            case MGlobal::kReplaceList:
                // The list has been cleared before viewport selection runs, so we
                // can add the new hits directly. UFE selection list is a superset
                // of Maya selection list, calling clear()/replaceWith() on UFE
                // selection list would clear Maya selection list.
                globalSelection->append(si);
                break;
            case MGlobal::kAddToList: globalSelection->append(si); break;
            case MGlobal::kRemoveFromList: globalSelection->remove(si); break;
            case MGlobal::kXORWithList:
                if (!globalSelection->remove(si)) {
                    globalSelection->append(si);
                }
                break;
            default: TF_WARN("Unexpected MGlobal::ListAdjustment enum for selection."); break;
            }
#endif

            break;
        }
    }
#endif // WANT_UFE_BUILD
}
#endif // MAYA_API_VERSION >= 20210000

PXR_NAMESPACE_CLOSE_SCOPE
