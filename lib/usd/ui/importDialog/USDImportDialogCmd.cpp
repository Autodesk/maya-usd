//
// Copyright 2019 Autodesk
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
#include "USDImportDialogCmd.h"

#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/usd/variantSets.h>

#include <maya/MArgParser.h>
#include <maya/MDagPath.h>
#include <maya/MFileObject.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnStringData.h>
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
#include <mayaUsdUI/ui/USDImportDialog.h>
#include <mayaUsdUI/ui/USDQtUtil.h>

#include <maya/MFnPlugin.h>

#include <QtGui/QCursor>
#include <QtWidgets/QApplication>

namespace MAYAUSD_NS_DEF {

const MString USDImportDialogCmd::fsName("usdImportDialog");

namespace {

constexpr auto kPrimPathFlag = "-pp";
constexpr auto kPrimPathFlagLong = "-primPath";
constexpr auto kClearDataFlag = "-cd";
constexpr auto kClearDataFlagLong = "-clearData";
constexpr auto kApplyToProxyFlag = "-ap";
constexpr auto kApplyToProxyFlagLong = "-applyToProxy";

constexpr auto kPrimCountFlag = "-pc";
constexpr auto kPrimCountFlagLong = "-primCount";
constexpr auto kSwitchedVariantCountFlag = "-swc";
constexpr auto kSwitchedVariantCountFlagLong = "-switchedVariantCount";

} // namespace

/*static*/
MStatus USDImportDialogCmd::initialize(MFnPlugin& plugin)
{
    return plugin.registerCommand(
        fsName, USDImportDialogCmd::creator, USDImportDialogCmd::createSyntax);
}

/*static*/
MStatus USDImportDialogCmd::finalize(MFnPlugin& plugin) { return plugin.deregisterCommand(fsName); }

/*static*/
void* USDImportDialogCmd::creator() { return new USDImportDialogCmd(); }

MStatus USDImportDialogCmd::applyToProxy(const MString& proxyPath)
{
    MDagPath       proxyShapeDagPath;
    MSelectionList selection;
    selection.add(proxyPath);
    MStatus status = selection.getDagPath(0, proxyShapeDagPath);
    if (status.error())
        return status;

    MObject proxyShapeObj = proxyShapeDagPath.node(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MFnDependencyNode fn(proxyShapeObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (fn.typeName() != MString("mayaUsdProxyShape"))
        return MS::kInvalidParameter;

    MayaUsdProxyShapeBase* proxyShape = dynamic_cast<MayaUsdProxyShapeBase*>(fn.userNode());
    if (!proxyShape)
        return MS::kInvalidParameter;

    MPlug primPath = fn.findPlug("primPath", &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MPlug filePath = fn.findPlug("filePath", &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    ImportData& importData = ImportData::instance();
    primPath.setValue(MString(importData.rootPrimPath().c_str()));
    CHECK_MSTATUS_AND_RETURN_IT(status);

    filePath.setValue(MString(importData.filename().c_str()));
    CHECK_MSTATUS_AND_RETURN_IT(status);

    auto rootPrim = proxyShape->usdPrim();
    if (!rootPrim)
        return MS::kNotFound;

    auto stage = rootPrim.GetStage();
    if (!stage)
        return MS::kNotFound;

    for (auto& primVariant : importData.primVariantSelections()) {
        auto prim = stage->GetPrimAtPath(primVariant.first);
        if (!prim || !prim.HasVariantSets())
            return MS::kNotFound;

        for (auto& variant : primVariant.second) {
            auto variantSet = prim.GetVariantSet(variant.first);
            if (variantSet)
                variantSet.SetVariantSelection(variant.second);
        }
    }
    return MS::kSuccess;
}

MStatus USDImportDialogCmd::doIt(const MArgList& args)
{
    MStatus    st;
    MArgParser argData(syntax(), args, &st);
    if (!st)
        return st;

    if (argData.isQuery()) {
        if (argData.isFlagSet(kPrimPathFlag)) {
            const ImportData& importData = ImportData::cinstance();
            std::string       rootPrimPath = importData.rootPrimPath();
            setResult(rootPrimPath.c_str());
            return MS::kSuccess;
        }

        if (argData.isFlagSet(kPrimCountFlag)) {
            const ImportData& importData = ImportData::cinstance();
            setResult(importData.primsInScopeCount());
            return MS::kSuccess;
        }

        if (argData.isFlagSet(kSwitchedVariantCountFlag)) {
            const ImportData& importData = ImportData::cinstance();
            setResult(importData.switchedVariantCount());
            return MS::kSuccess;
        }

        return MS::kInvalidParameter;
    }

    // Edit flags below:
    if (argData.isFlagSet(kClearDataFlag)) {
        ImportData& importData = ImportData::instance();
        importData.clearData();
        return MS::kSuccess;
    }

    // No command object is expected
    if (argData.isFlagSet(kApplyToProxyFlag)) {
        MStringArray proxyArray;
        st = argData.getObjects(proxyArray);
        if (!st || proxyArray.length() != 1)
            return MS::kInvalidParameter;

        return applyToProxy(proxyArray[0]);
    }

    MStringArray filenameArray;
    st = argData.getObjects(filenameArray);
    if (st && (filenameArray.length() > 0)) {
        // We only use the first one.
        MFileObject fo;
        MString     assetPath;
        fo.setRawFullName(filenameArray[0]);
        bool validTarget = fo.exists();
        if (!validTarget) {
            // Give the default usd-asset-resolver a chance
            if (const char* cStr = filenameArray[0].asChar()) {
                validTarget = !ArGetResolver().Resolve(cStr).empty();
                if (validTarget)
                    assetPath = filenameArray[0];
            }
        } else
            assetPath = fo.resolvedFullName();

        if (validTarget) {
            USDQtUtil   usdQtUtil;
            ImportData& importData = ImportData::instance();

            // Creating the View can pause Maya, usually only briefly but it's noticable, so we'll
            // toggle the wait cursor to show that it's working.
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

            std::unique_ptr<USDImportDialog> usdImportDialog(new USDImportDialog(
                assetPath.asChar(), &importData, usdQtUtil, MQtUtil::mainWindow()));

            QApplication::restoreOverrideCursor();

            if (usdImportDialog->execute()) {
                // The user clicked 'Apply' so copy the info from the dialog to the import data
                // instance.
                importData.setFilename(usdImportDialog->filename());
                importData.setStageInitialLoadSet(usdImportDialog->stageInitialLoadSet());
                importData.setRootPrimPath(usdImportDialog->rootPrimPath());
                // Don't set the stage pop mask until we solve how to use it together with
                // the root prim path.
                // importData.setStagePopulationMask(usdImportDialog->stagePopulationMask());
                importData.setPrimVariantSelections(usdImportDialog->primVariantSelections());

                importData.setPrimsInScopeCount(usdImportDialog->primsInScopeCount());
                importData.setSwitchedVariantCount(usdImportDialog->switchedVariantCount());

                setResult(assetPath);
            }
            return MS::kSuccess;
        }
    }

    return MS::kInvalidParameter;
}

MSyntax USDImportDialogCmd::createSyntax()
{
    MSyntax syntax;
    syntax.enableQuery(true);
    syntax.enableEdit(false);
    syntax.addFlag(kPrimPathFlag, kPrimPathFlagLong);
    syntax.addFlag(kClearDataFlag, kClearDataFlagLong);
    syntax.addFlag(kApplyToProxyFlag, kApplyToProxyFlagLong);
    syntax.addFlag(kPrimCountFlag, kPrimCountFlagLong);
    syntax.addFlag(kSwitchedVariantCountFlag, kSwitchedVariantCountFlagLong);

    syntax.setObjectType(MSyntax::kStringObjects, 0, 1);
    return syntax;
}

} // namespace MAYAUSD_NS_DEF