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
#include "views/USDImportDialog.h"
#include "views/USDQtUtil.h"

#include <mayaUsd/fileio/importData.h>

#include <maya/MStatus.h>
#include <maya/MSyntax.h>
#include <maya/MArgParser.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MFileObject.h>
#include <maya/MQtUtil.h>

// This is added to prevent multiple definitions of the MApiVersion string.
#define MNoVersionString
#include <maya/MFnPlugin.h>

MAYAUSD_NS_DEF {

const MString USDImportDialogCmd::fsName("usdImportDialog");

namespace {

constexpr auto kPrimPathFlag = "-pp";
constexpr auto kPrimPathFlagLong = "-primPath";
constexpr auto kClearDataFlag = "-cd";
constexpr auto kClearDataFlagLong = "-clearData";

}

/*static*/
MStatus USDImportDialogCmd::initialize(MFnPlugin& plugin)
{
	return plugin.registerCommand(
		fsName,
		USDImportDialogCmd::creator,
		USDImportDialogCmd::createSyntax);
}

/*static*/
MStatus USDImportDialogCmd::finalize(MFnPlugin& plugin)
{
	return plugin.deregisterCommand(fsName);
}

/*static*/
void* USDImportDialogCmd::creator()
{
	return new USDImportDialogCmd();
}

MStatus USDImportDialogCmd::doIt(const MArgList& args)
{
	MStatus st;
	MArgParser argData(syntax(), args, &st);
	if (!st) return st;

	if (argData.isQuery())
	{
		if (argData.isFlagSet(kPrimPathFlag))
		{
			const ImportData& importData = ImportData::cinstance();
			std::string rootPrimPath = importData.rootPrimPath();
			setResult(rootPrimPath.c_str());
			return MS::kSuccess;
		}

		return MS::kInvalidParameter;
	}

	// Edit flags below:
	if(argData.isFlagSet(kClearDataFlag))
	{
		ImportData& importData = ImportData::instance();
		importData.clearData();
		return MS::kSuccess;
	}

	MStringArray filenameArray;
	st = argData.getObjects(filenameArray);
	if (st && (filenameArray.length() > 0))
	{
		// We only use the first one.
		MFileObject fo;
		fo.setRawFullName(filenameArray[0]);
		if (fo.exists())
		{
			USDQtUtil usdQtUtil;
			ImportData& importData = ImportData::instance();
			MString usdFile = fo.resolvedFullName();
			std::unique_ptr<IUSDImportView> usdImportDialog(new USDImportDialog(usdFile.asChar(), &importData, usdQtUtil, MQtUtil::mainWindow()));
			if (usdImportDialog->execute())
			{
				// The user clicked 'Apply' so copy the info from the dialog to the import data instance.
				importData.setFilename(usdImportDialog->filename());
				importData.setStageInitialLoadSet(usdImportDialog->stageInitialLoadSet());
				importData.setRootPrimPath(usdImportDialog->rootPrimPath());
				// Don't set the stage pop mask until we solve how to use it together with
				// the root prim path.
				//importData.setStagePopulationMask(usdImportDialog->stagePopulationMask());
				importData.setPrimVariantSelections(usdImportDialog->primVariantSelections());
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
	syntax.setObjectType(MSyntax::kStringObjects, 1, 1);
	return syntax;
}

} // namespace MayaUsd