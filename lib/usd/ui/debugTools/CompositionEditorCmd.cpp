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

#include <mayaUsd/ufe/Utils.h>

#include <pxr/usd/usd/prim.h>

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
#include <QtGui/QPalette>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <UsdDebugUI/ApplicationHost.h>
#include <UsdDebugUI/CompositionEditorWidget.h>

namespace MAYAUSD_NS_DEF {

const MString CompositionEditorCmd::name("mayaUsdCompositionEditor");

namespace {

constexpr auto kPrimPathFlag = "-pp";
constexpr auto kPrimPathFlagLong = "-primPath";
constexpr auto kReloadFlag = "-rl";
constexpr auto kReloadFlagLong = "-reload";

const MString WORKSPACE_CONTROL_NAME = "mayaUsdCompositionEditor";

QPointer<Adsk::UsdDebug::CompositionEditorWidget> g_compositionEditorWidget;
Ufe::Observer::Ptr                                g_selectionObserver;

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
        g_compositionEditorWidget->setPrim(resolvePrimFromSelection());
    }
};

class MayaCompositionEditorHost : public Adsk::UsdDebug::ApplicationHost
{
public:
    static void ensureInstalled()
    {
        static MayaCompositionEditorHost* s_instance = nullptr;
        if (!s_instance) {
            s_instance = new MayaCompositionEditorHost();
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
        }
        return QColor();
    }

protected:
    MayaCompositionEditorHost() { injectInstance(this); }
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
void buildWidgetIntoCurrentParent(const PXR_NS::UsdPrim& prim)
{
    MayaCompositionEditorHost::ensureInstalled();

    QWidget* mayaParent = MQtUtil::getCurrentParent();

    g_compositionEditorWidget = new Adsk::UsdDebug::CompositionEditorWidget(nullptr);
    if (prim) {
        g_compositionEditorWidget->setPrim(prim);
    }
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
    if (g_selectionObserver) {
        if (auto sel = Ufe::GlobalSelection::get()) {
            sel->removeObserver(g_selectionObserver);
        }
        g_selectionObserver.reset();
    }

    if (workspaceControlExists()) {
        MString closeCmd;
        closeCmd.format("workspaceControl -e -close \"^1s\"", WORKSPACE_CONTROL_NAME);
        MGlobal::executeCommand(closeCmd);
    }
    g_compositionEditorWidget.clear();

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

    const bool isReload = argData.isFlagSet(kReloadFlag);

    if (isReload) {
        // Maya is invoking us through workspaceControl's -uiScript to rebuild
        // the widget inside an already-existing workspace control container.
        buildWidgetIntoCurrentParent(prim);
        return MS::kSuccess;
    }

    if (workspaceControlExists()) {
        // Bring an existing (possibly closed/hidden) workspace control back.
        MString restoreCmd;
        restoreCmd.format("workspaceControl -e -restore \"^1s\"", WORKSPACE_CONTROL_NAME);
        MGlobal::executeCommand(restoreCmd);
        if (g_compositionEditorWidget && prim) {
            g_compositionEditorWidget->setPrim(prim);
        }
        return MS::kSuccess;
    }

    // First invocation: create the workspace control wrapper, then create
    // the widget inside it. -retain false + -deleteLater false combined
    // with -uiScript matches the Layer Editor pattern so the widget is
    // rebuilt on layout save/restore.
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

    buildWidgetIntoCurrentParent(prim);

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
    syntax.addFlag(kReloadFlag, kReloadFlagLong);
    return syntax;
}

} // namespace MAYAUSD_NS_DEF
