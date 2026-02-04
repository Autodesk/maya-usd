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
#include <mayaUsd/utils/util.h>
#include <mayaUsdUI/ui/USDAssetResolverDialog.h>
#include <mayaUsdUI/ui/USDQtUtil.h>

#include <maya/MFnPlugin.h>

#include <QtGui/QCursor>
#include <QtWidgets/QApplication>

namespace MAYAUSD_NS_DEF {

const MString AssetResolverDialogCmd::name("assetResolverDialog");

namespace {

constexpr auto kParentWindowFlag = "-pw";
constexpr auto kParentWindowFlagLong = "-parentWindow";

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
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        const MString parentWindowName = parseTextArg(argData, kParentWindowFlag, "");
        QWidget*      parentWindow = findParentWindow(parentWindowName);

        std::unique_ptr<USDAssetResolverDialog> usdAssetResolverDialog(
            new USDAssetResolverDialog(parentWindow));

        QApplication::restoreOverrideCursor();
        usdAssetResolverDialog->execute();
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

    syntax.setObjectType(MSyntax::kStringObjects, 0, 1);
    return syntax;
}

} // namespace MAYAUSD_NS_DEF