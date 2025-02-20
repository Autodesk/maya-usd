//
// Copyright 2024 Autodesk
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or dataied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "schemaCommand.h"

#include <mayaUsd/ufe/Utils.h>

#include <usdUfe/undo/UsdUndoBlock.h>
#include <usdUfe/undo/UsdUndoableItem.h>
#include <usdUfe/utils/schemas.h>

#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MGlobal.h>
#include <maya/MStringArray.h>
#include <maya/MSyntax.h>
#include <ufe/path.h>
#include <ufe/pathString.h>

namespace MAYAUSD_NS_DEF {

////////////////////////////////////////////////////////////////////////////
//
// Error message formatting.

static MString formatMessage(const char* format, const std::string& text)
{
    return PXR_NS::TfStringPrintf(format, text.c_str()).c_str();
}

static MString formatMessage(
    const char*        format,
    const char*        action,
    PXR_NS::UsdPrim&   prim,
    const std::string& text)
{
    return PXR_NS::TfStringPrintf(format, action, prim.GetPath().GetString().c_str(), text.c_str())
        .c_str();
}

static std::string formatMessage(const char* format, const Ufe::Path& ufePath)
{
    return PXR_NS::TfStringPrintf(format, Ufe::PathString::string(ufePath).c_str());
}

////////////////////////////////////////////////////////////////////////////
//
// Command name and flags.

const char SchemaCommand::commandName[] = "mayaUsdSchema";

static const char kAppliedSchemasFlag[] = "app";
static const char kAppliedSchemasLongFlag[] = "appliedSchemas";
static const char kSchemaFlag[] = "sch";
static const char kSchemaLongFlag[] = "schema";
static const char kInstanceNameFlag[] = "in";
static const char kInstanceNameLongFlag[] = "instanceName";
static const char kRemoveSchemaFlag[] = "rem";
static const char kRemoveSchemaLongFlag[] = "removeSchema";

static const char kSingleApplicationFlag[] = "sas";
static const char kSingleApplicationLongFlag[] = "singleApplicationSchemas";
static const char kMultiApplicationFlag[] = "mas";
static const char kMultiApplicationLongFlag[] = "multiApplicationSchemas";

////////////////////////////////////////////////////////////////////////////
//
// Command data and argument parsing to fill that data.

class SchemaCommand::Data
{
public:
    // Parse the Maya argument list and fill the data with it.
    MStatus parseArgs(const MArgList& argList);

    // Convert the list of UFE paths given to the command to the corresponding USD prims.
    std::vector<PXR_NS::UsdPrim> getPrims() const;

    // Clears the list of UFE paths given to the command.
    // Used to reduce the memory consupmtion once the command has been executed.
    void clearPrimPaths() { _primPaths.clear(); }

    // Retrieve the schema name or schema instance name given to the command.
    const std::string& getSchema() const { return _schema; }
    const std::string& getInstanceName() const { return _instanceName; }

    // Check if the command is removing a schema.
    bool isRemovingSchema() const { return _isRemovingSchema; }

    // Check if the command is a query or which specific type of query.
    bool isQuerying() const { return isQueryingAppliedSchemas() || isQueryingKnownSchemas(); }
    bool isQueryingKnownSchemas() const
    {
        return isQueryingSingleAppSchemas() || isQueryingMultiAppSchemas();
    }
    bool isQueryingAppliedSchemas() const { return _isQueryingAppliedSchemas; }
    bool isQueryingSingleAppSchemas() const { return _singleApplicationSchemas; }
    bool isQueryingMultiAppSchemas() const { return _multiApplicationSchemas; }

    // Undo and redo data and implementation.
    UsdUfe::UsdUndoableItem& getUsdUndoItem() { return _undoData; }
    void                     undo() { _undoData.undo(); }
    void                     redo() { _undoData.redo(); }

private:
    void        parsePrimPaths(MArgDatabase& argData);
    std::string parseStringArg(MArgDatabase& argData, const char* argFlag);

    std::vector<Ufe::Path> _primPaths;
    bool                   _isRemovingSchema { false };
    bool                   _isQueryingAppliedSchemas { false };
    bool                   _singleApplicationSchemas { false };
    bool                   _multiApplicationSchemas { false };
    std::string            _schema;
    std::string            _instanceName;

    UsdUfe::UsdUndoableItem _undoData;
};

MStatus SchemaCommand::Data::parseArgs(const MArgList& argList)
{
    MStatus      status;
    MArgDatabase argData(SchemaCommand::createSyntax(), argList, &status);
    if (!status)
        return status;

    _isQueryingAppliedSchemas = argData.isFlagSet(kAppliedSchemasFlag);
    _schema = parseStringArg(argData, kSchemaFlag);
    _instanceName = parseStringArg(argData, kInstanceNameFlag);
    _isRemovingSchema = argData.isFlagSet(kRemoveSchemaFlag);
    _singleApplicationSchemas = argData.isFlagSet(kSingleApplicationFlag);
    _multiApplicationSchemas = argData.isFlagSet(kMultiApplicationFlag);

    parsePrimPaths(argData);

    return status;
}

std::string SchemaCommand::Data::parseStringArg(MArgDatabase& argData, const char* argFlag)
{
    if (!argData.isFlagSet(argFlag))
        return {};

    MString stringVal;
    argData.getFlagArgument(argFlag, 0, stringVal);
    return stringVal.asChar();
}

void SchemaCommand::Data::parsePrimPaths(MArgDatabase& argData)
{
    _primPaths.clear();

    MStringArray ufePathArray;
    argData.getObjects(ufePathArray);
    const unsigned int pathCount = ufePathArray.length();
    for (unsigned int index = 0; index < pathCount; ++index) {
        const std::string arg = ufePathArray[index].asChar();
        if (arg.size() <= 0)
            continue;
        _primPaths.push_back(Ufe::PathString::path(arg));
    }
}

std::vector<PXR_NS::UsdPrim> SchemaCommand::Data::getPrims() const
{
    std::vector<PXR_NS::UsdPrim> prims;

    for (const Ufe::Path& ufePath : _primPaths) {
        PXR_NS::UsdPrim prim = ufe::ufePathToPrim(ufePath);
        if (!prim)
            throw std::runtime_error(formatMessage("Prim path \"%s\" is invalid", ufePath));
        prims.push_back(prim);
    }

    return prims;
}

////////////////////////////////////////////////////////////////////////////
//
// Command creation and syntax.

void* SchemaCommand::creator() { return static_cast<MPxCommand*>(new SchemaCommand()); }

SchemaCommand::SchemaCommand()
    : _data(std::make_unique<SchemaCommand::Data>())
{
}

MSyntax SchemaCommand::createSyntax()
{
    MSyntax syntax;

    syntax.setObjectType(MSyntax::kStringObjects);

    syntax.addFlag(kAppliedSchemasFlag, kAppliedSchemasLongFlag);

    syntax.addFlag(kSchemaFlag, kSchemaLongFlag, MSyntax::kString);
    syntax.addFlag(kInstanceNameFlag, kInstanceNameLongFlag, MSyntax::kString);

    syntax.addFlag(kRemoveSchemaFlag, kRemoveSchemaLongFlag);

    syntax.addFlag(kSingleApplicationFlag, kSingleApplicationLongFlag);
    syntax.addFlag(kMultiApplicationFlag, kMultiApplicationLongFlag);

    return syntax;
}

bool SchemaCommand::isUndoable() const { return !_data->isQuerying(); }

////////////////////////////////////////////////////////////////////////////
//
// Command execution.

MStatus SchemaCommand::handleAppliedSchemas()
{
    std::set<PXR_NS::TfToken> allSchemas = UsdUfe::getPrimsAppliedSchemas(_data->getPrims());

    MStringArray results;
    for (const PXR_NS::TfToken& schema : allSchemas)
        results.append(schema.GetString().c_str());
    setResult(results);

    return MS::kSuccess;
}

MStatus SchemaCommand::handleKnownSchemas()
{
    const UsdUfe::KnownSchemas knownSchemas = UsdUfe::getKnownApplicableSchemas();

    for (const auto& schema : knownSchemas) {
        const bool shouldAppend = schema.second.isMultiApply ? _data->isQueryingMultiAppSchemas()
                                                             : _data->isQueryingSingleAppSchemas();
        if (shouldAppend)
            appendToResult(schema.second.schemaTypeName.GetString().c_str());
    }

    return MS::kSuccess;
}

MStatus SchemaCommand::handleApplyOrRemoveSchema()
{
    UsdUfe::UsdUndoBlock undoBlock(&_data->getUsdUndoItem());

    const std::string& schemaName = _data->getSchema();
    if (schemaName.empty()) {
        displayError("No schema given to modify the prims");
        return MS::kInvalidParameter;
    }

    auto maybeInfo = UsdUfe::findSchemasByTypeName(PXR_NS::TfToken(schemaName));
    if (!maybeInfo) {
        displayError(formatMessage("Cannot find the schema for the type named \"%s\"", schemaName));
        return MS::kInvalidParameter;
    }

    const PXR_NS::TfType& schemaType = maybeInfo->schemaType;

    if (maybeInfo->isMultiApply) {
        if (_data->getInstanceName().empty()) {
            displayError(formatMessage(
                "No schema instance name given for the \"%s\" multi-apply schema", schemaName));
            return MS::kInvalidParameter;
        }

        PXR_NS::TfToken instanceName(_data->getInstanceName());

        auto func = _data->isRemovingSchema() ? &UsdUfe::removeMultiSchemaFromPrim
                                              : &UsdUfe::applyMultiSchemaToPrim;
        const char* action = _data->isRemovingSchema() ? "remove" : "apply";

        for (PXR_NS::UsdPrim& prim : _data->getPrims()) {
            if (!func(prim, schemaType, instanceName)) {
                displayWarning(formatMessage(
                    "Could not %s schema \"%s\" to prim \"%s\"", action, prim, schemaName));
            }
        }
    } else {
        auto func = _data->isRemovingSchema() ? &UsdUfe::removeSchemaFromPrim
                                              : &UsdUfe::applySchemaToPrim;
        const char* action = _data->isRemovingSchema() ? "remove" : "apply";

        for (PXR_NS::UsdPrim& prim : _data->getPrims()) {
            if (!func(prim, schemaType)) {
                displayWarning(formatMessage(
                    "Could not %s schema \"%s\" to prim \"%s\"", action, prim, schemaName));
            }
        }
    }

    _data->clearPrimPaths();

    return MS::kSuccess;
}

MStatus SchemaCommand::doIt(const MArgList& argList)
{
    try {
        setCommandString(commandName);
        clearResult();

        MStatus status = _data->parseArgs(argList);
        if (!status)
            return status;

        if (_data->isQueryingAppliedSchemas())
            return handleAppliedSchemas();

        if (_data->isQueryingKnownSchemas())
            return handleKnownSchemas();

        return handleApplyOrRemoveSchema();
    } catch (const std::exception& exc) {
        displayError(exc.what());
        return MS::kFailure;
    }
}

MStatus SchemaCommand::redoIt()
{
    _data->redo();
    return MS::kSuccess;
}

MStatus SchemaCommand::undoIt()
{
    _data->undo();
    return MS::kSuccess;
}

} //  namespace MAYAUSD_NS_DEF
