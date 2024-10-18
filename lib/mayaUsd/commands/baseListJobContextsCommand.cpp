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
#include "baseListJobContextsCommand.h"

#include <mayaUsd/fileio/jobContextRegistry.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/utils/util.h>

#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MSyntax.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

namespace {

const UsdMayaJobContextRegistry::ContextInfo&
_GetInfo(const MArgDatabase& argData, const char* optionName)
{
    static const UsdMayaJobContextRegistry::ContextInfo emptyInfo;
    MString                                             contextName;
    MStatus status = argData.getFlagArgument(optionName, 0, contextName);
    if (status != MS::kSuccess) {
        return emptyInfo;
    }
    for (auto const& c : UsdMayaJobContextRegistry::ListJobContexts()) {
        auto const& info = UsdMayaJobContextRegistry::GetJobContextInfo(c);
        if (info.niceName == contextName.asChar()) {
            return info;
        }
    }
    return emptyInfo;
}

MString convertDictionaryToText(const VtDictionary& settings)
{
    // Would be nice to return a Python dictionary, but we need something MEL-compatible
    // Use the JobContextRegistry Python wrappers to get a dictionary.
    std::ostringstream optionsStream;
    for (const std::pair<std::string, VtValue> keyValue : settings) {

        bool        canConvert;
        std::string valueStr;
        std::tie(canConvert, valueStr) = UsdMayaUtil::ValueToArgument(keyValue.second);
        // Options don't handle empty arrays well preventing users from passing actual
        // values for options with such default value.
        if (canConvert && valueStr != "[]") {
            optionsStream << keyValue.first.c_str() << "=" << valueStr.c_str() << ";";
        }
    }
    return optionsStream.str().c_str();
}

const char* kEexportStr = "export";
const char* kExportAnnotationStr = "exportAnnotation";
const char* kExportArgumentsStr = "exportArguments";
const char* kHasExportUIStr = "hasExportUI";
const char* kShowExportUIStr = "showExportUI";
const char* kHasImportUIStr = "hasImportUI";
const char* kShowImportUIStr = "showImportUI";
const char* kImportStr = "import";
const char* kImportAnnotationStr = "importAnnotation";
const char* kImportArgumentsStr = "importArguments";
const char* kJobContextStr = "jobContext";
} // namespace

MStatus MayaUSDListJobContextsCommand::doIt(const MArgList& args)
{
    MStatus      status;
    MArgDatabase argData(syntax(), args, &status);

    if (status != MS::kSuccess) {
        return status;
    }

    if (argData.isFlagSet(kEexportStr)) {
        for (auto const& c : UsdMayaJobContextRegistry::ListJobContexts()) {
            auto const& info = UsdMayaJobContextRegistry::GetJobContextInfo(c);
            if (info.exportEnablerCallback) {
                appendToResult(info.niceName.GetText());
            }
        }
    } else if (argData.isFlagSet(kExportAnnotationStr)) {
        auto const& info = _GetInfo(argData, kExportAnnotationStr);
        if (!info.jobContext.IsEmpty()) {
            setResult(info.exportDescription.GetText());
        }
    } else if (argData.isFlagSet(kExportArgumentsStr)) {
        auto const& info = _GetInfo(argData, kExportArgumentsStr);
        if (info.exportEnablerCallback) {
            setResult(convertDictionaryToText(info.exportEnablerCallback()));
        }
    } else if (argData.isFlagSet(kHasExportUIStr)) {
        auto const& info = _GetInfo(argData, kHasExportUIStr);
        setResult(bool(info.exportUICallback != nullptr));
    } else if (argData.isFlagSet(kShowExportUIStr)) {
        auto const& info = _GetInfo(argData, kShowExportUIStr);
        if (!info.exportUICallback)
            return MS::kInvalidParameter;

        MString parentUIStr;
        if (argData.getFlagArgument(kShowExportUIStr, 1, parentUIStr) != MS::kSuccess)
            return MS::kInvalidParameter;

        MString settingsStr;
        if (argData.getFlagArgument(kShowExportUIStr, 2, settingsStr) != MS::kSuccess)
            return MS::kInvalidParameter;

        VtDictionary inputSettings;
        if (UsdMayaJobExportArgs::GetDictionaryFromEncodedOptions(settingsStr, &inputSettings)
            != MS::kSuccess)
            return MS::kInvalidParameter;

        setResult(convertDictionaryToText(
            info.exportUICallback(info.jobContext, parentUIStr.asChar(), inputSettings)));
    } else if (argData.isFlagSet(kHasImportUIStr)) {
        auto const& info = _GetInfo(argData, kHasImportUIStr);
        setResult(bool(info.importUICallback != nullptr));
    } else if (argData.isFlagSet(kShowImportUIStr)) {
        auto const& info = _GetInfo(argData, kShowImportUIStr);
        if (!info.importUICallback)
            return MS::kInvalidParameter;

        MString parentUIStr;
        if (argData.getFlagArgument(kShowImportUIStr, 1, parentUIStr) != MS::kSuccess)
            return MS::kInvalidParameter;

        MString settingsStr;
        if (argData.getFlagArgument(kShowImportUIStr, 2, settingsStr) != MS::kSuccess)
            return MS::kInvalidParameter;

        VtDictionary inputSettings;
        if (UsdMayaJobImportArgs::GetDictionaryFromEncodedOptions(settingsStr, &inputSettings)
            != MS::kSuccess)
            return MS::kInvalidParameter;

        setResult(convertDictionaryToText(
            info.importUICallback(info.jobContext, parentUIStr.asChar(), inputSettings)));
    } else if (argData.isFlagSet(kImportStr)) {
        for (auto const& c : UsdMayaJobContextRegistry::ListJobContexts()) {
            auto const& info = UsdMayaJobContextRegistry::GetJobContextInfo(c);
            if (info.importEnablerCallback) {
                appendToResult(info.niceName.GetText());
            }
        }
    } else if (argData.isFlagSet(kImportAnnotationStr)) {
        auto const& info = _GetInfo(argData, kImportAnnotationStr);
        if (!info.jobContext.IsEmpty()) {
            setResult(info.importDescription.GetText());
        }
    } else if (argData.isFlagSet(kImportArgumentsStr)) {
        auto const& info = _GetInfo(argData, kImportArgumentsStr);
        if (info.importEnablerCallback) {
            setResult(convertDictionaryToText(info.importEnablerCallback()));
        }
    } else if (argData.isFlagSet(kJobContextStr)) {
        auto const& info = _GetInfo(argData, kJobContextStr);
        if (!info.jobContext.IsEmpty()) {
            setResult(info.jobContext.GetText());
        }
    }

    return MS::kSuccess;
}

MSyntax MayaUSDListJobContextsCommand::createSyntax()
{
    MSyntax syntax;
    syntax.addFlag("-ex", "-export", MSyntax::kNoArg);
    syntax.addFlag("-ea", "-exportAnnotation", MSyntax::kString);
    syntax.addFlag("-eg", "-exportArguments", MSyntax::kString);
    syntax.addFlag("-heu", "-hasExportUI", MSyntax::kString);
    syntax.addFlag("-seu", "-showExportUI", MSyntax::kString, MSyntax::kString, MSyntax::kString);
    syntax.addFlag("-hiu", "-hasImportUI", MSyntax::kString);
    syntax.addFlag("-siu", "-showImportUI", MSyntax::kString, MSyntax::kString, MSyntax::kString);
    syntax.addFlag("-im", "-import", MSyntax::kNoArg);
    syntax.addFlag("-ia", "-importAnnotation", MSyntax::kString);
    syntax.addFlag("-ig", "-importArguments", MSyntax::kString);
    syntax.addFlag("-jc", "-jobContext", MSyntax::kString);

    syntax.enableQuery(false);
    syntax.enableEdit(false);

    return syntax;
}

void* MayaUSDListJobContextsCommand::creator() { return new MayaUSDListJobContextsCommand(); }

} // namespace MAYAUSD_NS_DEF
