//
// Copyright 2016 Pixar
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
#include "usdMaya/importCommand.h"

#include <mayaUsd/fileio/jobs/jobArgs.h>
#include "usdMaya/readJobWithSceneAssembly.h"
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>

#include "pxr/usd/ar/resolver.h"

#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MFileObject.h>
#include <maya/MSelectionList.h>
#include <maya/MSyntax.h>

#include <map>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE



UsdMayaImportCommand::UsdMayaImportCommand() :
    mUsdReadJob(nullptr)
{
}

/* virtual */
UsdMayaImportCommand::~UsdMayaImportCommand()
{
    if (mUsdReadJob) {
        delete mUsdReadJob;
    }
}

/* static */
MSyntax
UsdMayaImportCommand::createSyntax()
{
    MSyntax syntax;

    // These flags correspond to entries in
    // UsdMayaJobImportArgs::GetDefaultDictionary.
    syntax.addFlag("-shd",
                   UsdMayaJobImportArgsTokens->shadingMode.GetText(),
                   MSyntax::kString);
    syntax.addFlag("-ar",
                   UsdMayaJobImportArgsTokens->assemblyRep.GetText(),
                   MSyntax::kString);
    syntax.addFlag("-md",
                   UsdMayaJobImportArgsTokens->metadata.GetText(),
                   MSyntax::kString);
    syntax.makeFlagMultiUse(UsdMayaJobImportArgsTokens->metadata.GetText());
    syntax.addFlag("-api",
                   UsdMayaJobImportArgsTokens->apiSchema.GetText(),
                   MSyntax::kString);
    syntax.makeFlagMultiUse(UsdMayaJobImportArgsTokens->apiSchema.GetText());
    syntax.addFlag("-epv",
                   UsdMayaJobImportArgsTokens->excludePrimvar.GetText(),
                   MSyntax::kString);
    syntax.makeFlagMultiUse(
            UsdMayaJobImportArgsTokens->excludePrimvar.GetText());
    syntax.addFlag("-uac",
                   UsdMayaJobImportArgsTokens->useAsAnimationCache.GetText(),
                   MSyntax::kBoolean);

    // These are additional flags under our control.
    syntax.addFlag("-f" , "-file", MSyntax::kString);
    syntax.addFlag("-p" , "-parent", MSyntax::kString);
    syntax.addFlag("-ani", "-readAnimData", MSyntax::kBoolean);
    syntax.addFlag("-fr", "-frameRange", MSyntax::kDouble, MSyntax::kDouble);
    syntax.addFlag("-pp", "-primPath", MSyntax::kString);
    syntax.addFlag("-var", "-variant", MSyntax::kString, MSyntax::kString);
    syntax.makeFlagMultiUse("variant");

    syntax.addFlag("-v", "-verbose", MSyntax::kNoArg);

    syntax.enableQuery(false);
    syntax.enableEdit(false);

    return syntax;
}

/* static */
void*
UsdMayaImportCommand::creator()
{
    return new UsdMayaImportCommand();
}

/* virtual */
MStatus
UsdMayaImportCommand::doIt(const MArgList & args)
{
    MStatus status;

    MArgDatabase argData(syntax(), args, &status);

    // Check that all flags were valid
    if (status != MS::kSuccess) {
        return status;
    }

    // Get dictionary values.
    const VtDictionary userArgs =
        UsdMayaUtil::GetDictionaryFromArgDatabase(
            argData,
            UsdMayaJobImportArgs::GetDefaultDictionary());

    std::string mFileName;
    if (argData.isFlagSet("file")) {
        // Get the value
        MString tmpVal;
        argData.getFlagArgument("file", 0, tmpVal);
        mFileName = tmpVal.asChar();

        // Use the usd resolver for validation (but save the unresolved)
        if (ArGetResolver().Resolve(mFileName).empty()) {
            TF_RUNTIME_ERROR(
                    "File '%s' does not exist, or could not be resolved. "
                    "Exiting.",
                    mFileName.c_str());
            return MS::kFailure;
        }

        TF_STATUS("Importing '%s'", mFileName.c_str());
    }

    if (mFileName.empty()) {
        TF_RUNTIME_ERROR("Empty file specified. Exiting.");
        return MS::kFailure;
    }

    // Specify usd PrimPath.  Default will be "/<useFileBasename>"
    std::string mPrimPath;
    if (argData.isFlagSet("primPath"))
    {
        // Get the value
        MString tmpVal;
        argData.getFlagArgument("primPath", 0, tmpVal);
        mPrimPath = tmpVal.asChar();
    }

    // Add variant (variantSet, variant).  Multi-use
    std::map<std::string,std::string> mVariants;
    for (unsigned int i=0; i < argData.numberOfFlagUses("variant"); ++i)
    {
        MArgList tmpArgList;
        status = argData.getFlagArgumentList("variant", i, tmpArgList);
        // Get the value
        MString tmpKey = tmpArgList.asString(0, &status);
        MString tmpVal = tmpArgList.asString(1, &status);
        mVariants.insert( std::pair<std::string, std::string>(tmpKey.asChar(), tmpVal.asChar()) );
    }

    bool readAnimData = true;
    if (argData.isFlagSet("readAnimData")) {
        argData.getFlagArgument("readAnimData", 0, readAnimData);
    }

    GfInterval timeInterval;
    if (readAnimData) {
        if (argData.isFlagSet("frameRange")) {
            double startTime = 1.0;
            double endTime = 1.0;
            argData.getFlagArgument("frameRange", 0, startTime);
            argData.getFlagArgument("frameRange", 1, endTime);
            timeInterval = GfInterval(startTime, endTime);
        }
        else {
            timeInterval = GfInterval::GetFullInterval();
        }
    }
    else {
        timeInterval = GfInterval();
    }

    // Create the command
    if (mUsdReadJob) {
        delete mUsdReadJob;
    }

    UsdMayaJobImportArgs jobArgs =
            UsdMayaJobImportArgs::CreateFromDictionary(
                userArgs,
                /* importWithProxyShapes = */ false,
                timeInterval);

    MayaUsd::ImportData importData(mFileName);
    importData.setRootVariantSelections(std::move(mVariants));
    importData.setRootPrimPath(mPrimPath);

    mUsdReadJob = new UsdMaya_ReadJobWithSceneAssembly(importData, jobArgs);

    // Add optional command params
    if (argData.isFlagSet("parent")) {
        // Get the value
        MString tmpVal;
        argData.getFlagArgument("parent", 0, tmpVal);

        if (tmpVal.length()) {
            MSelectionList selList;
            selList.add(tmpVal);
            MDagPath dagPath;
            status = selList.getDagPath(0, dagPath);
            if (status != MS::kSuccess) {
                TF_RUNTIME_ERROR(
                        "Invalid path '%s' for -parent.",
                        tmpVal.asChar());
                return MS::kFailure;
            }
            mUsdReadJob->SetMayaRootDagPath( dagPath );
        }
    }

    // Execute the command
    std::vector<MDagPath> addedDagPaths;
    bool success = mUsdReadJob->Read(&addedDagPaths);
    if (success) {
        TF_FOR_ALL(iter, addedDagPaths) {
            appendToResult(iter->fullPathName());
        }
    }
    return (success) ? MS::kSuccess : MS::kFailure;
}

/* virtual */
MStatus
UsdMayaImportCommand::redoIt()
{
    if (!mUsdReadJob) {
        return MS::kFailure;
    }

    bool success = mUsdReadJob->Redo();

    return (success) ? MS::kSuccess : MS::kFailure;
}

/* virtual */
MStatus
UsdMayaImportCommand::undoIt()
{
    if (!mUsdReadJob) {
        return MS::kFailure;
    }

    bool success = mUsdReadJob->Undo();

    return (success) ? MS::kSuccess : MS::kFailure;
}


PXR_NAMESPACE_CLOSE_SCOPE
