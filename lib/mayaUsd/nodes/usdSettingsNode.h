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
#ifndef MAYAUSD_USD_SETTINGS_NODE_H
#define MAYAUSD_USD_SETTINGS_NODE_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/nodes/proxyStageProvider.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MMessage.h>
#include <maya/MObject.h>
#include <maya/MObjectHandle.h>
#include <maya/MPxNode.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>

#include <string>

namespace MAYAUSD_NS_DEF {

/*! \brief Generic singleton DG node that holds an in-memory USD stage identified by its DG name.
 *
 *  Each instance is identified by its locked Maya DG node name (e.g. "UsdDefaultRenderSettings"),
 *  which UsdSceneSettingsManager uses both as a lookup key and to dispatch the matching
 *  populator that seeds the node's USD stage content. UsdSceneSettingsManager guarantees one
 *  managed instance per registered node name throughout the Maya session.
 *
 *  As a pure DG node it has no DAG transform and does not appear in DAG traversals.
 *  Its USD stage is serialized into the Maya scene file via two hidden string
 *  attributes so no external USD files are needed.
 *
 *  The node implements ProxyStageProvider so MayaUsdAPI::ProxyStage clients can read the
 *  stage. UFE integration goes the other direction: MayaUsd::ufe::Utils::getStage()
 *  dispatches DG-segment paths through UsdSceneSettingsManager::getStageForNodeName().
 *  UsdStageMap itself stays free of any settings-node knowledge.
 */
class MAYAUSD_CORE_PUBLIC UsdSettingsNode
    : public MPxNode
    , public PXR_NS::ProxyStageProvider
{
public:
    static const MTypeId typeId;
    static const MString typeName;

    // Attributes
    static MObject serializedRootLayerAttr;
    static MObject serializedSessionLayerAttr;
    static MObject activeSettingsPathAttr;

    static void*   creator();
    static MStatus initialize();

    // ProxyStageProvider interface
    PXR_NS::UsdTimeCode    getTime() const override;
    PXR_NS::UsdStageRefPtr getUsdStage() const override;

    //! Return this node's DG name. Because the node is locked at creation,
    //! this name is stable for the lifetime of the scene and is the key
    //! UsdSceneSettingsManager uses to look the node up.
    std::string nodeName() const;

    //! Return true if the internal stage has already been created.
    //! Unlike getUsdStage(), this does not trigger lazy creation.
    bool hasStage() const { return _stage != nullptr; }

    //! UFE path string of the currently active settings prim.
    std::string activeSettingsPath() const;

    //! Author the UFE path string of the currently active settings prim. Returns
    //! false on plug-write failure.
    bool setActiveSettingsPath(const std::string& ufePath);

    // Serialization (called by UsdSceneSettingsManager scene callbacks)
    void serializeToAttributes();
    void deserializeFromAttributes();

protected:
    // Destructor is protected so the polymorphic delete path through MPxNode
    // remains valid.
    ~UsdSettingsNode() override = default;

private:
    UsdSettingsNode();

    UsdSettingsNode(const UsdSettingsNode&) = delete;
    UsdSettingsNode& operator=(const UsdSettingsNode&) = delete;

    //! Lazily create the USD stage and run the populator registered for this node name.
    void ensureStage() const;

    mutable PXR_NS::UsdStageRefPtr _stage;
};

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_USD_SETTINGS_NODE_H
