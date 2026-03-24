//
// Copyright 2025 Autodesk
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
// WIP – This file is under active development and should not be used in production.

#include "sceneRenderSettings.h"

#include <mayaUsd/ufe/Utils.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdRender/settings.h>
#include <pxr/usd/usdUtils/stageCache.h>

#include <maya/MDagModifier.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MPlug.h>
#include <maya/MSceneMessage.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
// Re-entrancy guard for callback-driven node creation.
bool _isCreatingInstance = false;
} // namespace

namespace MAYAUSD_NS_DEF {

const MTypeId UsdSceneRenderSettings::typeId(0x580000A6);
const MString UsdSceneRenderSettings::typeName("mayaUsdSceneRenderSettings");

MObject UsdSceneRenderSettings::serializedRootLayerAttr;
MObject UsdSceneRenderSettings::serializedSessionLayerAttr;
MObject UsdSceneRenderSettings::outStageCacheIdAttr;

MObjectHandle UsdSceneRenderSettings::_cachedInstance;
MCallbackId   UsdSceneRenderSettings::_afterNewCbId = 0;
MCallbackId   UsdSceneRenderSettings::_afterOpenCbId = 0;
MCallbackId   UsdSceneRenderSettings::_beforeSaveCbId = 0;

/* static */
void* UsdSceneRenderSettings::creator() { return new UsdSceneRenderSettings(); }

/* static */
MStatus UsdSceneRenderSettings::initialize()
{
    MStatus status;

    MFnTypedAttribute typedAttrFn;
    MFnStringData     stringDataFn;
    const MObject     defaultStringDataObj = stringDataFn.create("");

    // Serialized root layer: storable, hidden, internal.
    serializedRootLayerAttr = typedAttrFn.create(
        "serializedRootLayer", "srl", MFnData::kString, defaultStringDataObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    typedAttrFn.setStorable(true);
    typedAttrFn.setHidden(true);
    typedAttrFn.setInternal(true);
    status = addAttribute(serializedRootLayerAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Serialized session layer: storable, hidden, internal.
    serializedSessionLayerAttr = typedAttrFn.create(
        "serializedSessionLayer", "ssl", MFnData::kString, defaultStringDataObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    typedAttrFn.setStorable(true);
    typedAttrFn.setHidden(true);
    typedAttrFn.setInternal(true);
    status = addAttribute(serializedSessionLayerAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Output stage cache ID: allows downstream consumers (Arnold, Bifrost, ...)
    // to discover the stage via UsdUtilsStageCache.
    MFnNumericAttribute numericAttrFn;
    outStageCacheIdAttr
        = numericAttrFn.create("outStageCacheId", "ostcid", MFnNumericData::kInt, -1, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    numericAttrFn.setStorable(false);
    numericAttrFn.setWritable(false);
    status = addAttribute(outStageCacheIdAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = attributeAffects(serializedRootLayerAttr, outStageCacheIdAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = attributeAffects(serializedSessionLayerAttr, outStageCacheIdAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return status;
}

UsdSceneRenderSettings::UsdSceneRenderSettings()
    : MPxLocatorNode()
{
}

UsdSceneRenderSettings::~UsdSceneRenderSettings()
{
    // Remove our stage from the global cache so nothing holds it alive
    // after the node is destroyed.
    if (_stage) {
        UsdUtilsStageCache::Get().Erase(_stage);
        _stage = nullptr;
    }
}

bool UsdSceneRenderSettings::isBounded() const { return false; }

MStatus UsdSceneRenderSettings::compute(const MPlug& plug, MDataBlock& dataBlock)
{
    if (plug == outStageCacheIdAttr) {
        return computeOutStageCacheId(dataBlock);
    }

    return MS::kUnknownParameter;
}

MStatus UsdSceneRenderSettings::computeOutStageCacheId(MDataBlock& dataBlock)
{
    MStatus status;

    ensureStage();

    if (!_stage) {
        return MS::kFailure;
    }

    int  cacheId = -1;
    auto id = UsdUtilsStageCache::Get().Insert(_stage);
    if (id)
        cacheId = id.ToLongInt();

    MDataHandle outHandle = dataBlock.outputValue(outStageCacheIdAttr, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    outHandle.set(cacheId);
    outHandle.setClean();

    return MS::kSuccess;
}

UsdTimeCode UsdSceneRenderSettings::getTime() const { return UsdTimeCode::Default(); }

UsdStageRefPtr UsdSceneRenderSettings::getUsdStage() const
{
    ensureStage();
    return _stage;
}

/* static */
UsdStageRefPtr UsdSceneRenderSettings::getStage()
{
    MObject obj = findOrCreateInstance();
    if (obj.isNull()) {
        return nullptr;
    }

    MFnDependencyNode depFn(obj);
    auto*             node = dynamic_cast<UsdSceneRenderSettings*>(depFn.userNode());
    if (!node) {
        return nullptr;
    }
    return node->getUsdStage();
}

void UsdSceneRenderSettings::ensureStage() const
{
    if (_stage) {
        return;
    }

    SdfLayerRefPtr rootLayer = SdfLayer::CreateAnonymous("sceneRenderSettingsRoot");
    SdfLayerRefPtr sessionLayer = SdfLayer::CreateAnonymous("sceneRenderSettingsSession");
    _stage = UsdStage::Open(rootLayer, sessionLayer);

    populateDefaultRenderSettings();
}

void UsdSceneRenderSettings::populateDefaultRenderSettings() const
{
    if (!_stage) {
        return;
    }

    // Create a Scope "Render" to hold all render settings,
    // per UsdRender conventions (https://openusd.org/dev/api/usd_render_page_front.html).
    const SdfPath renderScopePath("/Render");
    const SdfPath renderSettingsPath("/Render/SceneRenderSettings");

    UsdGeomScope::Define(_stage, renderScopePath);
    // Define the RenderSettings prim, leave the attribute un-authored,
    // so it falls back to the default USD RenderSettings schema defaults.
    UsdRenderSettings renderSettings = UsdRenderSettings::Define(_stage, renderSettingsPath);
    if (!renderSettings) {
        return;
    }

    // Set stage metadata to point to the default render settings prim.
    _stage->SetMetadata(TfToken("renderSettingsPrimPath"), renderSettingsPath.GetString());
}

void UsdSceneRenderSettings::serializeToAttributes()
{
    if (!_stage) {
        return;
    }

    MFnDependencyNode depFn(thisMObject());

    // Temporarily unlock to allow attribute writes.
    bool wasLocked = depFn.isLocked();
    if (wasLocked) {
        depFn.setLocked(false);
    }

    std::string rootStr;
    _stage->GetRootLayer()->ExportToString(&rootStr);
    MPlug rootPlug(thisMObject(), serializedRootLayerAttr);
    rootPlug.setString(MString(rootStr.c_str()));

    std::string sessionStr;
    _stage->GetSessionLayer()->ExportToString(&sessionStr);
    MPlug sessionPlug(thisMObject(), serializedSessionLayerAttr);
    sessionPlug.setString(MString(sessionStr.c_str()));

    if (wasLocked) {
        depFn.setLocked(true);
    }
}

void UsdSceneRenderSettings::deserializeFromAttributes()
{
    MPlug rootPlug(thisMObject(), serializedRootLayerAttr);
    MPlug sessionPlug(thisMObject(), serializedSessionLayerAttr);

    MString rootStr = rootPlug.asString();
    MString sessionStr = sessionPlug.asString();

    SdfLayerRefPtr rootLayer = SdfLayer::CreateAnonymous("sceneRenderSettingsRoot");
    SdfLayerRefPtr sessionLayer = SdfLayer::CreateAnonymous("sceneRenderSettingsSession");

    if (rootStr.length() > 0) {
        rootLayer->ImportFromString(std::string(rootStr.asChar()));
    }

    if (sessionStr.length() > 0) {
        sessionLayer->ImportFromString(std::string(sessionStr.asChar()));
    }

    _stage = UsdStage::Open(rootLayer, sessionLayer);
}

// ---------------------------------------------------------------------------
// Singleton management
// ---------------------------------------------------------------------------

/* static */
MObject UsdSceneRenderSettings::findInstance()
{
    if (_cachedInstance.isValid()) {
        return _cachedInstance.object();
    }

    MItDependencyNodes it(MFn::kPluginLocatorNode);
    while (!it.isDone()) {
        MObject           obj = it.thisNode();
        MFnDependencyNode depFn(obj);
        if (depFn.typeId() == typeId) {
            _cachedInstance = MObjectHandle(obj);
            return obj;
        }
        it.next();
    }
    return MObject::kNullObj;
}

/* static */
MObject UsdSceneRenderSettings::findOrCreateInstance()
{
    MObject existing = findInstance();
    if (!existing.isNull()) {
        return existing;
    }

    if (_isCreatingInstance) {
        return MObject::kNullObj;
    }
    _isCreatingInstance = true;

    MDagModifier modifier;
    MObject      transformObj = modifier.createNode(typeName);
    modifier.doIt();

    // The createNode for a DAG node creates a transform + shape.
    // Find the shape under the transform.
    MObject    shapeObj = MObject::kNullObj;
    MFnDagNode dagFn(transformObj);
    if (dagFn.typeName() == typeName) {
        // If createNode returned the shape directly.
        shapeObj = transformObj;
    } else {
        // The transform was returned; find the shape child.
        for (unsigned int i = 0; i < dagFn.childCount(); ++i) {
            MObject           child = dagFn.child(i);
            MFnDependencyNode childDepFn(child);
            if (childDepFn.typeId() == typeId) {
                shapeObj = child;
                break;
            }
        }
    }

    if (!shapeObj.isNull()) {
        _cachedInstance = MObjectHandle(shapeObj);

        MFnDependencyNode depFn(shapeObj);
        depFn.setLocked(true);

        // Name and hide the parent transform.
        // Note: the transform is intentionally NOT locked so that generic
        // MEL scripts (e.g. the up-axis export helper that groups all
        // assemblies) can reparent it without errors.
        MFnDagNode shapeDagFn(shapeObj);
        MObject    parentObj = shapeDagFn.parent(0);
        if (!parentObj.isNull()) {
            MFnDependencyNode parentDepFn(parentObj);
            parentDepFn.setName("SceneRenderSettings");
            MPlug hiddenPlug = parentDepFn.findPlug("hiddenInOutliner", true);
            if (!hiddenPlug.isNull()) {
                hiddenPlug.setBool(true);
            }
        }

        // Dirty the UFE stage map so this node's stage is discoverable.
        // Unlike MayaUsdProxyShapeBase, this node is not recognized by
        // UsdStageMap::processNodeAdded(), so we must trigger a rebuild
        // explicitly for the stage map to discover it via
        // discoverSceneRenderSettingsNode() during rebuildIfDirty().
        MayaUsd::ufe::setStageMapDirty();
    }

    _isCreatingInstance = false;
    return shapeObj;
}

// ---------------------------------------------------------------------------
// Scene callbacks
// ---------------------------------------------------------------------------

namespace {

void afterNewCallback(void* /*clientData*/) { UsdSceneRenderSettings::findOrCreateInstance(); }

void afterOpenCallback(void* /*clientData*/)
{
    MObject instance = UsdSceneRenderSettings::findInstance();
    if (instance.isNull()) {
        UsdSceneRenderSettings::findOrCreateInstance();
        return;
    }

    // Deserialize the stage from the saved attributes.
    MFnDependencyNode depFn(instance);
    auto*             node = dynamic_cast<UsdSceneRenderSettings*>(depFn.userNode());
    if (node) {
        node->deserializeFromAttributes();
        // Dirty the stage map so the restored stage is discoverable
        // (see comment in findOrCreateInstance for why this is needed).
        MayaUsd::ufe::setStageMapDirty();
    }
}

void beforeSaveCallback(void* /*clientData*/)
{
    MObject instance = UsdSceneRenderSettings::findInstance();
    if (instance.isNull()) {
        return;
    }

    MFnDependencyNode depFn(instance);
    auto*             node = dynamic_cast<UsdSceneRenderSettings*>(depFn.userNode());
    if (node) {
        node->serializeToAttributes();
    }
}

} // namespace

/* static */
void UsdSceneRenderSettings::installCallbacks()
{
    MStatus status;

    _afterNewCbId
        = MSceneMessage::addCallback(MSceneMessage::kAfterNew, afterNewCallback, nullptr, &status);
    CHECK_MSTATUS(status);

    _afterOpenCbId = MSceneMessage::addCallback(
        MSceneMessage::kAfterSceneReadAndRecordEdits, afterOpenCallback, nullptr, &status);
    CHECK_MSTATUS(status);

    _beforeSaveCbId = MSceneMessage::addCallback(
        MSceneMessage::kBeforeSave, beforeSaveCallback, nullptr, &status);
    CHECK_MSTATUS(status);
}

/* static */
void UsdSceneRenderSettings::removeCallbacks()
{
    if (_afterNewCbId) {
        MSceneMessage::removeCallback(_afterNewCbId);
        _afterNewCbId = 0;
    }
    if (_afterOpenCbId) {
        MSceneMessage::removeCallback(_afterOpenCbId);
        _afterOpenCbId = 0;
    }
    if (_beforeSaveCbId) {
        MSceneMessage::removeCallback(_beforeSaveCbId);
        _beforeSaveCbId = 0;
    }

    // Delete the singleton node so the plugin can be unloaded.
    // The node (and its parent transform) are locked, so unlock first.
    MObject instance = findInstance();
    if (!instance.isNull()) {
        MFnDagNode shapeDagFn(instance);
        MObject    parentObj = shapeDagFn.parent(0);

        MFnDependencyNode depFn(instance);
        depFn.setLocked(false);
        if (!parentObj.isNull()) {
            MFnDependencyNode parentDepFn(parentObj);
            parentDepFn.setLocked(false);
        }

        MDagModifier modifier;
        modifier.deleteNode(instance);
        if (!parentObj.isNull()) {
            modifier.deleteNode(parentObj);
        }
        modifier.doIt();

        _cachedInstance = MObjectHandle();
    }
}

} // namespace MAYAUSD_NS_DEF
