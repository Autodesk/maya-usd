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
#include <mayaUsd/utils/util.h>
#include <mayaUsd/utils/utilComponentCreator.h>
#include <mayaUsd/utils/utilSerialization.h>

#include <pxr/usd/usd/stage.h>

#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MQtUtil.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#ifdef WANT_ADSK_USD_EDIT_FORWARD_BUILD
#include <stringResources.h>

#include <layerLocking.h>

#include <mayaUsd/editForward/MayaUsdEditForwardHost.h>

#include <mayaUsdUI/ui/editForwardDialog.h>

#include <usdUfe/ufe/Utils.h>

#include <AdskUsdEditForward/Host.h>
#include <AdskUsdEditForward/StageRuleProvider.h>

#include <QtCore/QPointer>
#endif

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Local copy of the proxy-shape boolean attribute reader (the original lives in
// an anonymous namespace in mayaCommandHook.cpp and is not reachable here).
std::string proxyShapeName(const std::string& proxyShapePath)
{
    std::size_t found = proxyShapePath.find_last_of("|");
    return (std::string::npos != found) ? proxyShapePath.substr(found + 1) : proxyShapePath;
}

bool getBooleanAttributeOnProxyShape(
    const std::string& proxyShapePath,
    const std::string& attributeName)
{
    if (proxyShapePath.empty())
        return false;

    MObject mobj;
    MStatus status = PXR_NS::UsdMayaUtil::GetMObjectByName(proxyShapeName(proxyShapePath), mobj);
    if (status == MStatus::kSuccess) {
        MFnDependencyNode fn;
        fn.setObject(mobj);
        bool attribute;
        if (PXR_NS::UsdMayaUtil::getPlugValue(fn, attributeName.c_str(), &attribute))
            return attribute;
    }
    return false;
}

} // namespace

namespace UsdLayerEditor {

void registerLayerEditorDCCFunctions()
{
#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)
    ComponentFns component;
    component.saveComponent
        = [](const PXR_NS::UsdStageRefPtr& /*stage*/, const std::string& dccObjectPath) {
              MayaUsd::ComponentUtils::saveAdskUsdComponent(dccObjectPath);
          };
    component.reloadComponent = [](const std::string& dccObjectPath) {
        MayaUsd::ComponentUtils::reloadAdskUsdComponent(dccObjectPath);
    };
    component.renameProxyShape
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
    component.sceneFolder = []() { return MayaUsd::utils::getSceneFolder(); };
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
    component.displayError = [](const std::string& error) {
        MGlobal::displayError(error.c_str());
    };
    component.getComponentLayersToSave = [](const std::string& dccObjectPath) {
        return MayaUsd::ComponentUtils::getAdskUsdComponentLayersToSave(dccObjectPath);
    };
    component.transferSessionLayer
        = [](const std::string& oldDccObjectPath, const std::string& newDccObjectPath) {
              auto oldStage = UsdMayaUtil::GetStageByProxyName(oldDccObjectPath);
              auto newStage = UsdMayaUtil::GetStageByProxyName(newDccObjectPath);
              if (oldStage && newStage)
                  newStage->GetSessionLayer()->TransferContent(oldStage->GetSessionLayer());
          };
    component.setProxyRootLayerPath = [](const std::string&            dccObjectPath,
                                         const std::string&            rootLayerPath,
                                         const PXR_NS::SdfLayerRefPtr& rootLayer) {
        MayaUsd::utils::setNewProxyPath(
            MString(dccObjectPath.c_str()),
            MString(rootLayerPath.c_str()),
            MayaUsd::utils::ProxyPathMode::kProxyPathAbsolute,
            rootLayer,
            /*wasTargetLayer=*/false);
    };
    setComponentFns(component);

    DccObjectFns dccObject;
    dccObject.isDccObjectStageIncoming = [](const std::string& dccObjectPath) {
        return getBooleanAttributeOnProxyShape(dccObjectPath, "stageIncoming");
    };
    dccObject.isDccObjectSharedStage = [](const std::string& dccObjectPath) {
        return getBooleanAttributeOnProxyShape(dccObjectPath, "shareStage");
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
    environment.mainWindowParent = []() -> QWidget* { return MQtUtil::mainWindow(); };
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
    editForwarding.openEditForwardDialog = [](const UsdStageRefPtr& stage) {
        // Reuse a single dialog instance; QPointer auto-nulls if it is destroyed.
        static QPointer<UsdEditForwardConfig::EditForwardDialog> dialog;
        if (!dialog) {
            dialog = new UsdEditForwardConfig::EditForwardDialog(
                StringResources::getAsQString(StringResources::kConfigureEditForwardingTitle),
                MQtUtil::mainWindow());
        }
        dialog->setActiveStage(stage);
        dialog->show();
        dialog->raise();
        dialog->activateWindow();
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
#endif // MAYAUSD_USE_SHARED_LAYER_EDITOR
}

void deregisterLayerEditorDCCFunctions()
{
    setLayerEditorDCCFunctions(LayerEditorDCCFunctions {});
}

} // namespace UsdLayerEditor
