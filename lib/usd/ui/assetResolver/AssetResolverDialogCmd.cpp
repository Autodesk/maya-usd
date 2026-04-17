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
#include "AssetResolverDialogCmd.h"

#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/usd/variantSets.h>

#include <maya/MArgParser.h>
#include <maya/MDagPath.h>
#include <maya/MFileObject.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnStringData.h>
#include <maya/MGlobal.h>
#include <maya/MQtUtil.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MSyntax.h>

// This is added to prevent multiple definitions of the MApiVersion string.
#define MNoVersionString
#include <mayaUsd/fileio/importData.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/UsdStageMap.h>
#include <mayaUsd/utils/util.h>
#include <mayaUsdUI/ui/PreferencesManagement.h>
#include <mayaUsdUI/ui/USDQtUtil.h>

#include <maya/MFnPlugin.h>

#include <AssetResolverWidgets/PathDialog/PathDialog.h>
#include <QtCore/QPointer>
#include <QtGui/QCursor>
#include <QtWidgets/QApplication>

namespace MAYAUSD_NS_DEF {

const MString AssetResolverDialogCmd::name("assetResolverDialog");

namespace {

QPointer<Adsk::AssetResolverPathDialog> g_assetResolverDialog;

constexpr auto kParentWindowFlag = "-pw";
constexpr auto kParentWindowFlagLong = "-parentWindow";
constexpr auto kTabFlag = "-tab";
constexpr auto kTabFlagLong = "-tabName";
constexpr auto kPathsTabName = "paths";
constexpr auto kSettingsTabName = "globalSettings";

MString parseTextArg(const MArgParser& argData, const char* flag, const MString& defaultValue)
{
    MString value = defaultValue;
    if (argData.isFlagSet(flag))
        argData.getFlagArgument(flag, 0, value);
    return value;
}

QWidget* findParentWindow(const MString& controlName)
{
    QWidget* originalWidget = MQtUtil::findControl(controlName);
    for (QWidget* widget = originalWidget; widget; widget = widget->parentWidget()) {
        if (widget->isWindow()) {
            return widget;
        }
    }
    return MQtUtil::mainWindow();
}

} // namespace

/*static*/
MStatus AssetResolverDialogCmd::initialize(MFnPlugin& plugin)
{
    return plugin.registerCommand(
        name, AssetResolverDialogCmd::creator, AssetResolverDialogCmd::createSyntax);
}

/*static*/
MStatus AssetResolverDialogCmd::finalize(MFnPlugin& plugin)
{
    return plugin.deregisterCommand(name);
}
void* AssetResolverDialogCmd::creator() { return new AssetResolverDialogCmd(); }

MStatus AssetResolverDialogCmd::doIt(const MArgList& args)
{
    MStatus    st;
    MArgParser argData(syntax(), args, &st);

    if (st) {
        const MString tabName = parseTextArg(argData, kTabFlag, kPathsTabName);

        if (g_assetResolverDialog) {
            if (tabName == kSettingsTabName) {
                g_assetResolverDialog->setCurrentTab(
                    Adsk::AssetResolverPathDialog::Tab::GlobalSettings);
            } else {
                g_assetResolverDialog->setCurrentTab(Adsk::AssetResolverPathDialog::Tab::Paths);
            }
            g_assetResolverDialog->raise();
            g_assetResolverDialog->activateWindow();
            return MS::kSuccess;
        }

        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        const MString parentWindowName = parseTextArg(argData, kParentWindowFlag, "");
        QWidget*      parentWindow = findParentWindow(parentWindowName);

        g_assetResolverDialog = new Adsk::AssetResolverPathDialog(parentWindow);
        g_assetResolverDialog->setAttribute(Qt::WA_DeleteOnClose);

        g_assetResolverDialog->setGetStagesFunctor([]() {
            auto allStages = ufe::UsdStageMap::getInstance().allStages();
            return std::vector<PXR_NS::UsdStageRefPtr>(allStages.begin(), allStages.end());
        });

        if (tabName == kSettingsTabName) {
            g_assetResolverDialog->setCurrentTab(
                Adsk::AssetResolverPathDialog::Tab::GlobalSettings);
        } else {
            g_assetResolverDialog->setCurrentTab(Adsk::AssetResolverPathDialog::Tab::Paths);
        }

        QObject::connect(
            g_assetResolverDialog,
            &Adsk::AssetResolverPathDialog::settingsApplied,
            [](const Adsk::AssetResolverSettings& settings) {
                PreferencesManagement::SaveUsdPreferences(settings);
            });

        QApplication::restoreOverrideCursor();
        g_assetResolverDialog->show();
        return MS::kSuccess;
    }

    return MS::kInvalidParameter;
}

MSyntax AssetResolverDialogCmd::createSyntax()
{
    MSyntax syntax;
    syntax.enableQuery(true);
    syntax.enableEdit(false);
    syntax.addFlag(kParentWindowFlag, kParentWindowFlagLong, MSyntax::kString);
    syntax.addFlag(kTabFlag, kTabFlagLong, MSyntax::kString);

    syntax.setObjectType(MSyntax::kStringObjects, 0, 1);
    return syntax;
}

} // namespace MAYAUSD_NS_DEF