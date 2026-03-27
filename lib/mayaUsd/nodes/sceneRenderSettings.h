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
#ifndef MAYAUSD_SCENE_RENDER_SETTINGS_H
#define MAYAUSD_SCENE_RENDER_SETTINGS_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/nodes/proxyStageProvider.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MMessage.h>
#include <maya/MObject.h>
#include <maya/MObjectHandle.h>
#include <maya/MPxLocatorNode.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>

namespace MAYAUSD_NS_DEF {

/*! \brief Singleton DAG node that holds an in-memory USD Stage for scene-level render settings.
 *
 *  \warning WIP – This class is under active development and should not be used in production.
 *           Its API may change without notice.
 *
 *  This node is automatically created on scene new and restored on scene open.
 *  It is hidden in the outliner by default. Its USD stage data is serialized
 *  into the Maya scene file so no external USD files are needed.
 *
 *  The node implements ProxyStageProvider so it works with the existing
 *  MayaUsdAPI::ProxyStage interface.
 */
class MAYAUSD_CORE_PUBLIC UsdSceneRenderSettings
    : public MPxLocatorNode
    , public PXR_NS::ProxyStageProvider
{
public:
    static const MTypeId typeId;
    static const MString typeName;

    // Attributes
    static MObject serializedRootLayerAttr;
    static MObject serializedSessionLayerAttr;
    static MObject outStageCacheIdAttr;

    static void*   creator();
    static MStatus initialize();

    // MPxLocatorNode overrides
    MStatus compute(const MPlug& plug, MDataBlock& dataBlock) override;
    bool    isBounded() const override;

    // ProxyStageProvider interface
    PXR_NS::UsdTimeCode    getTime() const override;
    PXR_NS::UsdStageRefPtr getUsdStage() const override;

    // Singleton management
    static MObject findInstance();
    static MObject findOrCreateInstance();

    //! Get the USD stage from the singleton node, creating it if needed.
    static PXR_NS::UsdStageRefPtr getStage();

    // Scene callback management
    static void installCallbacks();
    static void removeCallbacks();

    //! Return true if the internal stage has already been created.
    //! Unlike getUsdStage(), this does not trigger lazy creation.
    bool hasStage() const { return _stage != nullptr; }

    // Serialization (called from scene callbacks)
    void serializeToAttributes();
    void deserializeFromAttributes();

private:
    UsdSceneRenderSettings();
    ~UsdSceneRenderSettings() override;

    UsdSceneRenderSettings(const UsdSceneRenderSettings&) = delete;
    UsdSceneRenderSettings& operator=(const UsdSceneRenderSettings&) = delete;

    MStatus computeOutStageCacheId(MDataBlock& dataBlock);

    void ensureStage() const;
    void populateDefaultRenderSettings() const;

    mutable PXR_NS::UsdStageRefPtr _stage;

    static MObjectHandle _cachedInstance;
    static MCallbackId   _afterNewCbId;
    static MCallbackId   _afterOpenCbId;
    static MCallbackId   _beforeSaveCbId;
};

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_SCENE_RENDER_SETTINGS_H
