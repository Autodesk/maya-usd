//
// Copyright 2016 Pixar
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
#include "exportTranslator.h"

#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/jobs/writeJob.h>

#include <maya/MFileObject.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>

#include <set>
#include <sstream>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

const MString UsdMayaExportTranslator::translatorName("USD Export");

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
    // If we are in neither of these modes then there won't be anything to do
    if (mode != MPxFileTranslator::kExportActiveAccessMode
        && mode != MPxFileTranslator::kExportAccessMode) {
        return MS::kSuccess;
    }

    std::string fileName(file.fullName().asChar(), file.fullName().length());

    MSelectionList           objSelList;
    UsdMayaUtil::MDagPathSet dagPaths;
    GetFilteredSelectionToExport(
        (mode == MPxFileTranslator::kExportActiveAccessMode), objSelList, dagPaths);

    if (dagPaths.empty()) {
        TF_WARN("No DAG nodes to export. Skipping.");
        return MS::kSuccess;
    }

    VtDictionary        userArgs;
    std::vector<double> timeSamples;
    MStatus             status = UsdMayaJobExportArgs::GetDictionaryFromEncodedOptions(
        optionsString, &userArgs, &timeSamples);
    if (status != MS::kSuccess)
        return status;

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
        std::ostringstream optionsStream;
        for (const std::pair<std::string, VtValue> keyValue :
             PXR_NS::UsdMayaJobExportArgs::GetDefaultDictionary()) {

            bool        canConvert;
            std::string valueStr;
            std::tie(canConvert, valueStr) = UsdMayaUtil::ValueToArgument(keyValue.second);
            // Options don't handle empty arrays well preventing users from passing actual
            // values for options with such default value.
            if (canConvert && valueStr != "[]") {
                optionsStream << keyValue.first.c_str() << "=" << valueStr.c_str() << ";";
            }
        }
        optionsStream << "animation=0;";
        optionsStream << "startTime=1;";
        optionsStream << "endTime=1;";
        optionsStream << "frameStride=1.0;";
        optionsStream << "frameSample=0.0;";

        defaultOptions = optionsStream.str();
    });

    return defaultOptions;
}

} // namespace MAYAUSD_NS_DEF
