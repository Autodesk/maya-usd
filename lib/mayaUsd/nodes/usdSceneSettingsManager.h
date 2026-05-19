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
#ifndef MAYAUSD_USD_SCENE_SETTINGS_MANAGER_H
#define MAYAUSD_USD_SCENE_SETTINGS_MANAGER_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/nodes/usdSettingsNode.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MMessage.h>
#include <maya/MObject.h>
#include <maya/MObjectHandle.h>

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace MAYAUSD_NS_DEF {

//! Plugin-side registry that owns the lifecycle of all UsdSettingsNode
//! instances. Each managed node is keyed by its locked Maya DG node name (e.g.
//! "UsdDefaultRenderSettings") and paired with a populator that seeds its USD
//! stage. The manager guarantees exactly one non-referenced instance per
//! registered name across File > New, Open, Save and Maya Reference workflows.
//! Activated from the mayaUsdPlugin entry point via onPluginInitialize() /
//! onPluginFinalize().
class MAYAUSD_CORE_PUBLIC UsdSceneSettingsManager
{
public:
    //! Seeds a managed node's USD stage and its node-level attributes (e.g.
    //! the activeSettingsPath plug) with domain-specific content.
    using Populator = std::function<void(PXR_NS::UsdStageRefPtr, UsdSettingsNode&)>;

    // -----------------------------------------------------------------------
    // Registration
    // -----------------------------------------------------------------------

    //! Register a settings node by its DG node name. If the plugin is already
    //! initialized (sub-plugin load), the node is created immediately.
    static void registerSettingNode(const std::string& nodeName, Populator populate);

    //! Invoke the populator for \p nodeName on \p stage and \p node. Called
    //! from UsdSettingsNode::ensureStage() and ::deserializeFromAttributes().
    static void
    callPopulator(const std::string& nodeName, PXR_NS::UsdStageRefPtr stage, UsdSettingsNode& node);

    // -----------------------------------------------------------------------
    // Node access
    // -----------------------------------------------------------------------

    //! Live MObject for \p nodeName, or a null MObject if none is tracked.
    //! O(1) cache lookup; never falls through to a scene scan.
    static MObject find(const std::string& nodeName);

    //! USD stage for \p nodeName, creating the node if needed.
    static PXR_NS::UsdStageRefPtr getStage(const std::string& nodeName);

    //! Non-creating variant of getStage(): returns nullptr for unregistered or
    //! torn-down names. The stage itself is still materialized lazily on first
    //! call (which runs the populator and fires the stage observer hook).
    static PXR_NS::UsdStageRefPtr getStageForNodeName(const std::string& nodeName);

    //! Live UsdSettingsNode* for \p nodeName, or nullptr if none is tracked.
    //! Does not create or materialize the stage.
    static UsdSettingsNode* getNodeForNodeName(const std::string& nodeName);

    //! Reverse lookup: live MObject of the managed node whose stage matches
    //! \p stage, or a null MObject if none. Does NOT trigger lazy stage
    //! creation (uses UsdSettingsNode::hasStage()).
    static MObject nodeForStage(PXR_NS::UsdStageWeakPtr stage);

    //! Stages of all managed nodes that have already materialized one.
    //! Does NOT trigger lazy creation. Intended for non-perturbing walks.
    static std::vector<PXR_NS::UsdStageRefPtr> getAllLiveStages();

    // -----------------------------------------------------------------------
    // Stage observation hook
    //
    // Lets the UFE layer attach USD-notice listeners to each newly
    // materialized stage without making this layer depend on UFE. Lookup APIs
    // are pure getters and do not invoke the hook.
    // -----------------------------------------------------------------------

    using StageObserverHook = std::function<void(const PXR_NS::UsdStageRefPtr&)>;

    //! Install the hook; pass a default-constructed argument to clear it.
    static void setStageObserverHook(StageObserverHook hook);

    //! Invoke the installed hook (if any) for \p stage. Called by
    //! UsdSettingsNode::ensureStage() and ::deserializeFromAttributes()
    //! immediately after the stage is materialized.
    static void notifyStageObserver(const PXR_NS::UsdStageRefPtr& stage);

    // -----------------------------------------------------------------------
    // Plugin lifecycle (called from MayaUsdProxyShapePlugin)
    // -----------------------------------------------------------------------

    //! Install scene callbacks and create one node per registered name.
    //! Must be called after registerNode() for UsdSettingsNode::typeName.
    static void onPluginInitialize();

    //! Remove scene callbacks and delete all managed nodes.
    //! Must be called before deregisterNode() for UsdSettingsNode::typeName.
    static void onPluginFinalize();

private:
    //! Live MObject for \p nodeName, creating it if absent. Used internally;
    //! external callers go through getStage().
    static MObject findOrCreate(const std::string& nodeName);

    //! Create a UsdSettingsNode whose locked DG name is \p nodeName.
    static MObject createNode(const std::string& nodeName);

    //! Scan the scene and rebuild the instances map.
    static void rebuildInstancesFromScene();

    //! Cast a valid MObjectHandle to a UsdSettingsNode*. nullptr on failure.
    static UsdSettingsNode* nodeFromHandle(const MObjectHandle& handle);

    static void onAfterNew(void* clientData);
    static void onAfterOpen(void* clientData);
    static void onBeforeSave(void* clientData);

    // Function-local statics so registration from other TUs' static
    // initializers is order-independent.
    static std::map<std::string, Populator>&     registry();
    static std::map<std::string, MObjectHandle>& instances();
    static StageObserverHook&                    stageObserverHook();

    static MCallbackId afterNewCbId;
    static MCallbackId afterOpenCbId;
    static MCallbackId beforeSaveCbId;
    static bool        isPluginInitialized;
};

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_USD_SCENE_SETTINGS_MANAGER_H
