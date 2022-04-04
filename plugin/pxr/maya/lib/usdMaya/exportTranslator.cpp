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
#include "usdMaya/exportTranslator.h"

#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/jobs/writeJob.h>

#include <maya/MFileObject.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

void* UsdMayaExportTranslator::creator() { return new UsdMayaExportTranslator(); }

UsdMayaExportTranslator::UsdMayaExportTranslator()
    : MPxFileTranslator()
{
}

UsdMayaExportTranslator::~UsdMayaExportTranslator() { }

MStatus UsdMayaExportTranslator::writer(
    const MFileObject&                file,
    const MString&                    optionsString,
    MPxFileTranslator::FileAccessMode mode)
{

    std::string fileName(file.fullName().asChar());

    MSelectionList objSelList;
    if (mode == MPxFileTranslator::kExportActiveAccessMode) {
        // Get selected objects
        MGlobal::getActiveSelectionList(objSelList);
    } else if (mode == MPxFileTranslator::kExportAccessMode) {
        // Get all objects at DAG root
        objSelList.add("|*", true);
    }

    // Convert selection list to jobArgs dagPaths
    UsdMayaUtil::MDagPathSet dagPaths;
    for (unsigned int i = 0; i < objSelList.length(); i++) {
        MDagPath dagPath;
        if (objSelList.getDagPath(i, dagPath) == MS::kSuccess) {
            dagPaths.insert(dagPath);
        }
    }

    if (dagPaths.empty()) {
        TF_WARN("No DAG nodes to export. Skipping.");
        return MS::kSuccess;
    }

    VtDictionary userArgs;
    MStatus      status
        = UsdMayaJobExportArgs::GetDictionaryFromEncodedOptions(optionsString, &userArgs);
    if (status != MS::kSuccess)
        return status;

    std::vector<double> timeSamples;
    UsdMayaJobExportArgs::GetDictionaryTimeSamples(userArgs, timeSamples);

    auto jobArgs = UsdMayaJobExportArgs::CreateFromDictionary(userArgs, dagPaths, timeSamples);
    bool append = false;

    UsdMaya_WriteJob writeJob(jobArgs);
    if (!writeJob.Write(fileName, append)) {
        return MS::kFailure;
    }

    return MS::kSuccess;
}

MPxFileTranslator::MFileKind UsdMayaExportTranslator::identifyFile(
    const MFileObject& file,
    const char* /*buffer*/,
    short /*size*/) const
{
    MFileKind     retValue = kNotMyFileType;
    const MString fileName = file.fullName();
    const int     lastIndex = fileName.length() - 1;

    const int periodIndex = fileName.rindex('.');
    if (periodIndex < 0 || periodIndex >= lastIndex) {
        return retValue;
    }

    const MString fileExtension = fileName.substring(periodIndex + 1, lastIndex);

    if (fileExtension == UsdMayaTranslatorTokens->UsdFileExtensionDefault.GetText()
        || fileExtension == UsdMayaTranslatorTokens->UsdFileExtensionASCII.GetText()
        || fileExtension == UsdMayaTranslatorTokens->UsdFileExtensionCrate.GetText()
        || fileExtension == UsdMayaTranslatorTokens->UsdFileExtensionPackage.GetText()) {
        retValue = kIsMyFileType;
    }

    return retValue;
}

/* static */
const std::string& UsdMayaExportTranslator::GetDefaultOptions()
{
    static std::string    defaultOptions;
    static std::once_flag once;
    std::call_once(once, []() {
        std::vector<std::string> entries;
        for (const std::pair<std::string, VtValue> keyValue :
             UsdMayaJobExportArgs::GetDefaultDictionary()) {
            bool        canConvert;
            std::string valueStr;
            std::tie(canConvert, valueStr) = UsdMayaUtil::ValueToArgument(keyValue.second);
            // Options don't handle empty arrays well preventing users from passing actual
            // values for options with such default value.
            if (canConvert && valueStr != "[]") {
                entries.push_back(
                    TfStringPrintf("%s=%s", keyValue.first.c_str(), valueStr.c_str()));
            }
        }
        entries.push_back("animation=0");
        entries.push_back("startTime=1");
        entries.push_back("endTime=1");
        entries.push_back("frameStride=1.0");
        defaultOptions = TfStringJoin(entries, ";");
    });

    return defaultOptions;
}

PXR_NAMESPACE_CLOSE_SCOPE
