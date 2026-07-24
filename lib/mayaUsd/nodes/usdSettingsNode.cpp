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

#include "usdSettingsNode.h"

#include "usdSceneSettingsManager.h"

#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MFnData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MPlug.h>

namespace MAYAUSD_NS_DEF {

const MTypeId UsdSettingsNode::typeId(0x580000A6);
const MString UsdSettingsNode::typeName("UsdDefaultSettings");

MObject UsdSettingsNode::serializedRootLayerAttr;
MObject UsdSettingsNode::serializedSessionLayerAttr;
MObject UsdSettingsNode::activeSettingsPathAttr;

/* static */
void* UsdSettingsNode::creator() { return new UsdSettingsNode(); }

/* static */
MStatus UsdSettingsNode::initialize()
{
    MStatus status;

    MFnTypedAttribute typedAttrFn;
    MFnStringData     stringDataFn;
    const MObject     defaultStringDataObj = stringDataFn.create("");

    // Serialized root layer text. Stored on the node so the in-memory USD stage
    // round-trips through the Maya scene file with no external .usd asset; hidden
    // and internal because it is an implementation detail (users edit the stage,
    // not this string).
    serializedRootLayerAttr = typedAttrFn.create(
        "serializedRootLayer", "srl", MFnData::kString, defaultStringDataObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    typedAttrFn.setStorable(true);
    typedAttrFn.setHidden(true);
    typedAttrFn.setInternal(true);
    status = addAttribute(serializedRootLayerAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Serialized session layer text; mirrors serializedRootLayer above so transient
    // session-layer edits also persist with the Maya scene file.
    serializedSessionLayerAttr = typedAttrFn.create(
        "serializedSessionLayer", "ssl", MFnData::kString, defaultStringDataObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    typedAttrFn.setStorable(true);
    typedAttrFn.setHidden(true);
    typedAttrFn.setInternal(true);
    status = addAttribute(serializedSessionLayerAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // UFE path of the currently active settings prim, persisted with the
    // scene. Hidden/internal: reads/writes go through the typed accessors.
    activeSettingsPathAttr = typedAttrFn.create(
        "activeSettingsPath", "asp", MFnData::kString, defaultStringDataObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    typedAttrFn.setStorable(true);
    typedAttrFn.setHidden(true);
    typedAttrFn.setInternal(true);
    status = addAttribute(activeSettingsPathAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return status;
}

UsdSettingsNode::UsdSettingsNode()
    : MPxNode()
{
}

PXR_NS::UsdTimeCode UsdSettingsNode::getTime() const { return PXR_NS::UsdTimeCode::Default(); }

PXR_NS::UsdStageRefPtr UsdSettingsNode::getUsdStage() const
{
    ensureStage();
    return _stage;
}

std::string UsdSettingsNode::nodeName() const
{
    MFnDependencyNode depFn(thisMObject());
    return depFn.name().asChar();
}

std::string UsdSettingsNode::activeSettingsPath() const
{
    MPlug plug(thisMObject(), activeSettingsPathAttr);
    return plug.asString().asChar();
}

bool UsdSettingsNode::setActiveSettingsPath(const std::string& ufePath)
{
    MPlug   plug(thisMObject(), activeSettingsPathAttr);
    MStatus status = plug.setString(MString(ufePath.c_str()));
    return status == MS::kSuccess;
}

void UsdSettingsNode::ensureStage() const
{
    if (_stage) {
        return;
    }

    const std::string      name = nodeName();
    PXR_NS::SdfLayerRefPtr rootLayer = PXR_NS::SdfLayer::CreateAnonymous(name + "Root");
    PXR_NS::SdfLayerRefPtr sessionLayer = PXR_NS::SdfLayer::CreateAnonymous(name + "Session");
    if (!rootLayer || !sessionLayer) {
        PXR_NAMESPACE_USING_DIRECTIVE
        TF_RUNTIME_ERROR(
            "UsdSettingsNode::ensureStage: SdfLayer::CreateAnonymous returned null for '%s'.",
            name.c_str());
        return;
    }
    _stage = PXR_NS::UsdStage::Open(rootLayer, sessionLayer);
    if (!_stage) {
        PXR_NAMESPACE_USING_DIRECTIVE
        TF_RUNTIME_ERROR(
            "UsdSettingsNode::ensureStage: UsdStage::Open returned null for '%s'.", name.c_str());
        return;
    }

    UsdSceneSettingsManager::callPopulator(name, _stage, const_cast<UsdSettingsNode&>(*this));

    // Hook USD-notice observers (UFE notification flow) onto the freshly
    // created stage. Done after the populator so the initial population is
    // observed; idempotent if the hook is missing or the stage is already
    // observed.
    UsdSceneSettingsManager::notifyStageObserver(_stage);
}

void UsdSettingsNode::serializeToAttributes()
{
    // Force the populator to run before persisting. Without this, a New ->
    // Save sequence in which no caller ever reached getUsdStage() would
    // serialize empty default-string plugs, and the next File > Open would
    // restore an empty stage missing all populator-authored content.
    if (!_stage) {
        getUsdStage();
    }
    if (!_stage) {
        return;
    }

    // Export each layer and only overwrite the matching plug on success.
    // ExportToString returns false on USD's side when serialization fails;
    // overwriting the plug with the partially-populated string would silently
    // corrupt the next File > Open. The previous valid serialization is
    // preserved in that case so the file remains loadable.
    const std::string nameForLog = nodeName();

    std::string rootStr;
    if (_stage->GetRootLayer()->ExportToString(&rootStr)) {
        MPlug   rootPlug(thisMObject(), serializedRootLayerAttr);
        MStatus rootStatus = rootPlug.setString(MString(rootStr.c_str()));
        if (rootStatus != MS::kSuccess) {
            PXR_NAMESPACE_USING_DIRECTIVE
            TF_RUNTIME_ERROR(
                "UsdSettingsNode::serializeToAttributes: setString on the root layer plug failed "
                "for '%s'.",
                nameForLog.c_str());
        }
    } else {
        PXR_NAMESPACE_USING_DIRECTIVE
        TF_RUNTIME_ERROR(
            "UsdSettingsNode::serializeToAttributes: ExportToString on the root layer failed for "
            "'%s'; keeping previous serialized value.",
            nameForLog.c_str());
    }

    std::string sessionStr;
    if (_stage->GetSessionLayer()->ExportToString(&sessionStr)) {
        MPlug   sessionPlug(thisMObject(), serializedSessionLayerAttr);
        MStatus sessionStatus = sessionPlug.setString(MString(sessionStr.c_str()));
        if (sessionStatus != MS::kSuccess) {
            PXR_NAMESPACE_USING_DIRECTIVE
            TF_RUNTIME_ERROR(
                "UsdSettingsNode::serializeToAttributes: setString on the session layer plug "
                "failed for '%s'.",
                nameForLog.c_str());
        }
    } else {
        PXR_NAMESPACE_USING_DIRECTIVE
        TF_RUNTIME_ERROR(
            "UsdSettingsNode::serializeToAttributes: ExportToString on the session layer failed "
            "for '%s'; keeping previous serialized value.",
            nameForLog.c_str());
    }
}

void UsdSettingsNode::deserializeFromAttributes()
{
    const std::string name = nodeName();

    MPlug rootPlug(thisMObject(), serializedRootLayerAttr);
    MPlug sessionPlug(thisMObject(), serializedSessionLayerAttr);

    MString rootStr = rootPlug.asString();
    MString sessionStr = sessionPlug.asString();

    PXR_NS::SdfLayerRefPtr rootLayer = PXR_NS::SdfLayer::CreateAnonymous(name + "Root");
    PXR_NS::SdfLayerRefPtr sessionLayer = PXR_NS::SdfLayer::CreateAnonymous(name + "Session");
    if (!rootLayer || !sessionLayer) {
        PXR_NAMESPACE_USING_DIRECTIVE
        TF_RUNTIME_ERROR(
            "UsdSettingsNode::deserializeFromAttributes: failed to create anonymous layers for "
            "'%s'.",
            name.c_str());
        return;
    }

    if (rootStr.length() > 0 && !rootLayer->ImportFromString(std::string(rootStr.asChar()))) {
        PXR_NAMESPACE_USING_DIRECTIVE
        TF_WARN(
            "UsdSettingsNode::deserializeFromAttributes: ImportFromString failed on the root layer "
            "of '%s'; falling back to defaults.",
            name.c_str());
        rootStr = MString();
    }

    if (sessionStr.length() > 0
        && !sessionLayer->ImportFromString(std::string(sessionStr.asChar()))) {
        PXR_NAMESPACE_USING_DIRECTIVE
        TF_WARN(
            "UsdSettingsNode::deserializeFromAttributes: ImportFromString failed on the session "
            "layer of '%s'.",
            name.c_str());
        sessionStr = MString();
    }

    _stage = PXR_NS::UsdStage::Open(rootLayer, sessionLayer);
    if (!_stage) {
        PXR_NAMESPACE_USING_DIRECTIVE
        TF_RUNTIME_ERROR(
            "UsdSettingsNode::deserializeFromAttributes: UsdStage::Open returned null for '%s'.",
            name.c_str());
        return;
    }

    // If neither the root nor the session layer carried persisted content,
    // we are loading a scene file that was saved before this node ever
    // materialized its stage (so onBeforeSave's serializeToAttributes had
    // empty layers to write). Run the populator so the resulting stage
    // matches what a freshly-created node would produce.
    if (rootStr.length() == 0 && sessionStr.length() == 0) {
        UsdSceneSettingsManager::callPopulator(name, _stage, *this);
    }

    // Newly deserialized stage: re-hook USD-notice observers via the manager
    // (see ensureStage() for rationale).
    UsdSceneSettingsManager::notifyStageObserver(_stage);
}

} // namespace MAYAUSD_NS_DEF
