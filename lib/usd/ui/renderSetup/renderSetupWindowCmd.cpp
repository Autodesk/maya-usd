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
#include "renderSetupWindowCmd.h"

#include "mayaEditCommitter.h"

#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/mayaNodeTypeObserver.h>

// This is added to prevent multiple definitions of the MApiVersion string.
#define MNoVersionString
#include <maya/MArgParser.h>
#include <maya/MFn.h>
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MMessage.h>
#include <maya/MNodeMessage.h>
#include <maya/MQtUtil.h>
#include <maya/MSceneMessage.h>
#include <maya/MSyntax.h>

#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtGui/QAction>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QVBoxLayout>
#include <RenderSetup/RenderSetupWidget.h>

#include <algorithm>
#include <vector>

namespace MAYAUSD_NS_DEF {

class RenderSetupWindow;

const MString     RenderSetupWindowCmd::commandName("mayaUsdRenderSetupWindow");
const std::string kUSDRenderSettingsNodeName("UsdDefaultRenderSettings");

namespace {
constexpr auto kReloadFlag = "-rl";
constexpr auto kReloadFlagLong = "-reload";

const MString WINDOW_TITLE_NAME = "USD Render Setup";
const MString WORKSPACE_CONTROL_NAME = "mayaUsdRenderSetup";

// Global pointer to the (singleton) render setup window.
QPointer<RenderSetupWindow> g_renderSetupWindow;

bool workspaceControlExists()
{
    MString cmd;
    cmd.format("workspaceControl -exists \"^1s\"", WORKSPACE_CONTROL_NAME);
    int result = 0;
    MGlobal::executeCommand(cmd, result);
    return result != 0;
}
} // namespace

class RenderSetupWindow
    : public QMainWindow
    , private MayaUsd::MayaNodeTypeObserver::Listener
{
public:
    typedef QMainWindow PARENT_CLASS;

    explicit RenderSetupWindow(QWidget* parent);
    ~RenderSetupWindow() override;

    void refreshStages();

    void        processNodeAdded(MObject& node) override;
    void        processNodeRemoved(MObject& node) override;
    static void nodeRenamedCB(MObject& node, const MString& oldName, void* clientData);
    static void onSceneChangedCB(void* clientData);

private:
    void applyStages()
    {
        _editCommitter->setStages(_hostStages);
        _tree->setStages(_hostStages);
    }

private:
    Adsk::RenderSetupWidget*               _tree;
    MayaUsdRenderSetup::MayaEditCommitter* _editCommitter { nullptr };
    std::vector<Adsk::HostStage>           _hostStages;
    std::vector<MCallbackId>               _sceneCallbackIds;
};

RenderSetupWindow::RenderSetupWindow(QWidget* parent)
    : PARENT_CLASS(parent)
{
    // Create the render setup widget and set it as the central widget of the window.
    _tree = new Adsk::RenderSetupWidget(this);
    _editCommitter = new MayaUsdRenderSetup::MayaEditCommitter(nullptr);
    _tree->setEditCommitter(std::unique_ptr<Adsk::IEditCommitter>(_editCommitter));
    setCentralWidget(_tree);
    _tree->show();

    auto* viewMenu = menuBar()->addMenu(tr("View"));
    auto* hierarchyAction = viewMenu->addAction(tr("Display USD Hierarchy"));
    hierarchyAction->setCheckable(true);
    hierarchyAction->setChecked(false);
    connect(hierarchyAction, &QAction::toggled, [this](bool checked) {
        _tree->setLayoutMode(
            checked ? Adsk::RenderTreeModel::LayoutMode::Hierarchy
                    : Adsk::RenderTreeModel::LayoutMode::Flat);
    });

    layout()->setContentsMargins(0, 0, 0, 0);
    layout()->setSpacing(0);

    // Observe proxy shape node additions and removals directly.
    PXR_NS::MayaUsdProxyShapeBase::getProxyShapesObserver().addTypeListener(*this);

    // Refresh after scene open or new scene.
    _sceneCallbackIds.push_back(
        MSceneMessage::addCallback(MSceneMessage::kAfterOpen, onSceneChangedCB, this));
    _sceneCallbackIds.push_back(
        MSceneMessage::addCallback(MSceneMessage::kAfterNew, onSceneChangedCB, this));
    _sceneCallbackIds.push_back(
        MNodeMessage::addNameChangedCallback(MObject::kNullObj, nodeRenamedCB, this));
}

RenderSetupWindow::~RenderSetupWindow()
{
    PXR_NS::MayaUsdProxyShapeBase::getProxyShapesObserver().removeTypeListener(*this);

    for (auto id : _sceneCallbackIds) {
        MMessage::removeCallback(id);
    }
}

void RenderSetupWindow::processNodeAdded(MObject& /*node*/)
{
    QTimer::singleShot(0, this, &RenderSetupWindow::refreshStages);
}
void RenderSetupWindow::processNodeRemoved(MObject& /*node*/)
{
    QTimer::singleShot(0, this, &RenderSetupWindow::refreshStages);
}

/* static */
void RenderSetupWindow::nodeRenamedCB(MObject& obj, const MString& oldName, void* clientData)
{
    if (oldName.length() != 0 && obj.hasFn(MFn::kShape)) {
        auto* self = static_cast<RenderSetupWindow*>(clientData);

        // We only need to refresh if the oldName was one we have in our list.
        for (const auto& stage : self->_hostStages) {
            if (stage.displayName == oldName.asChar()) {
                QTimer::singleShot(0, self, &RenderSetupWindow::refreshStages);
                break;
            }
        }
    }
}

void RenderSetupWindow::onSceneChangedCB(void* clientData)
{
    auto self = reinterpret_cast<RenderSetupWindow*>(clientData);
    QTimer::singleShot(0, self, &RenderSetupWindow::refreshStages);
}

void RenderSetupWindow::refreshStages()
{
    _hostStages.clear();

    // Add all the USD stages and sort them alphabetically by display name.
    for (const auto& stage : MayaUsd::ufe::getAllStages()) {
        Adsk::HostStage hostStage;
        hostStage.stage = stage;
        hostStage.displayName = MayaUsd::ufe::stagePath(stage).back().string();
        _hostStages.push_back(hostStage);
    }
    std::sort(_hostStages.begin(), _hostStages.end(), [](const auto& a, const auto& b) {
        return a.displayName < b.displayName;
    });

#ifdef MAYA_HAS_USD_SETTINGS_NODES
    // Add default setting stage (from DG node) but put it first in the vector.
    auto defaultStage = MayaUsd::UsdSceneSettingsManager::getStage(kUSDRenderSettingsNodeName);
    if (defaultStage) {
        Adsk::HostStage hostStage;
        hostStage.stage = defaultStage;
        hostStage.displayName = kUSDRenderSettingsNodeName;
        _hostStages.insert(_hostStages.begin(), hostStage);
    }
#endif

    applyStages();
}

/*static*/
MStatus RenderSetupWindowCmd::initialize(MFnPlugin& plugin)
{
    return plugin.registerCommand(
        commandName, RenderSetupWindowCmd::creator, RenderSetupWindowCmd::createSyntax);
}

/*static*/
MStatus RenderSetupWindowCmd::finalize(MFnPlugin& plugin)
{
    if (workspaceControlExists()) {
        MString closeCmd;
        closeCmd.format("workspaceControl -e -close \"^1s\"", WORKSPACE_CONTROL_NAME);
        MGlobal::executeCommand(closeCmd);
    }
    g_renderSetupWindow.clear();

    return plugin.deregisterCommand(commandName);
}

void* RenderSetupWindowCmd::creator() { return new RenderSetupWindowCmd(); }

void RenderSetupWindowCmd::createWindowIntoCurrentParent()
{
    QWidget* mayaParent = MQtUtil::getCurrentParent();

    if (!g_renderSetupWindow) {
        g_renderSetupWindow = new RenderSetupWindow(nullptr);
    }
    g_renderSetupWindow->refreshStages();
    MQtUtil::addWidgetToMayaLayout(g_renderSetupWindow.data(), mayaParent);
}

MStatus RenderSetupWindowCmd::doIt(const MArgList& argList)
{
    MStatus    st;
    MArgParser argParser(syntax(), argList, &st);
    if (!st)
        return st;

    const bool isReload = argParser.isFlagSet(kReloadFlag);

    if (isReload) {
        // Maya is invoking us through workspaceControl's -uiScript to rebuild
        // the widget inside an already-existing workspace control container.
        createWindowIntoCurrentParent();
        return MS::kSuccess;
    }

    if (workspaceControlExists()) {
        // Bring an existing (possibly closed/hidden) workspace control back.
        MString restoreCmd;
        restoreCmd.format("workspaceControl -e -restore \"^1s\"", WORKSPACE_CONTROL_NAME);
        MGlobal::executeCommand(restoreCmd);
        if (g_renderSetupWindow) {
            g_renderSetupWindow->refreshStages();
        } else {
            createWindowIntoCurrentParent();
        }
        return MS::kSuccess;
    }

    // First invocation: create the workspace control wrapper, then create
    // the widget inside it. -retain false + -deleteLater false combined
    // with -uiScript matches the Layer Editor pattern so the widget is
    // rebuilt on layout save/restore.
    MString      createCmd;
    MStringArray cmdArgs;
    cmdArgs.append(WINDOW_TITLE_NAME);
    cmdArgs.append(std::to_string(MQtUtil::dpiScale(720)).c_str());
    cmdArgs.append(std::to_string(MQtUtil::dpiScale(480)).c_str());
    cmdArgs.append("mayaUsdPlugin");
    cmdArgs.append(WORKSPACE_CONTROL_NAME);
    createCmd.format(
        "workspaceControl"
        " -label \"^1s\""
        " -retain false"
        " -deleteLater false"
        " -loadImmediately true"
        " -floating true"
        " -initialWidth ^2s"
        " -initialHeight ^3s"
        " -requiredPlugin \"^4s\""
        " \"^5s\"",
        cmdArgs);
    MGlobal::executeCommand(createCmd);

    createWindowIntoCurrentParent();

    // Install the -uiScript only after the initial build, so it doesn't
    // run twice on creation. Mirrors the Layer Editor pattern.
    MString uiScriptCmd;
    uiScriptCmd.format(
        "workspaceControl -e -uiScript \"^1s -reload\" \"^2s\"",
        RenderSetupWindowCmd::commandName,
        WORKSPACE_CONTROL_NAME);
    MGlobal::executeCommand(uiScriptCmd);

    return MS::kSuccess;
}

MSyntax RenderSetupWindowCmd::createSyntax()
{
    MSyntax syntax;
    syntax.enableQuery(false);
    syntax.enableEdit(false);
    syntax.addFlag(kReloadFlag, kReloadFlagLong);
    return syntax;
}

} // namespace MAYAUSD_NS_DEF
