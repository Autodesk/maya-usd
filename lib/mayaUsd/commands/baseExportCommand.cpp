//
// Copyright 2016 Pixar
// Copyright 2020 Autodesk
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
#include "baseExportCommand.h"

#include <vector>

#include <pxr/pxr.h>

#include <maya/MGlobal.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MFileObject.h>
#include <maya/MSelectionList.h>

#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/utils/writeUtil.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {

MSyntax MayaUSDExportCommand::createSyntax()
{
    MSyntax syntax;

    // These flags correspond to entries in
    // UsdMayaJobExportArgs::GetDefaultDictionary.
    syntax.addFlag(kMergeTransformAndShapeFlag,
                   UsdMayaJobExportArgsTokens->mergeTransformAndShape.GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag(kExportInstancesFlag,
                   UsdMayaJobExportArgsTokens->exportInstances.GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag(kExportRefsAsInstanceableFlag,
                   UsdMayaJobExportArgsTokens->exportRefsAsInstanceable.GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag(kExportDisplayColorFlag,
                   UsdMayaJobExportArgsTokens->exportDisplayColor.GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag(kShadingModeFlag,
                   UsdMayaJobExportArgsTokens->shadingMode.GetText(),
                   MSyntax::kString);
    syntax.addFlag(kMaterialsScopeNameFlag,
                   UsdMayaJobExportArgsTokens->materialsScopeName.GetText(),
                   MSyntax::kString);
    syntax.addFlag(kExportUVsFlag,
                   UsdMayaJobExportArgsTokens->exportUVs.GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag(kExportMaterialCollectionsFlag,
                   UsdMayaJobExportArgsTokens->exportMaterialCollections
                       .GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag(kMaterialCollectionsPathFlag,
                   UsdMayaJobExportArgsTokens->materialCollectionsPath.GetText(),
                   MSyntax::kString);
    syntax.addFlag(kExportCollectionBasedBindingsFlag,
                   UsdMayaJobExportArgsTokens->exportCollectionBasedBindings
                       .GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag(kNormalizeNurbsFlag,
                   UsdMayaJobExportArgsTokens->normalizeNurbs.GetText() ,
                   MSyntax::kBoolean);
    syntax.addFlag(kExportColorSetsFlag,
                   UsdMayaJobExportArgsTokens->exportColorSets.GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag(kStripNamespacesFlag,
                   UsdMayaJobExportArgsTokens->stripNamespaces.GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag(kEulerFilterFlag,
                   UsdMayaJobExportArgsTokens->eulerFilter.GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag(kDefaultMeshSchemeFlag,
                   UsdMayaJobExportArgsTokens->defaultMeshScheme.GetText(),
                   MSyntax::kString);
    syntax.addFlag(kExportVisibilityFlag,
                   UsdMayaJobExportArgsTokens->exportVisibility.GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag(kExportReferenceObjectsFlag,
                   UsdMayaJobExportArgsTokens->exportReferenceObjects.GetText(),
                   MSyntax::kBoolean);
    syntax.addFlag(kExportSkelsFlag,
                   UsdMayaJobExportArgsTokens->exportSkels.GetText(),
                   MSyntax::kString);
    syntax.addFlag(kExportSkinFlag,
                   UsdMayaJobExportArgsTokens->exportSkin.GetText(),
                   MSyntax::kString);
    syntax.addFlag(kParentScopeFlag,
                   UsdMayaJobExportArgsTokens->parentScope.GetText(),
                   MSyntax::kString);
    syntax.addFlag(kRenderableOnlyFlag,
                   UsdMayaJobExportArgsTokens->renderableOnly.GetText(),
                   MSyntax::kNoArg);
    syntax.addFlag(kDefaultCamerasFlag,
                   UsdMayaJobExportArgsTokens->defaultCameras.GetText(),
                   MSyntax::kNoArg);
    syntax.addFlag(kRenderLayerModeFlag,
                   UsdMayaJobExportArgsTokens->renderLayerMode.GetText(),
                   MSyntax::kString);
    syntax.addFlag(kKindFlag,
                   UsdMayaJobExportArgsTokens->kind.GetText(),
                   MSyntax::kString);
    syntax.addFlag(kCompatibilityFlag,
                   UsdMayaJobExportArgsTokens->compatibility.GetText(),
                   MSyntax::kString);

    syntax.addFlag(kChaserFlag,
                   UsdMayaJobExportArgsTokens->chaser.GetText(),
                   MSyntax::kString);
    syntax.makeFlagMultiUse(UsdMayaJobExportArgsTokens->chaser.GetText());

    syntax.addFlag(kChaserArgsFlag,
                   UsdMayaJobExportArgsTokens->chaserArgs.GetText(),
                   MSyntax::kString, MSyntax::kString, MSyntax::kString);
    syntax.makeFlagMultiUse(UsdMayaJobExportArgsTokens->chaserArgs.GetText());

    syntax.addFlag(kMelPerFrameCallbackFlag,
                   UsdMayaJobExportArgsTokens->melPerFrameCallback.GetText(),
                   MSyntax::kNoArg);
    syntax.addFlag(kMelPostCallbackFlag,
                   UsdMayaJobExportArgsTokens->melPostCallback.GetText(),
                   MSyntax::kNoArg);
    syntax.addFlag(kPythonPerFrameCallbackFlag,
                   UsdMayaJobExportArgsTokens->pythonPerFrameCallback.GetText(),
                   MSyntax::kString);
    syntax.addFlag(kPythonPostCallbackFlag,
                   UsdMayaJobExportArgsTokens->pythonPostCallback.GetText(),
                   MSyntax::kString);
    syntax.addFlag(kVerboseFlag,
                   UsdMayaJobExportArgsTokens->verbose.GetText(),
                   MSyntax::kNoArg);

    // These are additional flags under our control.
    syntax.addFlag(kFrameRangeFlag, kFrameRangeFlagLong, MSyntax::kDouble, MSyntax::kDouble);
    syntax.addFlag(kFrameStrideFlag, kFrameStrideFlagLong, MSyntax::kDouble);
    syntax.addFlag(kFrameSampleFlag, kFrameSampleFlagLong, MSyntax::kDouble);
    syntax.makeFlagMultiUse(kFrameSampleFlag);

    syntax.addFlag(kAppendFlag, kAppendFlagLong, MSyntax::kBoolean);
    syntax.addFlag(kFileFlag, kFileFlagLong, MSyntax::kString);
    syntax.addFlag(kSelectionFlag, kSelectionFlagLong, MSyntax::kNoArg);

    syntax.addFlag(kFilterTypesFlag, kFilterTypesFlagLong, MSyntax::kString);
    syntax.makeFlagMultiUse(kFilterTypesFlag);

    syntax.enableQuery(false);
    syntax.enableEdit(false);

    syntax.setObjectType(MSyntax::kSelectionList);
    syntax.setMinObjects(0);

    return syntax;
}

void* MayaUSDExportCommand::creator()
{
    return new MayaUSDExportCommand();
}

/* virtual */
std::unique_ptr<UsdMaya_WriteJob> MayaUSDExportCommand::initializeWriteJob(const PXR_NS::UsdMayaJobExportArgs & args)
{
    return std::unique_ptr<UsdMaya_WriteJob>(new UsdMaya_WriteJob(args));
}

MStatus MayaUSDExportCommand::doIt(const MArgList & args)
{
try
{
    MStatus status;

    MArgDatabase argData(syntax(), args, &status);

    // Check that all flags were valid
    if (status != MS::kSuccess) {
        return status;
    }

    // Read all of the dictionary args first.
    const VtDictionary userArgs = UsdMayaUtil::GetDictionaryFromArgDatabase(
            argData, UsdMayaJobExportArgs::GetDefaultDictionary());

    // Now read all of the other args that are specific to this command.
    bool append = false;
    std::string fileName;

    if (argData.isFlagSet(kAppendFlag)) {
        argData.getFlagArgument(kAppendFlag, 0, append);
    }

    if (argData.isFlagSet(kFileFlag))
    {
        // Get the value
        MString tmpVal;
        argData.getFlagArgument(kFileFlag, 0, tmpVal);

        // resolve the path into an absolute path
        MFileObject absoluteFile;
        absoluteFile.setRawFullName(tmpVal);
        absoluteFile.setRawFullName( absoluteFile.resolvedFullName() ); // Make sure an absolute path
        fileName = absoluteFile.resolvedFullName().asChar();

        if (fileName.empty()) {
            fileName = tmpVal.asChar();
        }
    }
    else {
        TF_RUNTIME_ERROR("-file not specified.");
        return MS::kFailure;
    }

    if (fileName.empty()) {
        return MS::kFailure;
    }

    // If you provide a frame range we consider this an anim
    // export even if start and end are the same
    GfInterval timeInterval;
    if (argData.isFlagSet(kFrameRangeFlag)) {
        double startTime = 1;
        double endTime = 1;
        argData.getFlagArgument(kFrameRangeFlag, 0, startTime);
        argData.getFlagArgument(kFrameRangeFlag, 1, endTime);
        if (startTime > endTime) {
            // If the user accidentally set start > end, resync to the closed
            // interval with the single start point.
            timeInterval = GfInterval(startTime);
        }
        else {
            // Use the user's interval as-is.
            timeInterval = GfInterval(startTime, endTime);
        }
    } else {
        // No animation, so empty interval.
        timeInterval = GfInterval();
    }

    double frameStride = 1.0;
    if (argData.isFlagSet(kFrameStrideFlag)) {
        argData.getFlagArgument(kFrameStrideFlag, 0, frameStride);
    }

    std::set<double> frameSamples;
    unsigned int numFrameSamples = argData.numberOfFlagUses(kFrameSampleFlag);
    for (unsigned int i = 0; i < numFrameSamples; ++i) {
        MArgList tmpArgList;
        argData.getFlagArgumentList(kFrameSampleFlag, i, tmpArgList);
        frameSamples.insert(tmpArgList.asDouble(0));
    }

    // Get the objects to export as a MSelectionList
    MSelectionList objSelList;
    if (argData.isFlagSet(kSelectionFlag)) {
        MGlobal::getActiveSelectionList(objSelList);
    }
    else {
        argData.getObjects(objSelList);

        // If no objects specified, then get all objects at DAG root
        if (objSelList.isEmpty()) {
            objSelList.add("|*", true);
        }
    }

    // Convert selection list to jobArgs dagPaths
    UsdMayaUtil::MDagPathSet dagPaths;
    for (unsigned int i=0; i < objSelList.length(); i++) {
        MDagPath dagPath;
        status = objSelList.getDagPath(i, dagPath);
        if (status == MS::kSuccess)
        {
            dagPaths.emplace(dagPath);
        }
    }

    const std::vector<double> timeSamples = UsdMayaWriteUtil::GetTimeSamples(
            timeInterval, frameSamples, frameStride);
    UsdMayaJobExportArgs jobArgs = UsdMayaJobExportArgs::CreateFromDictionary(
            userArgs, dagPaths, timeSamples);

    unsigned int numFilteredTypes = argData.numberOfFlagUses(kFilterTypesFlag);
    for (unsigned int i=0; i < numFilteredTypes; i++) {
        MArgList tmpArgList;
        argData.getFlagArgumentList(kFilterTypesFlag, i, tmpArgList);
        jobArgs.AddFilteredTypeName(tmpArgList.asString(0));
    }

    std::unique_ptr<UsdMaya_WriteJob> writeJob = initializeWriteJob(jobArgs);
    if (!writeJob || !writeJob->Write(fileName, append)) {
        return MS::kFailure;
    }

    return MS::kSuccess;
} // end of try block
catch (std::exception & e)
{
    TF_RUNTIME_ERROR("std::exception encountered: %s", e.what());
    return MS::kFailure;
}
} // end of function

} // MAYAUSD_NS_DEF
