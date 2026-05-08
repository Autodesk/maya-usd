//
// Copyright 2026 Autodesk
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

#include "usdSceneSettingsManager.h"

#include <mayaUsd/utils/blockSceneModificationContext.h>

#include <maya/MDGModifier.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MPlug.h>
#include <maya/MSceneMessage.h>

namespace MAYAUSD_NS_DEF {

// ---------------------------------------------------------------------------
// Static member definitions
// ---------------------------------------------------------------------------

MCallbackId UsdSceneSettingsManager::afterNewCbId = 0;
MCallbackId UsdSceneSettingsManager::afterOpenCbId = 0;
MCallbackId UsdSceneSettingsManager::beforeSaveCbId = 0;
bool        UsdSceneSettingsManager::isPluginInitialized = false;

// ---------------------------------------------------------------------------
// Storage accessors
// ---------------------------------------------------------------------------

/* static */
std::map<std::string, UsdSceneSettingsManager::Populator>& UsdSceneSettingsManager::registry()
{
    static std::map<std::string, Populator> r;
    return r;
}

/* static */
std::map<std::string, MObjectHandle>& UsdSceneSettingsManager::instances()
{
    static std::map<std::string, MObjectHandle> i;
    return i;
}

/* static */
UsdSceneSettingsManager::StageObserverHook& UsdSceneSettingsManager::stageObserverHook()
{
    static StageObserverHook h;
    return h;
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

/* static */
void UsdSceneSettingsManager::registerSettingNode(const std::string& nodeName, Populator populate)
{
    registry()[nodeName] = std::move(populate);

    // If the plugin is already running, create the node immediately so callers
    // registered after plugin init (e.g. sub-plugins) get a live node at once.
    if (isPluginInitialized) {
        findOrCreate(nodeName);
    }
}

/* static */
void UsdSceneSettingsManager::callPopulator(
    const std::string&     nodeName,
    PXR_NS::UsdStageRefPtr stage)
{
    // Centralize the stage null-guard here so individual populators do not
    // each have to repeat it. ensureStage may pass a null stage if
    // UsdStage::Open returned null.
    if (!stage) {
        return;
    }
    auto it = registry().find(nodeName);
    if (it != registry().end() && it->second) {
        it->second(stage);
    }
}

// ---------------------------------------------------------------------------
// Node access
// ---------------------------------------------------------------------------

/* static */
MObject UsdSceneSettingsManager::find(const std::string& nodeName)
{
    auto it = instances().find(nodeName);
    if (it == instances().end() || !it->second.isValid()) {
        return MObject::kNullObj;
    }
    return it->second.object();
}

/* static */
MObject UsdSceneSettingsManager::findOrCreate(const std::string& nodeName)
{
    MObject existing = find(nodeName);
    if (!existing.isNull()) {
        return existing;
    }

    // Refuse during teardown so a callback fired by deleteNode in
    // onPluginFinalize() can't resurrect a managed node moments before
    // deregisterNode(typeId) runs.
    if (!isPluginInitialized) {
        return MObject::kNullObj;
    }

    // Node creation must not mark the scene as having unsaved changes.
    MayaUsd::utils::BlockSceneModificationContext blockModContext;

    MObject nodeObj = createNode(nodeName);
    if (!nodeObj.isNull()) {
        instances()[nodeName] = MObjectHandle(nodeObj);
    }

    return nodeObj;
}

/* static */
PXR_NS::UsdStageRefPtr UsdSceneSettingsManager::getStage(const std::string& nodeName)
{
    MObject obj = findOrCreate(nodeName);
    if (obj.isNull()) {
        return nullptr;
    }
    MFnDependencyNode depFn(obj);
    auto*             node = dynamic_cast<UsdSettingsNode*>(depFn.userNode());
    if (!node) {
        return nullptr;
    }
    return node->getUsdStage();
}

/* static */
PXR_NS::UsdStageRefPtr UsdSceneSettingsManager::getStageForNodeName(const std::string& nodeName)
{
    // Never create a node here. The instances map is keyed by DG node name
    // (see createNode) and is the authoritative source for managed nodes.
    // The stage itself is materialized lazily on first access
    // (UsdSettingsNode::ensureStage), which runs the populator and fires
    // the stage observer hook.
    auto it = instances().find(nodeName);
    if (it == instances().end() || !it->second.isValid()) {
        return nullptr;
    }
    UsdSettingsNode* node = nodeFromHandle(it->second);
    if (!node) {
        return nullptr;
    }
    return node->getUsdStage();
}

/* static */
MObject UsdSceneSettingsManager::nodeForStage(PXR_NS::UsdStageWeakPtr stage)
{
    if (!stage) {
        return MObject::kNullObj;
    }
    for (const auto& [nodeName, handle] : instances()) {
        if (!handle.isValid()) {
            continue;
        }
        UsdSettingsNode* node = nodeFromHandle(handle);
        // Gate on hasStage() so this never triggers lazy stage creation.
        if (!node || !node->hasStage()) {
            continue;
        }
        if (PXR_NS::UsdStageWeakPtr(node->getUsdStage()) == stage) {
            return handle.object();
        }
    }
    return MObject::kNullObj;
}

/* static */
std::vector<PXR_NS::UsdStageRefPtr> UsdSceneSettingsManager::getAllLiveStages()
{
    std::vector<PXR_NS::UsdStageRefPtr> result;
    result.reserve(instances().size());
    for (const auto& [nodeName, handle] : instances()) {
        UsdSettingsNode* node = nodeFromHandle(handle);
        // Gate on hasStage() so this never triggers lazy stage creation.
        if (node && node->hasStage()) {
            result.push_back(node->getUsdStage());
        }
    }
    return result;
}

/* static */
void UsdSceneSettingsManager::setStageObserverHook(StageObserverHook hook)
{
    stageObserverHook() = std::move(hook);
}

/* static */
void UsdSceneSettingsManager::notifyStageObserver(const PXR_NS::UsdStageRefPtr& stage)
{
    if (stageObserverHook() && stage) {
        stageObserverHook()(stage);
    }
}

// ---------------------------------------------------------------------------
// Plugin lifecycle
// ---------------------------------------------------------------------------

/* static */
void UsdSceneSettingsManager::onPluginInitialize()
{
    MStatus status;

    afterNewCbId
        = MSceneMessage::addCallback(MSceneMessage::kAfterNew, onAfterNew, nullptr, &status);
    CHECK_MSTATUS(status);

    afterOpenCbId = MSceneMessage::addCallback(
        MSceneMessage::kAfterSceneReadAndRecordEdits, onAfterOpen, nullptr, &status);
    CHECK_MSTATUS(status);

    beforeSaveCbId
        = MSceneMessage::addCallback(MSceneMessage::kBeforeSave, onBeforeSave, nullptr, &status);
    CHECK_MSTATUS(status);

    isPluginInitialized = true;

    // kAfterNew does not fire for the default scene present at Maya startup;
    // create nodes for all registered node names now.
    for (const auto& [nodeName, populate] : registry()) {
        findOrCreate(nodeName);
    }
}

/* static */
void UsdSceneSettingsManager::onPluginFinalize()
{
    if (afterNewCbId) {
        MSceneMessage::removeCallback(afterNewCbId);
        afterNewCbId = 0;
    }
    if (afterOpenCbId) {
        MSceneMessage::removeCallback(afterOpenCbId);
        afterOpenCbId = 0;
    }
    if (beforeSaveCbId) {
        MSceneMessage::removeCallback(beforeSaveCbId);
        beforeSaveCbId = 0;
    }

    isPluginInitialized = false;

    // Snapshot the live handles, then clear the map BEFORE deletion so any
    // callback fired during MDGModifier::doIt() (which can call back through
    // find* / findOrCreate) sees a clean state and does not resurrect a
    // managed node we are about to deregister. findOrCreate() also refuses
    // to create new nodes once isPluginInitialized is false (see above).
    std::vector<MObject> toDelete;
    toDelete.reserve(instances().size());
    for (auto& [nodeName, handle] : instances()) {
        if (handle.isValid()) {
            toDelete.push_back(handle.object());
        }
    }
    instances().clear();

    for (MObject& obj : toDelete) {
        MFnDependencyNode depFn(obj);
        depFn.setLocked(false);

        MDGModifier modifier;
        modifier.deleteNode(obj);
        modifier.doIt();
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

/* static */
MObject UsdSceneSettingsManager::createNode(const std::string& nodeName)
{
    auto it = registry().find(nodeName);
    if (it == registry().end()) {
        MGlobal::displayError(
            MString("[UsdSceneSettingsManager] No registered node name: ") + nodeName.c_str());
        return MObject::kNullObj;
    }

    MDGModifier modifier;
    MObject     nodeObj = modifier.createNode(UsdSettingsNode::typeName);
    modifier.doIt();

    if (nodeObj.isNull()) {
        return MObject::kNullObj;
    }

    // Apply the requested DG name and lock the node so the name cannot drift.
    // The name is the manager's primary lookup key; if Maya silently suffixed
    // it to disambiguate, every subsequent find() / getStageForNodeName() /
    // UFE dispatch would miss.
    MFnDependencyNode depFn(nodeObj);
    MStatus           nameStatus;
    const MString     requestedName(nodeName.c_str());
    const MString     actualName = depFn.setName(requestedName, &nameStatus);
    if (nameStatus != MS::kSuccess || actualName != requestedName) {
        MGlobal::displayError(
            MString("[UsdSceneSettingsManager] could not apply requested name '") + requestedName
            + "' to a freshly created " + UsdSettingsNode::typeName + " (got '" + actualName
            + "').");
        MDGModifier deleter;
        deleter.deleteNode(nodeObj);
        deleter.doIt();
        return MObject::kNullObj;
    }
    depFn.setLocked(true);

    return nodeObj;
}

/* static */
void UsdSceneSettingsManager::rebuildInstancesFromScene()
{
    instances().clear();

    MItDependencyNodes scan(MFn::kPluginDependNode);
    while (!scan.isDone()) {
        MObject           obj = scan.thisNode();
        MFnDependencyNode depFn(obj);

        if (depFn.typeId() == UsdSettingsNode::typeId && !depFn.isFromReferencedFile()) {
            // The DG node name is the registry key (set at creation, stable under lock).
            std::string nodeName = depFn.name().asChar();
            if (!nodeName.empty() && registry().count(nodeName) && !instances().count(nodeName)) {
                instances()[nodeName] = MObjectHandle(obj);
            }
        }
        scan.next();
    }
}

/* static */
UsdSettingsNode* UsdSceneSettingsManager::nodeFromHandle(const MObjectHandle& handle)
{
    if (!handle.isValid()) {
        return nullptr;
    }
    MFnDependencyNode depFn(handle.object());
    return dynamic_cast<UsdSettingsNode*>(depFn.userNode());
}

// ---------------------------------------------------------------------------
// Scene message callbacks
// ---------------------------------------------------------------------------

/* static */
void UsdSceneSettingsManager::onAfterNew(void* /*clientData*/)
{
    instances().clear();
    for (const auto& [nodeName, populate] : registry()) {
        findOrCreate(nodeName);
    }
}

/* static */
void UsdSceneSettingsManager::onAfterOpen(void* /*clientData*/)
{
    // Rebuild the instance map from what was actually loaded from the file.
    rebuildInstancesFromScene();

    // Deserialize each found node, but only when the stage is not yet live.
    // kAfterSceneReadAndRecordEdits fires for File > Open, Import, and Reference;
    // skipping nodes whose stage already exists avoids clobbering live data when
    // the trigger is an Import or Reference rather than a fresh Open.
    for (const auto& [nodeName, handle] : instances()) {
        UsdSettingsNode* node = nodeFromHandle(handle);
        if (node && !node->hasStage()) {
            node->deserializeFromAttributes();
        }
    }

    // Create nodes for any registered node name not present in the loaded file.
    for (const auto& [nodeName, populate] : registry()) {
        if (!instances().count(nodeName) || !instances()[nodeName].isValid()) {
            findOrCreate(nodeName);
        }
    }
}

/* static */
void UsdSceneSettingsManager::onBeforeSave(void* /*clientData*/)
{
    for (const auto& [nodeName, handle] : instances()) {
        UsdSettingsNode* node = nodeFromHandle(handle);
        if (node) {
            node->serializeToAttributes();
        }
    }
}

} // namespace MAYAUSD_NS_DEF
