//
// Copyright 2021 Autodesk
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
#include "PullPushCommands.h"

#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/primUpdaterManager.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/undo/OpUndoItemRecorder.h>
#include <mayaUsd/undo/OpUndoItems.h>
#include <mayaUsd/utils/util.h>

#include <maya/MArgParser.h>
#include <maya/MGlobal.h>
#include <maya/MStringArray.h>
#include <maya/MSyntax.h>
#include <ufe/hierarchy.h>
#include <ufe/path.h>
#include <ufe/pathString.h>
#include <ufe/selection.h>

#include <algorithm>

namespace MAYAUSD_NS_DEF {
namespace ufe {

namespace {

static constexpr auto kExportOptionsFlag = "exo";
static constexpr auto kExportOptionsFlagLong = "exportOptions";

// Reports an error to the Maya scripting console.
void reportError(const MString& errorString) { MGlobal::displayError(errorString); }

// Reports an error based on the status. Reports nothing on success.
MStatus reportError(MStatus status)
{
    switch (status.statusCode()) {
    case MStatus::kNotFound: reportError("No object were provided."); break;
    case MStatus::kInvalidParameter: reportError("Invalid object path."); break;
    case MStatus::kUnknownParameter: reportError("Invalid parameter."); break;
    case MStatus::kSuccess: break;
    default: reportError("Command parsing error."); break;
    }

    return status;
}

// Parses a string as a UFE path, detecting invalid paths and empty paths.
MStatus parseArgAsUfePath(const MString& arg, Ufe::Path& outputPath)
{
    // Note: parsing can throw exceptions. Treat them as parsing error and return an empty path.
    try {
        outputPath = Ufe::PathString::path(arg.asChar());

        if (outputPath.size() == 0)
            return MS::kNotFound;

        return MS::kSuccess;
    } catch (const std::exception&) {
        return MS::kInvalidParameter;
    }
}

// Create the syntax for a command taking some string parameters representing UFE paths.
MSyntax createSyntaxWithUfeArgs(int paramCount)
{
    MSyntax syntax;

    syntax.enableQuery(false);
    syntax.enableEdit(false);

    for (int i = 0; i < paramCount; ++i)
        syntax.addArg(MSyntax::kString);

    return syntax;
}

// Verifies if a UFE path corresponds to a valid USD prim.
bool isPrimPath(const Ufe::Path& path) { return ufePathToPrim(path).IsValid(); }

// Parse the indexed argument as text.
MStatus parseTextArg(const MArgParser& argParser, int index, MString& outputText)
{
    argParser.getCommandArgument(index, outputText);
    if (outputText.length() <= 0)
        return MS::kNotFound;

    return MS::kSuccess;
}

// Parse the indexed argument as a UFE path.
MStatus parseUfePathArg(const MArgParser& argParser, int index, Ufe::Path& outputPath)
{
    MString text;
    MStatus status = parseTextArg(argParser, index, text);
    if (MS::kSuccess != status)
        return status;

    return parseArgAsUfePath(text, outputPath);
}

MStatus parseDagPathArg(const MArgParser& argParser, int index, MDagPath& outputDagPath)
{
    MString text;
    MStatus status = parseTextArg(argParser, index, text);
    if (MS::kSuccess != status)
        return status;

    MObject obj;
    status = PXR_NS::UsdMayaUtil::GetMObjectByName(text, obj);
    if (status != MStatus::kSuccess)
        return status;

    return MDagPath::getAPathTo(obj, outputDagPath);
}

MString parseTextArg(const MArgDatabase& argData, const char* flag, const MString& defaultValue)
{
    MString value = defaultValue;
    if (argData.isFlagSet(flag))
        argData.getFlagArgument(flag, 0, value);
    return value;
}

} // namespace

//------------------------------------------------------------------------------
// PullPushBaseCommand
//------------------------------------------------------------------------------

// MPxCommand API to specify the command is undoable.
bool PullPushBaseCommand::isUndoable() const { return true; }

// MPxCommand API to redo the command.
MStatus PullPushBaseCommand::redoIt() { return fUndoItemList.redo() ? MS::kSuccess : MS::kFailure; }

// MPxCommand API to undo the command.
MStatus PullPushBaseCommand::undoIt() { return fUndoItemList.undo() ? MS::kSuccess : MS::kFailure; }

//------------------------------------------------------------------------------
// EditAsMayaCommand
//------------------------------------------------------------------------------

// The edit as maya command name.
const char EditAsMayaCommand::commandName[] = "mayaUsdEditAsMaya";

// Empty edit as maya command.
EditAsMayaCommand::EditAsMayaCommand() { }

// MPxCommand API to create the command object.
void* EditAsMayaCommand::creator()
{
    // Note: use static cast to make sure the right pointer type is returned into the void *.
    return static_cast<MPxCommand*>(new EditAsMayaCommand());
}

// MPxCommand API to register the command syntax.
MSyntax EditAsMayaCommand::createSyntax() { return createSyntaxWithUfeArgs(1); }

// MPxCommand API to execute the command.
MStatus EditAsMayaCommand::doIt(const MArgList& argList)
{
    clearResult();

    setCommandString(commandName);

    MStatus    status = MS::kSuccess;
    MArgParser argParser(syntax(), argList, &status);
    if (status != MS::kSuccess)
        return status;

    status = parseUfePathArg(argParser, 0, fPath);
    if (status != MS::kSuccess)
        return reportError(status);

    if (!isPrimPath(fPath))
        return reportError(MS::kInvalidParameter);

    // Scope the undo item recording so we can undo on failure.
    {
        OpUndoItemRecorder undoRecorder(fUndoItemList);

        auto& manager = PXR_NS::PrimUpdaterManager::getInstance();
        status = manager.editAsMaya(fPath) ? MS::kSuccess : MS::kFailure;
    }

    // Undo potentially partially-made edit-as-Maya on failure.
    if (status != MS::kSuccess)
        fUndoItemList.undo();

    return status;
}

//------------------------------------------------------------------------------
// MergeToUsdCommand
//------------------------------------------------------------------------------

// The merge to USD command name.
const char MergeToUsdCommand::commandName[] = "mayaUsdMergeToUsd";

// Empty merge to USD command.
MergeToUsdCommand::MergeToUsdCommand() { }

// MPxCommand API to create the command object.
void* MergeToUsdCommand::creator()
{
    // Note: use static cast to make sure the right pointer type is returned into the void *.
    return static_cast<MPxCommand*>(new MergeToUsdCommand());
}

// MPxCommand API to register the command syntax.
MSyntax MergeToUsdCommand::createSyntax()
{
    MSyntax syntax = createSyntaxWithUfeArgs(1);
    syntax.addFlag(kExportOptionsFlag, kExportOptionsFlagLong, MSyntax::kString);
    return syntax;
}

// MPxCommand API to execute the command.
MStatus MergeToUsdCommand::doIt(const MArgList& argList)
{
    clearResult();

    setCommandString(commandName);

    PXR_NS::VtDictionary userArgs;

    MStatus    status = MS::kSuccess;
    MArgParser argParser(syntax(), argList, &status);
    if (status != MS::kSuccess)
        return status;

    if (status != MStatus::kSuccess)
        return status;

    MDagPath dagPath;
    status = parseDagPathArg(argParser, 0, dagPath);
    if (status != MS::kSuccess)
        return reportError(status);

    status = fDagNode.setObject(dagPath);
    if (status != MS::kSuccess)
        return reportError(status);

    if (!PXR_NS::PrimUpdaterManager::readPullInformation(dagPath, fPulledPath))
        return reportError(MS::kInvalidParameter);

    MArgDatabase argData(syntax(), argList, &status);
    if (status != MS::kSuccess)
        return reportError(status);

    const MString exportOptions = parseTextArg(argData, kExportOptionsFlag, "");
    if (exportOptions.length() > 0) {
        status = PXR_NS::UsdMayaJobExportArgs::GetDictionaryFromEncodedOptions(
            exportOptions, &userArgs, nullptr);
        if (status != MS::kSuccess)
            return status;
    }

    // Scope the undo item recording so we can undo on failure.
    {
        OpUndoItemRecorder undoRecorder(fUndoItemList);

        auto& manager = PXR_NS::PrimUpdaterManager::getInstance();
        status = manager.mergeToUsd(fDagNode, fPulledPath, userArgs) ? MS::kSuccess : MS::kFailure;
    }

    // Undo potentially partially-made merge-to-USD on failure.
    if (status != MS::kSuccess)
        fUndoItemList.undo();

    return status;
}

//------------------------------------------------------------------------------
// DiscardEditsCommand
//------------------------------------------------------------------------------

// The discard edits command name.
const char DiscardEditsCommand::commandName[] = "mayaUsdDiscardEdits";

// Empty discard edits command.
DiscardEditsCommand::DiscardEditsCommand() { }

// MPxCommand API to create the command object.
void* DiscardEditsCommand::creator()
{
    // Note: use static cast to make sure the right pointer type is returned into the void *.
    return static_cast<MPxCommand*>(new DiscardEditsCommand());
}

// MPxCommand API to register the command syntax.
MSyntax DiscardEditsCommand::createSyntax() { return createSyntaxWithUfeArgs(1); }

// MPxCommand API to execute the command.
MStatus DiscardEditsCommand::doIt(const MArgList& argList)
{
    clearResult();

    setCommandString(commandName);

    MStatus    status = MS::kSuccess;
    MArgParser argParser(syntax(), argList, &status);
    if (status != MS::kSuccess)
        return status;

    MString nodeName;
    status = parseTextArg(argParser, 0, nodeName);
    if (status != MS::kSuccess)
        return reportError(status);

    MDagPath dagPath = PXR_NS::UsdMayaUtil::nameToDagPath(nodeName.asChar());
    if (!PXR_NS::PrimUpdaterManager::readPullInformation(dagPath, fPath))
        return reportError(MS::kInvalidParameter);

    // Scope the undo item recording so we can undo on failure.
    {
        OpUndoItemRecorder undoRecorder(fUndoItemList);

        auto& manager = PXR_NS::PrimUpdaterManager::getInstance();
        status = manager.discardEdits(dagPath) ? MS::kSuccess : MS::kFailure;
    }

    // Undo potentially partially-made discard-edit on failure.
    if (status != MS::kSuccess)
        fUndoItemList.undo();

    return status;
}

//------------------------------------------------------------------------------
// DuplicateCommand
//------------------------------------------------------------------------------

// The copy between Maya and USD command name.
const char DuplicateCommand::commandName[] = "mayaUsdDuplicate";

// Empty copy between Maya and USD command.
DuplicateCommand::DuplicateCommand() { }

// MPxCommand API to create the command object.
void* DuplicateCommand::creator()
{
    // Note: use static cast to make sure the right pointer type is returned into the void *.
    return static_cast<MPxCommand*>(new DuplicateCommand());
}

// MPxCommand API to register the command syntax.
MSyntax DuplicateCommand::createSyntax()
{
    MSyntax syntax = createSyntaxWithUfeArgs(2);
    syntax.addFlag(kExportOptionsFlag, kExportOptionsFlagLong, MSyntax::kString);
    return syntax;
}

// MPxCommand API to execute the command.
MStatus DuplicateCommand::doIt(const MArgList& argList)
{
    clearResult();

    setCommandString(commandName);

    PXR_NS::VtDictionary userArgs;

    MStatus    status = MS::kSuccess;
    MArgParser argParser(syntax(), argList, &status);
    if (status != MS::kSuccess)
        return status;

    status = parseUfePathArg(argParser, 0, fSrcPath);
    if (status != MS::kSuccess)
        return reportError(status);

    status = parseUfePathArg(argParser, 1, fDstPath);
    if (status != MS::kSuccess)
        return reportError(status);

    MArgDatabase argData(syntax(), argList, &status);
    if (status != MS::kSuccess)
        return reportError(status);

    const MString exportOptions = parseTextArg(argData, kExportOptionsFlag, "");
    if (exportOptions.length() > 0) {
        status = PXR_NS::UsdMayaJobExportArgs::GetDictionaryFromEncodedOptions(
            exportOptions, &userArgs, nullptr);
        if (status != MS::kSuccess)
            return status;
    }

    // Scope the undo item recording so we can undo on failure.
    {
        OpUndoItemRecorder undoRecorder(fUndoItemList);

        auto& manager = PXR_NS::PrimUpdaterManager::getInstance();
        status = manager.duplicate(fSrcPath, fDstPath, userArgs) ? MS::kSuccess : MS::kFailure;

        // If the duplicate src is Maya, the duplicate child of the destination
        // will be USD, and vice-versa.  Construct an appropriate child path:
        // - Maya duplicate to USD: we always duplicate directly under the
        //   proxy shape, so add a single path component USD path segment.
        // - USD duplicate to Maya: no path segment to add.
        Ufe::Path childPath = (fSrcPath.runTimeId() == getMayaRunTimeId())
            ?
            // Maya duplicate to USD
            fDstPath + Ufe::PathSegment(fSrcPath.back(), getUsdRunTimeId(), '/')
            // USD duplicate to Maya
            : fDstPath + fSrcPath.back();

        Ufe::Selection sn;
        sn.append(Ufe::Hierarchy::createItem(childPath));
        // It is appropriate to use the overload that uses the global list, as
        // the undo recorder will transfer the items on the global list to
        // fUndoItemList.
        UfeSelectionUndoItem::select("duplicate", sn);
    }

    // Undo potentially partially-made duplicate on failure.
    if (status != MS::kSuccess)
        fUndoItemList.undo();

    return status;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
