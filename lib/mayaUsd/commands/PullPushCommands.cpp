//
// Copyright 2022 Autodesk
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

#include <maya/MArgList.h>
#include <maya/MArgParser.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnDagNode.h>
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

static constexpr auto kIgnoreVariantsFlag = "iva";
static constexpr auto kIgnoreVariantsFlagLong = "ignoreVariants";

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
MStatus parseTextArg(
    const MArgParser& argParser,
    int               index,
    MString&          outputText,
    const bool        allowEmpty = false)
{
    MStatus st;
    st = argParser.getCommandArgument(index, outputText);
    CHECK_MSTATUS_AND_RETURN_IT(st);

    // Note: when requested, we allow the outputText string to be empty
    //       (input string to command was empty string). In that case
    //       it will mean the Maya (hidden) world node.
    if (!allowEmpty && outputText.length() <= 0)
        return MS::kNotFound;

    return MS::kSuccess;
}

// Parse the indexed argument as a UFE path.
MStatus parseUfePathArg(
    const MArgParser& argParser,
    int               index,
    Ufe::Path&        outputPath,
    const bool        allowEmpty = false)
{
    MString text;
    MStatus status = parseTextArg(argParser, index, text, allowEmpty);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Note: when requested we allow the Ufe outputPath to be empty
    //       which will mean the Maya (hidden) world node.
    if (allowEmpty && (text.length() == 0)) {
        outputPath = Ufe::Path();
        return MS::kSuccess;
    }

    return parseArgAsUfePath(text, outputPath);
}

MStatus parseObjectArg(const MArgParser& argParser, int index, MObject& outputObject)
{
    MString text;
    MStatus status = parseTextArg(argParser, index, text);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return PXR_NS::UsdMayaUtil::GetMObjectByName(text, outputObject);
}

MStatus parseDagObjects(const MArgParser& argParser, MDagPathArray& outputDagPaths)
{
    MStringArray stringObjects;
    MStatus      status = argParser.getObjects(stringObjects);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    for (const MString& text : stringObjects) {
        MObject obj;
        status = PXR_NS::UsdMayaUtil::GetMObjectByName(text, obj);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MDagPath dagPath;
        status = MDagPath::getAPathTo(obj, dagPath);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        outputDagPaths.append(dagPath);
    }
    return MS::kSuccess;
}

MString parseTextArg(const MArgDatabase& argData, const char* flag, const MString& defaultValue)
{
    MString value = defaultValue;
    if (argData.isFlagSet(flag))
        argData.getFlagArgument(flag, 0, value);
    return value;
}

MStringArray parseTextArrayArg(const MArgDatabase& argData, const char* flag)
{
    MStringArray values;
    if (argData.isFlagSet(flag)) {
        const auto count = argData.numberOfFlagUses(flag);
        values.setLength(count);
        for (unsigned int i = 0; i < count; i++) {
            MArgList tmpArgList;
            if (argData.getFlagArgumentList(flag, i, tmpArgList))
                values[i] = tmpArgList.asString(0);
        }
    }
    return values;
}

} // namespace

//------------------------------------------------------------------------------
// PullPushBaseCommand
//------------------------------------------------------------------------------

// MPxCommand API to specify the command is undoable.
bool PullPushBaseCommand::isUndoable() const { return true; }

// MPxCommand API to redo the command.
MStatus PullPushBaseCommand::redoIt() { return _undoItemList.redo() ? MS::kSuccess : MS::kFailure; }

// MPxCommand API to undo the command.
MStatus PullPushBaseCommand::undoIt() { return _undoItemList.undo() ? MS::kSuccess : MS::kFailure; }

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
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = parseUfePathArg(argParser, 0, _path);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (!isPrimPath(_path))
        return reportError(MS::kInvalidParameter);

    // Scope the undo item recording so we can undo on failure.
    {
        OpUndoItemRecorder undoRecorder(_undoItemList);

        auto& manager = PXR_NS::PrimUpdaterManager::getInstance();
        status = manager.editAsMaya(_path) ? MS::kSuccess : MS::kFailure;
    }

    // Undo potentially partially-made edit-as-Maya on failure.
    if (status != MS::kSuccess)
        _undoItemList.undo();

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
    MSyntax syntax;
    syntax.enableQuery(false);
    syntax.enableEdit(false);
    syntax.setObjectType(MSyntax::kStringObjects, /*minimumObjects=*/1);
    syntax.addFlag(kExportOptionsFlag, kExportOptionsFlagLong, MSyntax::kString);
    syntax.makeFlagMultiUse(kExportOptionsFlag);
    syntax.addFlag(kIgnoreVariantsFlag, kIgnoreVariantsFlagLong, MSyntax::kBoolean);
    return syntax;
}

// MPxCommand API to execute the command.
MStatus MergeToUsdCommand::doIt(const MArgList& argList)
{
    clearResult();

    setCommandString(commandName);

    MStatus    status = MS::kSuccess;
    MArgParser argParser(syntax(), argList, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MDagPathArray dagPaths;
    status = parseDagObjects(argParser, dagPaths);
    if (status != MS::kSuccess)
        return reportError(status);

    MArgDatabase argData(syntax(), argList, &status);
    if (status != MS::kSuccess)
        return reportError(status);

    PXR_NS::VtDictionary commandUserArgs;
    if (argData.isFlagSet(kIgnoreVariantsFlag)) {
        const int index = 0;
        commandUserArgs[UsdMayaPrimUpdaterArgsTokens->ignoreVariants]
            = argData.flagArgumentBool(kIgnoreVariantsFlag, index);
    }

    // Parse exportOptions strings, decoding them to UsdMayaJobExportArgs dictionaries.
    std::vector<PXR_NS::VtDictionary> dagUserArgs;
    {
        const MStringArray exportOptions = parseTextArrayArg(argData, kExportOptionsFlag);
        const auto         numExportOptions = exportOptions.length();

        if (numExportOptions == 0) {
            dagUserArgs.push_back(commandUserArgs);
        } else {
            if ((numExportOptions != 1) && (numExportOptions != dagPaths.length())) {
                reportError("When providing multiple exportOptions, the number of exportOptions "
                            "must match the number of dag objects.");
                return MS::kFailure;
            }

            dagUserArgs.resize(numExportOptions);
            for (unsigned int i = 0; i < numExportOptions; i++) {
                status = PXR_NS::UsdMayaJobExportArgs::GetDictionaryFromEncodedOptions(
                    exportOptions[i], &dagUserArgs[i]);

                CHECK_MSTATUS_AND_RETURN_IT(status);

                PXR_NS::VtDictionaryOver(commandUserArgs, &dagUserArgs[i]);
            }
        }
    }

    // Create the merge operations args for each given dagPath.
    std::vector<PushToUsdArgs> mergeArgsVect;
    mergeArgsVect.reserve(dagPaths.length());

    for (std::size_t i = 0; i < dagPaths.length(); i++) {
        // Fewer exportOptions than dag objects means one applies to all objects.
        const auto& userArgs = dagUserArgs.at(std::min(i, dagUserArgs.size() - 1));
        auto        mergeArgs = PXR_NS::PushToUsdArgs::forMerge(dagPaths[i], userArgs);

        if (!mergeArgs)
            return reportError(MS::kInvalidParameter);

        mergeArgsVect.push_back(std::move(mergeArgs));
    }

    // Scope the undo item recording so we can undo on failure.
    {
        OpUndoItemRecorder undoRecorder(_undoItemList);

        auto&      manager = PXR_NS::PrimUpdaterManager::getInstance();
        const auto mergedPaths = manager.mergeToUsd(mergeArgsVect);
        status = (mergedPaths.size() == mergeArgsVect.size()) ? MS::kSuccess : MS::kFailure;

        if (status == MS::kSuccess) {
            // Select the merged prims.  See DuplicateCommand::doIt() comments.
            Ufe::Selection sn;
            for (const auto& mergedPath : mergedPaths)
                sn.append(Ufe::Hierarchy::createItem(mergedPath));
            if (!UfeSelectionUndoItem::select("mergeToUsd: select merged prim", sn)) {
                return MS::kFailure;
            }
        }
    }

    // Undo potentially partially-made merge-to-USD on failure.
    if (status != MS::kSuccess)
        _undoItemList.undo();

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
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MString nodeName;
    status = parseTextArg(argParser, 0, nodeName);
    if (status != MS::kSuccess)
        return reportError(status);

    MDagPath  dagPath = PXR_NS::UsdMayaUtil::nameToDagPath(nodeName.asChar());
    Ufe::Path pulledPath;
    if (!readPullInformation(dagPath, pulledPath))
        return reportError(MS::kInvalidParameter);

    // Scope the undo item recording so we can undo on failure.
    {
        OpUndoItemRecorder undoRecorder(_undoItemList);

        auto& manager = PXR_NS::PrimUpdaterManager::getInstance();
        status = manager.discardEdits(dagPath) ? MS::kSuccess : MS::kFailure;

        if (status == MS::kSuccess) {
            // Select the original prim, if it exists --- orphaned DG edits will
            // have no corresponding prim.  If it does not exist, the selection
            // will simply be cleared.  See DuplicateCommand::doIt() comments.
            Ufe::Selection sn;
            auto           pulledItem = Ufe::Hierarchy::createItem(pulledPath);
            if (pulledItem) {
                sn.append(pulledItem);
            }
            if (!UfeSelectionUndoItem::select("discardEdits: select original prim", sn)) {
                return MS::kFailure;
            }
        }
    }

    // Undo potentially partially-made discard-edit on failure.
    if (status != MS::kSuccess)
        _undoItemList.undo();

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
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject   srcMayaObject;
    Ufe::Path srcPath;
    status = parseUfePathArg(argParser, 0, srcPath);
    if (status != MS::kSuccess) {
        status = parseObjectArg(argParser, 0, srcMayaObject);
        if (status != MS::kSuccess) {
            return reportError(status);
        }
    }

    Ufe::Path dstPath;
    status = parseUfePathArg(argParser, 1, dstPath, true /*allowEmpty*/);
    if (status != MS::kSuccess)
        return reportError(status);

    MArgDatabase argData(syntax(), argList, &status);
    if (status != MS::kSuccess)
        return reportError(status);

    const MString exportOptions = parseTextArg(argData, kExportOptionsFlag, "");
    if (exportOptions.length() > 0) {
        status = PXR_NS::UsdMayaJobExportArgs::GetDictionaryFromEncodedOptions(
            exportOptions, &userArgs);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    // Scope the undo item recording so we can undo on failure.
    {
        OpUndoItemRecorder undoRecorder(_undoItemList);

        auto& manager = PXR_NS::PrimUpdaterManager::getInstance();
        auto  dstUfePaths = srcPath.empty()
            ? manager.duplicateToUsd(srcMayaObject, dstPath, userArgs)
            : manager.duplicate(srcPath, dstPath, userArgs);

        if (dstUfePaths.size() > 0) {
            // Select the duplicate.
            //
            // If the duplicate src is Maya, the duplicate child of the
            // destination will be USD, and vice-versa.  Construct an
            // appropriate child path:
            // - Maya duplicate to USD: we always duplicate directly under the
            //   proxy shape, so add a single path component USD path segment.
            // - USD duplicate to Maya: no path segment to add.
            const Ufe::Path& childPath = dstUfePaths[0];
            auto             childItem = Ufe::Hierarchy::createItem(childPath);
            if (!childItem)
                return MS::kFailure;
            Ufe::Selection sn;
            sn.append(childItem);
            // It is appropriate to use the overload that uses the global list,
            // as the undo recorder will transfer the items on the global list
            // to fUndoItemList.
            if (!UfeSelectionUndoItem::select("duplicate: select duplicate", sn)) {
                return MS::kFailure;
            }
        }
    }

    // Undo potentially partially-made duplicate on failure.
    if (status != MS::kSuccess)
        _undoItemList.undo();

    return status;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
