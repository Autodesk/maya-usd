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
#include "CompositionEditorCmd.h"

#include <mayaUsd/commands/abstractLayerEditorWindow.h>
#include <mayaUsd/listeners/notice.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/undo/MayaUsdUndoBlock.h>
#include <mayaUsd/utils/query.h>
#include <mayaUsdUI/ui/undoChunkUtils.h>

#include <usdUfe/undo/UsdUndoManager.h>
#include <usdUfe/utils/uiCallback.h>

#include <pxr/base/tf/notice.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/tf/weakBase.h>
#include <pxr/base/tf/weakPtr.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/base/vt/types.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

// This is added to prevent multiple definitions of the MApiVersion string.
#define MNoVersionString
#include <maya/MArgParser.h>
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MSyntax.h>
#include <ufe/globalSelection.h>
#include <ufe/observableSelection.h>
#include <ufe/observer.h>
#include <ufe/path.h>
#include <ufe/pathString.h>
#include <ufe/selectionNotification.h>

#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtGui/QPalette>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <UsdDebugUI/ApplicationHost.h>
#include <UsdDebugUI/CompositionEditorWidget.h>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace MAYAUSD_NS_DEF {

const MString CompositionEditorCmd::name("mayaUsdCompositionEditor");

namespace {

constexpr auto kPrimPathFlag = "-pp";
constexpr auto kPrimPathFlagLong = "-primPath";
constexpr auto kLayerFlag = "-lyr";
constexpr auto kLayerFlagLong = "-layer";
constexpr auto kReloadFlag = "-rl";
constexpr auto kReloadFlagLong = "-reload";

const MString WORKSPACE_CONTROL_NAME = "mayaUsdCompositionEditor";

// The USD Layer Editor reports its layer selection through this UI callback.
const PXR_NS::TfToken kLayerEditorSelectionOp("onLayerEditorSelectionChanged");

QPointer<Adsk::UsdDebug::CompositionEditorWidget> g_compositionEditorWidget;
Ufe::Observer::Ptr                                g_selectionObserver;
UsdUfe::UICallback::Ptr                           g_layerSelectionCallback;

// The stage the widget currently renders against.
PXR_NS::UsdStageWeakPtr g_widgetStage;

PXR_NS::UsdPrim resolvePrimFromArg(const MString& primPathStr)
{
    if (primPathStr.length() == 0) {
        return PXR_NS::UsdPrim();
    }
    Ufe::Path ufePath = Ufe::PathString::path(primPathStr.asChar());
    return ufe::ufePathToPrim(ufePath);
}

PXR_NS::UsdPrim resolvePrimFromSelection()
{
    auto selection = Ufe::GlobalSelection::get();
    if (!selection || selection->empty()) {
        return PXR_NS::UsdPrim();
    }
    return ufe::ufePathToPrim(selection->front()->path());
}

// Empty the Layer tab, leaving the widget on its "no layer targeted" placeholder.
void clearLayer()
{
    if (g_compositionEditorWidget) {
        g_compositionEditorWidget->setLayer(PXR_NS::SdfLayerHandle());
    }
}

void setPrim(const PXR_NS::UsdPrim& prim)
{
    if (!g_compositionEditorWidget) {
        return;
    }
    if (!prim || !prim.IsValid()) {
        g_compositionEditorWidget->setPrim(PXR_NS::UsdPrim());
        return;
    }

    // Selecting a prim from a different stage clears the selected layer.
    const bool stageChanged = prim.GetStage() != g_widgetStage;
    if (stageChanged) {
        clearLayer();
    }

    g_compositionEditorWidget->setPrim(prim);
    g_widgetStage = prim.GetStage();
}

// Point the Layer tab at layer. stageHint is the stage the layer was picked from, when
// the caller knows it; otherwise the owning stage is looked up.
void setLayer(
    const PXR_NS::SdfLayerHandle&  layer,
    const PXR_NS::UsdStageWeakPtr& stageHint = PXR_NS::UsdStageWeakPtr())
{
    if (!g_compositionEditorWidget || !layer) {
        return;
    }

    PXR_NS::UsdStageWeakPtr stage = stageHint;
    if (!stage && g_widgetStage && g_widgetStage->HasLocalLayer(layer)) {
        // The same layer can be used by more than one open stage, and getAllStages() is
        // an unordered set, so searching it would pick an arbitrary owner and retarget
        // the widget away from the prim the caller just selected. Stay on the current
        // stage whenever it is a valid owner of the layer.
        stage = g_widgetStage;
    }
    if (!stage) {
        for (const PXR_NS::UsdStageWeakPtr& currentStage : ufe::getAllStages()) {
            if (currentStage && currentStage->HasLocalLayer(layer)) {
                stage = currentStage;
                break;
            }
        }
    }

    if (stage && stage != g_widgetStage) {
        setPrim(stage->GetPseudoRoot());
    }

    g_compositionEditorWidget->setLayer(layer);
}

// Point the Layer tab at the layer named by layerId. Returns false when that
// identifier names no open layer, leaving the tab where it was.
bool setLayerById(
    const std::string&             layerId,
    const PXR_NS::UsdStageWeakPtr& stage = PXR_NS::UsdStageWeakPtr())
{
    PXR_NS::SdfLayerHandle layer = PXR_NS::SdfLayer::Find(layerId);
    if (!layer) {
        return false;
    }

    setLayer(layer, stage);
    return true;
}

class SelectionObserver : public Ufe::Observer
{
public:
    void operator()(const Ufe::Notification& notification) override
    {
        if (!dynamic_cast<const Ufe::SelectionChanged*>(&notification)) {
            return;
        }
        if (!g_compositionEditorWidget) {
            return;
        }
        setPrim(resolvePrimFromSelection());
    }
};

// Retargets the Layer tab from the USD Layer Editor's selection.
class LayerEditorSelectionCallback : public UsdUfe::UICallback
{
public:
    void
    operator()(const PXR_NS::VtDictionary& context, PXR_NS::VtDictionary& callbackData) override
    {
        if (!g_compositionEditorWidget) {
            return;
        }

        const PXR_NS::VtStringArray layerIds = PXR_NS::VtDictionaryGet<PXR_NS::VtStringArray>(
            callbackData, "layerIds", PXR_NS::VtDefault = PXR_NS::VtStringArray());

        // Nothing selected, nothing shown.
        if (layerIds.empty()) {
            clearLayer();
            return;
        }

        // The Layer Editor names the stage it is editing, and that is authoritative:
        // the same layer can be used by more than one open stage.
        const std::string stagePathStr = PXR_NS::VtDictionaryGet<std::string>(
            context, "objectPath", PXR_NS::VtDefault = std::string());

        PXR_NS::UsdStageWeakPtr stage;
        if (!stagePathStr.empty()) {
            stage = ufe::getStage(Ufe::PathString::path(stagePathStr));
        }

        // The tab shows one layer at a time; a multi-selection targets the first.
        setLayerById(layerIds[0], stage);
    }
};

// The Layer Editor reports selection changes only, so an editor that already has a layer
// selected when the Composition Editor opens has to be read out once - otherwise the
// Layer tab would sit on the session layer until the user picks another layer.
void syncLayerFromLayerEditor()
{
    AbstractLayerEditorCreator* creator = AbstractLayerEditorCreator::instance();
    if (!creator) {
        return;
    }

    for (const std::string& panelName : creator->getAllPanelNames()) {
        AbstractLayerEditorWindow* window = creator->getWindow(panelName.c_str());
        if (!window) {
            continue;
        }

        const std::vector<std::string> layerIds = window->getSelectedLayers();
        if (layerIds.empty()) {
            continue;
        }

        PXR_NS::UsdStageWeakPtr stage;
        if (PXR_NS::UsdPrim prim = PXR_NS::UsdMayaQuery::GetPrim(window->proxyShapeName(true))) {
            stage = prim.GetStage();
        }

        // There is one Layer tab to fill, so the first panel with a selection wins.
        if (setLayerById(layerIds[0], stage)) {
            return;
        }
    }
}

class MayaCompositionEditorHost;
MayaCompositionEditorHost* g_compositionEditorHost = nullptr;

// TfWeakBase is a secondary base so the Qt metaobject machinery still sees
// QObject (through ApplicationHost) as the first base, which Qt requires.
class MayaCompositionEditorHost
    : public Adsk::UsdDebug::ApplicationHost
    , public PXR_NS::TfWeakBase
{
public:
    static void ensureInstalled()
    {
        if (!g_compositionEditorHost) {
            g_compositionEditorHost = new MayaCompositionEditorHost();
        }
        // The host outlives a plugin unload, so a previous finalize() may have
        // revoked the listener. Re-arm it, otherwise the lock refresh would
        // silently stop working after an unload/reload cycle.
        g_compositionEditorHost->registerLayerLockListener();
    }

    // Stop feeding the widget lock notifications. Called from the command's
    // finalize() so a plugin unload cannot leave TfNotice holding a callback
    // into code that is about to be unloaded.
    static void stopListening()
    {
        if (g_compositionEditorHost) {
            g_compositionEditorHost->revokeLayerLockListener();
        }
    }

    int pm(const PixelMetric& metric) const override
    {
        switch (metric) {
        case PixelMetric::OuterMargin: return MQtUtil::dpiScale(6);
        case PixelMetric::ContentMargin: return MQtUtil::dpiScale(4);
        }
        return 0;
    }

    QColor themeColor(const ThemeColors& color) const override
    {
        switch (color) {
        case ThemeColors::DisabledForeground:
            return QApplication::palette().color(QPalette::Disabled, QPalette::WindowText);
        default:
            // All other roles use the base host's themed defaults.
            return ApplicationHost::themeColor(color);
        }
    }

    bool executeInCmd(
        const std::string&           editLabel,
        const std::string&           layerId,
        const std::function<bool()>& edit) override
    {
        if (!edit) {
            return false;
        }

        // Ensure the layer being edited has a UsdUndoStateDelegate so the inverse
        // of the edit is recorded
        if (PXR_NS::SdfLayerHandle layer = PXR_NS::SdfLayer::Find(layerId)) {
            UsdUfe::UsdUndoManager::instance().trackLayerStates(layer);
        }

        MayaUsdUI::UndoChunkGuard undoChunk(editLabel);
        MayaUsdUndoBlock          undoBlock;
        return edit();
    }

    float uiScale() const override { return static_cast<float>(MQtUtil::dpiScale(1.0f)); }

    QVariant loadPersistentData(const QString& group, const QString& key) const override
    {
        const MString varName = optionVarName(group, key);
        if (!MGlobal::optionVarExists(varName)) {
            return QVariant();
        }

        return QVariant(QString::fromUtf8(MGlobal::optionVarStringValue(varName).asChar()));
    }

    void
    savePersistentData(const QString& group, const QString& key, const QVariant& value) override
    {
        // Store every value as a string optionVar
        const MString varName = optionVarName(group, key);
        MGlobal::setOptionVarValue(varName, MString(value.toString().toUtf8().constData()));
    }

protected:
    MayaCompositionEditorHost() { injectInstance(this); }

    // Locking a layer authors nothing. Relay Maya's lock notice as the host signal the
    // widget listens on, which makes it re-read the composition.
    void registerLayerLockListener()
    {
        if (_layerLockNoticeKey.IsValid()) {
            return;
        }
        PXR_NS::TfWeakPtr<MayaCompositionEditorHost> me(this);
        _layerLockNoticeKey
            = PXR_NS::TfNotice::Register(me, &MayaCompositionEditorHost::onLayerLockChanged);
    }

    void onLayerLockChanged(const PXR_NS::UsdMayaLayerLockChangedNotice&)
    {
        Q_EMIT layerLockStateChanged();
    }

    void revokeLayerLockListener()
    {
        if (_layerLockNoticeKey.IsValid()) {
            PXR_NS::TfNotice::Revoke(_layerLockNoticeKey);
        }
    }

    static MString optionVarName(const QString& group, const QString& key)
    {
        const QString name = QStringLiteral("mayaUsd_CompositionEditor_%1_%2").arg(group, key);
        return MString(name.toUtf8().constData());
    }

private:
    PXR_NS::TfNotice::Key _layerLockNoticeKey;
};

bool workspaceControlExists()
{
    MString cmd;
    cmd.format("workspaceControl -exists \"^1s\"", WORKSPACE_CONTROL_NAME);
    int result = 0;
    MGlobal::executeCommand(cmd, result);
    return result != 0;
}

// Instantiate the widget into Maya's current parent (the workspace control's
// QWidget when invoked through workspaceControl's -uiScript callback).
void buildWidgetIntoCurrentParent(const PXR_NS::UsdPrim& prim, const PXR_NS::SdfLayerHandle& layer)
{
    MayaCompositionEditorHost::ensureInstalled();

    QWidget* mayaParent = MQtUtil::getCurrentParent();

    // A brand new widget has no stage of its own yet.
    g_widgetStage = PXR_NS::UsdStageWeakPtr();
    g_compositionEditorWidget = new Adsk::UsdDebug::CompositionEditorWidget(nullptr);
    setPrim(prim);
    MQtUtil::addWidgetToMayaLayout(g_compositionEditorWidget.data(), mayaParent);

    // Mirror the global UFE selection into the widget so the user does not
    // have to re-invoke the command after each selection change. Observer
    // is registered once and reused across widget rebuilds (close+reopen).
    if (!g_selectionObserver) {
        if (auto sel = Ufe::GlobalSelection::get()) {
            g_selectionObserver = std::make_shared<SelectionObserver>();
            sel->addObserver(g_selectionObserver);
        }
    }

    // Same for the Layer Editor's layer selection, which drives the Layer tab.
    if (!g_layerSelectionCallback) {
        g_layerSelectionCallback = std::make_shared<LayerEditorSelectionCallback>();
        UsdUfe::registerUICallback(kLayerEditorSelectionOp, g_layerSelectionCallback);
    }

    // An explicit -layer wins over whatever the Layer Editor has selected.
    if (layer) {
        setLayer(layer);
    } else {
        syncLayerFromLayerEditor();
    }
}

} // namespace

/*static*/
MStatus CompositionEditorCmd::initialize(MFnPlugin& plugin)
{
    return plugin.registerCommand(
        name, CompositionEditorCmd::creator, CompositionEditorCmd::createSyntax);
}

/*static*/
MStatus CompositionEditorCmd::finalize(MFnPlugin& plugin)
{
    MayaCompositionEditorHost::stopListening();

    if (g_selectionObserver) {
        if (auto sel = Ufe::GlobalSelection::get()) {
            sel->removeObserver(g_selectionObserver);
        }
        g_selectionObserver.reset();
    }

    if (g_layerSelectionCallback) {
        UsdUfe::unregisterUICallback(kLayerEditorSelectionOp, g_layerSelectionCallback);
        g_layerSelectionCallback.reset();
    }

    if (workspaceControlExists()) {
        MString closeCmd;
        closeCmd.format("workspaceControl -e -close \"^1s\"", WORKSPACE_CONTROL_NAME);
        MGlobal::executeCommand(closeCmd);
    }
    g_compositionEditorWidget.clear();
    g_widgetStage = PXR_NS::UsdStageWeakPtr();

    return plugin.deregisterCommand(name);
}

void* CompositionEditorCmd::creator() { return new CompositionEditorCmd(); }

MStatus CompositionEditorCmd::doIt(const MArgList& args)
{
    MStatus    st;
    MArgParser argData(syntax(), args, &st);
    if (!st) {
        return MS::kInvalidParameter;
    }

    MString primPathStr;
    if (argData.isFlagSet(kPrimPathFlag)) {
        argData.getFlagArgument(kPrimPathFlag, 0, primPathStr);
    }
    PXR_NS::UsdPrim prim = resolvePrimFromArg(primPathStr);
    if (!prim) {
        prim = resolvePrimFromSelection();
    }

    MString layerIdStr;
    if (argData.isFlagSet(kLayerFlag)) {
        argData.getFlagArgument(kLayerFlag, 0, layerIdStr);
    }
    PXR_NS::SdfLayerHandle layer;
    if (layerIdStr.length() > 0) {
        layer = PXR_NS::SdfLayer::Find(layerIdStr.asChar());
        if (!layer) {
            MString msg;
            msg.format("mayaUsdCompositionEditor: no layer found for identifier ^1s", layerIdStr);
            MGlobal::displayWarning(msg);
        }
    }

    const bool isReload = argData.isFlagSet(kReloadFlag);

    if (isReload) {
        // Maya is invoking us through workspaceControl's -uiScript to rebuild
        // the widget inside an already-existing workspace control container.
        buildWidgetIntoCurrentParent(prim, layer);
        return MS::kSuccess;
    }

    if (workspaceControlExists()) {
        // Bring an existing (possibly closed/hidden) workspace control back.
        MString restoreCmd;
        restoreCmd.format("workspaceControl -e -restore \"^1s\"", WORKSPACE_CONTROL_NAME);
        MGlobal::executeCommand(restoreCmd);
        setPrim(prim);
        setLayer(layer);
        return MS::kSuccess;
    }

    // First invocation: create the workspace control wrapper, then create
    // the widget inside it. -retain false + -deleteLater false combined
    // with -uiScript matches the Layer Editor pattern so the widget is
    // rebuilt on layout save/restore. The workspace control persists its own
    // size across sessions, so -initialWidth/-initialHeight only seed the
    // first-launch geometry.
    MString createCmd;
    createCmd.format(
        "workspaceControl"
        " -label \"USD Composition Editor\""
        " -retain false"
        " -deleteLater false"
        " -loadImmediately true"
        " -floating true"
        " -initialWidth 700"
        " -initialHeight 600"
        " -requiredPlugin \"mayaUsdPlugin\""
        " \"^1s\"",
        WORKSPACE_CONTROL_NAME);
    MGlobal::executeCommand(createCmd);

    buildWidgetIntoCurrentParent(prim, layer);

    // Install the -uiScript only after the initial build, so it doesn't
    // run twice on creation. Mirrors the Layer Editor pattern.
    MString uiScriptCmd;
    uiScriptCmd.format(
        "workspaceControl -e -uiScript \"^1s -reload\" \"^2s\"",
        CompositionEditorCmd::name,
        WORKSPACE_CONTROL_NAME);
    MGlobal::executeCommand(uiScriptCmd);

    return MS::kSuccess;
}

MSyntax CompositionEditorCmd::createSyntax()
{
    MSyntax syntax;
    syntax.enableQuery(false);
    syntax.enableEdit(false);
    syntax.addFlag(kPrimPathFlag, kPrimPathFlagLong, MSyntax::kString);
    syntax.addFlag(kLayerFlag, kLayerFlagLong, MSyntax::kString);
    syntax.addFlag(kReloadFlag, kReloadFlagLong);
    return syntax;
}

} // namespace MAYAUSD_NS_DEF
