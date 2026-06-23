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
#include "mayaLayerEditorDCCFunctions.h"

#include <layerEditorDCCFunctions.h>
#include <tokens.h> // UsdLayerEditorOptionVars

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/ufe/ProxyShapeHandler.h>
#include <mayaUsd/utils/stageCache.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilComponentCreator.h>
#include <mayaUsd/utils/utilFileSystem.h>
#include <mayaUsd/utils/utilSerialization.h>

#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/tokens.h>

#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>
#include <maya/MDistance.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <ghc/fs_std.hpp>

#ifdef WANT_ADSK_USD_EDIT_FORWARD_BUILD
#include <layerLocking.h>

#include <mayaUsd/editForward/MayaUsdEditForwardHost.h>

#include <usdUfe/ufe/Utils.h>
#endif

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

void registerLayerEditorDCCFunctions()
{
    ComponentFns component;
    component.saveComponent
        = [](const PXR_NS::UsdStageRefPtr& /*stage*/, const std::string& dccObjectPath) {
              MayaUsd::ComponentUtils::saveAdskUsdComponent(dccObjectPath);
          };
    component.reloadComponent = [](const std::string& dccObjectPath) {
        MayaUsd::ComponentUtils::reloadAdskUsdComponent(dccObjectPath);
    };
    component.isStageAComponent = [](const std::string& dccObjectPath) {
        if (dccObjectPath.empty())
            return false;
        return MayaUsd::ComponentUtils::isAdskUsdComponent(dccObjectPath);
    };
    component.isUnsavedComponent = [](const PXR_NS::UsdStageRefPtr& stage) {
        return MayaUsd::ComponentUtils::isUnsavedAdskUsdComponent(stage);
    };
    component.shouldDisplayComponentInitialSaveDialog
        = [](const PXR_NS::UsdStageRefPtr& stage, const std::string& dccObjectPath) {
              return MayaUsd::ComponentUtils::shouldDisplayComponentInitialSaveDialog(
                  stage, dccObjectPath);
          };
    component.moveComponent = [](const std::string& saveLocation,
                                 const std::string& componentName,
                                 const std::string& dccObjectPath) {
        return MayaUsd::ComponentUtils::moveAdskUsdComponent(
            saveLocation, componentName, dccObjectPath);
    };
    component.previewComponentSave = [](const std::string& saveLocation,
                                        const std::string& componentName,
                                        const std::string& dccObjectPath) {
        return MayaUsd::ComponentUtils::previewSaveAdskUsdComponent(
            saveLocation, componentName, dccObjectPath);
    };
    component.getComponentLayersToSave = [](const std::string& dccObjectPath) {
        return MayaUsd::ComponentUtils::getAdskUsdComponentLayersToSave(dccObjectPath);
    };
    setComponentFns(component);

    DccObjectFns dccObject;
    dccObject.isDccObjectStageIncoming = [](const std::string& dccObjectPath) {
        return UsdMayaUtil::GetBooleanAttributeOnProxyShape(dccObjectPath, "stageIncoming");
    };
    dccObject.isDccObjectSharedStage = [](const std::string& dccObjectPath) {
        return UsdMayaUtil::GetBooleanAttributeOnProxyShape(dccObjectPath, "shareStage");
    };
    dccObject.renameObject
        = [](const std::string& oldDccObjectPath, const std::string& newName) -> std::string {
        if (oldDccObjectPath.empty() || newName.empty())
            return {};
        MObject proxyNode;
        if (PXR_NS::UsdMayaUtil::GetMObjectByName(oldDccObjectPath, proxyNode) != MStatus::kSuccess)
            return {};
        MDagModifier dagMod;
        if (dagMod.renameNode(proxyNode, newName.c_str()) != MStatus::kSuccess || dagMod.doIt() != MStatus::kSuccess)
            return {};
        MDagPath newPath;
        if (MDagPath::getAPathTo(proxyNode, newPath) != MStatus::kSuccess)
            return {};
        return newPath.fullPathName().asUTF8();
    };
    dccObject.captureSessionLayer
        = [](const std::string& dccObjectPath) -> PXR_NS::SdfLayerRefPtr {
        auto stage = UsdMayaUtil::GetStageByProxyName(dccObjectPath);
        return stage ? PXR_NS::SdfLayerRefPtr(stage->GetSessionLayer()) : PXR_NS::SdfLayerRefPtr {};
    };
    dccObject.transferSessionLayer
        = [](const PXR_NS::SdfLayerRefPtr& sourceSessionLayer, const std::string& dstDccObjectPath) {
              auto newStage = UsdMayaUtil::GetStageByProxyName(dstDccObjectPath);
              if (sourceSessionLayer && newStage)
                  newStage->GetSessionLayer()->TransferContent(sourceSessionLayer);
          };
    dccObject.setDccObjectRootLayerPath = [](const std::string&            dccObjectPath,
                                             const std::string&            rootLayerPath,
                                             const PXR_NS::SdfLayerRefPtr& rootLayer) {
        MayaUsd::utils::setNewProxyPath(
            MString(dccObjectPath.c_str()),
            MString(rootLayerPath.c_str()),
            MayaUsd::utils::ProxyPathMode::kProxyPathAbsolute,
            rootLayer,
            /*isTargetLayer=*/false);
    };
    setDccObjectFns(dccObject);

    SaveOptionFns saveOption;
    saveOption.requireUsdPathsRelativeToSceneFile = []() {
        return MGlobal::optionVarExists("mayaUsd_MakePathRelativeToSceneFile")
            && MGlobal::optionVarIntValue("mayaUsd_MakePathRelativeToSceneFile") != 0;
    };
    saveOption.requireUsdPathsRelativeToParentLayer = []() {
        return MGlobal::optionVarExists("mayaUsd_MakePathRelativeToParentLayer")
            && MGlobal::optionVarIntValue("mayaUsd_MakePathRelativeToParentLayer") != 0;
    };
    saveOption.requireUsdPathsRelativeToEditTargetLayer = []() {
        return MGlobal::optionVarExists("mayaUsd_MakePathRelativeToEditTargetLayer")
            && MGlobal::optionVarIntValue("mayaUsd_MakePathRelativeToEditTargetLayer") != 0;
    };
    saveOption.wantReferenceCompositionArc = []() {
        return MGlobal::optionVarExists("mayaUsd_WantReferenceCompositionArc")
            && MGlobal::optionVarIntValue("mayaUsd_WantReferenceCompositionArc") != 0;
    };
    saveOption.wantPrependCompositionArc = []() {
        return MGlobal::optionVarExists("mayaUsd_WantPrependCompositionArc")
            && MGlobal::optionVarIntValue("mayaUsd_WantPrependCompositionArc") != 0;
    };
    saveOption.wantPayloadLoaded = []() {
        return MGlobal::optionVarExists("mayaUsd_WantPayloadLoaded")
            && MGlobal::optionVarIntValue("mayaUsd_WantPayloadLoaded") != 0;
    };
    saveOption.getReferencedPrimPath = []() -> std::string {
        if (!MGlobal::optionVarExists("mayaUsd_ReferencedPrimPath"))
            return {};
        return MGlobal::optionVarStringValue("mayaUsd_ReferencedPrimPath").asChar();
    };
    saveOption.setRequireUsdPathsRelativeToSceneFile
        = [](bool v) { MGlobal::setOptionVarValue("mayaUsd_MakePathRelativeToSceneFile", v ? 1 : 0); };
    saveOption.setRequireUsdPathsRelativeToParentLayer
        = [](bool v) { MGlobal::setOptionVarValue("mayaUsd_MakePathRelativeToParentLayer", v ? 1 : 0); };
    saveOption.confirmExistingFileSave = []() {
        static const MString k = UsdLayerEditorOptionVars->ConfirmExistingFileSave.GetText();
        return !MGlobal::optionVarExists(k) || MGlobal::optionVarIntValue(k) != 0; // default true
    };
    saveOption.getSaveLayerFormatBinary = []() {
        static const MString k = UsdLayerEditorOptionVars->SaveLayerFormatArgBinaryOption.GetText();
        return !MGlobal::optionVarExists(k) || MGlobal::optionVarIntValue(k) != 0; // default true
    };
    saveOption.setSaveLayerFormatBinary = [](bool v) {
        static const MString k = UsdLayerEditorOptionVars->SaveLayerFormatArgBinaryOption.GetText();
        MGlobal::setOptionVarValue(k, v ? 1 : 0);
    };
    saveOption.getSerializedUsdEditsLocation = []() -> int {
        static const MString k = UsdLayerEditorOptionVars->SerializedUsdEditsLocation.GetText();
        return MGlobal::optionVarExists(k) ? MGlobal::optionVarIntValue(k) : 1; // kSaveToUSDFiles
    };
    saveOption.setSerializedUsdEditsLocation = [](int v) {
        static const MString k = UsdLayerEditorOptionVars->SerializedUsdEditsLocation.GetText();
        MGlobal::setOptionVarValue(k, v);
    };
    setSaveOptionFns(saveOption);

    EnvironmentFns environment;
    environment.getPinLayerEditorStage = []() {
        static const MString k = UsdLayerEditorOptionVars->PinLayerEditorStage.GetText();
        return MGlobal::optionVarExists(k) && MGlobal::optionVarIntValue(k) != 0;
    };
    environment.setPinLayerEditorStage = [](bool v) {
        static const MString k = UsdLayerEditorOptionVars->PinLayerEditorStage.GetText();
        MGlobal::setOptionVarValue(k, v ? 1 : 0);
    };
    environment.isInteractiveDCCSession
        = []() { return MGlobal::mayaState() == MGlobal::kInteractive; };
    environment.shouldExpandOrCollapseAll = []() {
        int modifiers = 0;
        MGlobal::executeCommand("getModifiers", modifiers);
        return (modifiers % 2) != 0; // magic constant: SHIFT held
    };
    environment.layerContentsArraySizeLimit = []() -> int64_t {
        const MString k
            = PXR_NS::UsdMayaUtil::convert(MayaUsdOptionVars->LayerContentsArraySizeLimit);
        return MGlobal::optionVarExists(k) ? MGlobal::optionVarIntValue(k) : 8;
    };
    environment.layerContentsTimeSamplesSizeLimit = []() -> int64_t {
        const MString k
            = PXR_NS::UsdMayaUtil::convert(MayaUsdOptionVars->LayerContentsTimeSamplesSizeLimit);
        return MGlobal::optionVarExists(k) ? MGlobal::optionVarIntValue(k) : 8;
    };
    environment.displayError = [](const std::string& error) {
        MGlobal::displayError(error.c_str());
    };
    setEnvironmentFns(environment);

#ifdef WANT_ADSK_USD_EDIT_FORWARD_BUILD
    EditForwardingFns editForwarding;
    editForwarding.supportsEditForwarding = []() { return true; };
    editForwarding.echoEditForwarding = []() {
        const MString optVar
            = PXR_NS::UsdMayaUtil::convert(MayaUsdOptionVars->LayerEditorEchoEditForwarding);
        return MGlobal::optionVarExists(optVar) && MGlobal::optionVarIntValue(optVar) != 0;
    };
    editForwarding.setEchoEditForwarding = [](bool echo) {
        const MString optVar
            = PXR_NS::UsdMayaUtil::convert(MayaUsdOptionVars->LayerEditorEchoEditForwarding);
        MGlobal::setOptionVarValue(optVar, echo ? 1 : 0);
        if (auto host = std::dynamic_pointer_cast<MayaUsdEditForwardHost>(
                AdskUsdEditForward::Host::GetInstance())) {
            host->SetWantsEcho(echo);
        }
    };
    editForwarding.handleEFEditTargetUpdate = [](const UsdStageRefPtr& stage) -> bool {
        auto controller = MayaUsdEditForwardController::GetForStage(stage);
        if (!controller || !controller->isForwardingActive())
            return false; // not forwarding: let normal auto-targeting run

        // If edit forwarding is active and the fallback target is now locked, redirect the
        // fallback to the session layer so unmatched edits still have a writable destination.
        // The stage edit target itself is already the session layer in EF mode.
        auto fallback = controller->fallbackTarget();
        if (fallback && UsdLayerEditor::isLayerLocked(fallback)) {
            std::string errMsg;
            if (!UsdUfe::isAnyLayerModifiable(stage, &errMsg)) {
                controller->setFallbackTarget(stage->GetSessionLayer());
            }
        }
        return true; // forwarding active: skip normal auto-targeting
    };
    setEditForwardingFns(editForwarding);
#endif

    FileSystemFns fileSystem;
    fileSystem.getDCCSceneDir
        = []() { return UsdMayaUtilFileSystem::getMayaSceneFileDir(); };
    fileSystem.getDCCWorkspaceScenesDir
        = []() { return std::string(UsdMayaUtil::GetCurrentMayaWorkspacePath().asChar()); };
    fileSystem.sceneFolder = []() { return MayaUsd::utils::getSceneFolder(); };
    fileSystem.prepareLayerSaveUILayer = [](const std::string& relativeAnchor) -> bool {
        const char* script = "import mayaUsd_USDRootFileRelative as murel\n"
                             "murel.usdFileRelative.setRelativeFilePathRoot(r'''%s''')";
        const std::string commandString = PXR_NS::TfStringPrintf(script, relativeAnchor.c_str());
        return MGlobal::executePythonCommand(commandString.c_str());
    };
    fileSystem.checkWriteAccess = [](const std::string& filePath) -> bool {
        const fs::filesystem::path p(filePath);
        if (!fs::filesystem::exists(p))
            return true;
        const auto perms = fs::filesystem::status(p).permissions();
        return (perms & fs::filesystem::perms::owner_write) != fs::filesystem::perms::none;
    };
    setFileSystemFns(fileSystem);

    SerializationFns serialization;
    serialization.getStageCaches = []() {
        std::vector<PXR_NS::UsdStageCache*> caches;
        for (PXR_NS::UsdStageCache& cache : UsdMayaStageCache::GetAllCaches())
            caches.push_back(&cache);
        return caches;
    };
    serialization.getAllStages = []() { return MayaUsd::ufe::ProxyShapeHandler::getAllStages(); };
    serialization.setLayerUpAxisAndUnits = [](const PXR_NS::SdfLayerRefPtr& layer) {
        const PXR_NS::TfToken upAxis
            = MGlobal::isZAxisUp() ? PXR_NS::UsdGeomTokens->z : PXR_NS::UsdGeomTokens->y;
        const double metersPerUnit
            = UsdMayaUtil::ConvertMDistanceUnitToUsdGeomLinearUnit(MDistance::internalUnit());
        layer->SetField(
            PXR_NS::SdfPath::AbsoluteRootPath(),
            PXR_NS::UsdGeomTokens->metersPerUnit,
            metersPerUnit);
        layer->SetField(
            PXR_NS::SdfPath::AbsoluteRootPath(), PXR_NS::UsdGeomTokens->upAxis, upAxis);
    };
    serialization.updateDCCObjectRootLayer
        = [](const std::string&            proxyPath,
             const std::string&            layerPath,
             const PXR_NS::SdfLayerRefPtr& layer,
             bool                          wasTargetLayer) {
              MayaUsd::utils::setNewProxyPath(
                  MString(proxyPath.c_str()),
                  MString(layerPath.c_str()),
                  MayaUsd::utils::kProxyPathFollowProxyShape,
                  layer,
                  wasTargetLayer);
          };
    setSerializationFns(serialization);
}

void deregisterLayerEditorDCCFunctions()
{
    setLayerEditorDCCFunctions(LayerEditorDCCFunctions {});
}

} // namespace UsdLayerEditor
