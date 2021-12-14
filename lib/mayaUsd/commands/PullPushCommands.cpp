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

#include <mayaUsd/fileio/primUpdaterManager.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/undo/OpUndoItemRecorder.h>
#include <mayaUsd/utils/util.h>

#include <maya/MArgParser.h>
#include <maya/MGlobal.h>
#include <maya/MStringArray.h>
#include <maya/MSyntax.h>
#include <ufe/path.h>
#include <ufe/pathString.h>

#include <algorithm>

namespace MAYAUSD_NS_DEF {
namespace ufe {

namespace {

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

    status = parseArgAsUfePath(text, outputPath);
    if (MS::kSuccess != status)
        return status;

    return MS::kSuccess;
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

} // namespace

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

// MPxCommand API to specify the command is undoable.
bool EditAsMayaCommand::isUndoable() const { return true; }

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

    OpUndoItemRecorder undoRecorder(fUndoInfo);

    auto& manager = PXR_NS::PrimUpdaterManager::getInstance();
    if (!manager.editAsMaya(fPath))
        return MS::kFailure;

    return MS::kSuccess;
}

// MPxCommand API to redo the command.
MStatus EditAsMayaCommand::redoIt() { return fUndoInfo.redo() ? MS::kSuccess : MS::kFailure; }

// MPxCommand API to undo the command.
MStatus EditAsMayaCommand::undoIt() { return fUndoInfo.undo() ? MS::kSuccess : MS::kFailure; }

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
MSyntax MergeToUsdCommand::createSyntax() { return createSyntaxWithUfeArgs(1); }

// MPxCommand API to specify the command is undoable.
bool MergeToUsdCommand::isUndoable() const { return true; }

// MPxCommand API to execute the command.
MStatus MergeToUsdCommand::doIt(const MArgList& argList)
{
    clearResult();

    setCommandString(commandName);

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

    OpUndoItemRecorder undoRecorder(fUndoInfo);

    auto& manager = PXR_NS::PrimUpdaterManager::getInstance();
    if (!manager.mergeToUsd(fDagNode, fPulledPath))
        return MS::kFailure;

    return MS::kSuccess;
}

// MPxCommand API to redo the command.
MStatus MergeToUsdCommand::redoIt() { return fUndoInfo.redo() ? MS::kSuccess : MS::kFailure; }

// MPxCommand API to undo the command.
MStatus MergeToUsdCommand::undoIt() { return fUndoInfo.undo() ? MS::kSuccess : MS::kFailure; }

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

// MPxCommand API to specify the command is undoable.
bool DiscardEditsCommand::isUndoable() const { return true; }

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

    OpUndoItemRecorder undoRecorder(fUndoInfo);

    auto& manager = PXR_NS::PrimUpdaterManager::getInstance();
    if (!manager.discardEdits(fPath))
        return MS::kFailure;

    return MS::kSuccess;
}

// MPxCommand API to redo the command.
MStatus DiscardEditsCommand::redoIt() { return fUndoInfo.redo() ? MS::kSuccess : MS::kFailure; }

// MPxCommand API to undo the command.
MStatus DiscardEditsCommand::undoIt() { return fUndoInfo.undo() ? MS::kSuccess : MS::kFailure; }

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
MSyntax DuplicateCommand::createSyntax() { return createSyntaxWithUfeArgs(2); }

// MPxCommand API to specify the command is undoable.
bool DuplicateCommand::isUndoable() const { return true; }

// MPxCommand API to execute the command.
MStatus DuplicateCommand::doIt(const MArgList& argList)
{
    clearResult();

    setCommandString(commandName);

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

    OpUndoItemRecorder undoRecorder(fUndoInfo);

    auto& manager = PXR_NS::PrimUpdaterManager::getInstance();
    if (!manager.duplicate(fSrcPath, fDstPath))
        return MS::kFailure;

    return MS::kSuccess;
}

// MPxCommand API to redo the command.
MStatus DuplicateCommand::redoIt() { return fUndoInfo.redo() ? MS::kSuccess : MS::kFailure; }

// MPxCommand API to undo the command.
MStatus DuplicateCommand::undoIt() { return fUndoInfo.undo() ? MS::kSuccess : MS::kFailure; }

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
