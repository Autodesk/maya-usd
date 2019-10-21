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
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/utils/writeUtil.h>

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

    std::string  fileName(file.fullName().asChar(), file.fullName().length());
    VtDictionary userArgs;
    bool         exportAnimation = false;
    GfInterval   timeInterval(1.0, 1.0);
    double       frameStride = 1.0;
    bool         append = false;

    std::set<double> frameSamples;

    MStringArray filteredTypes;
    // Get the options
    if (optionsString.length() > 0) {
        MStringArray optionList;
        MStringArray theOption;
        optionsString.split(';', optionList);
        for (int i = 0; i < (int)optionList.length(); ++i) {
            theOption.clear();
            optionList[i].split('=', theOption);
            if (theOption.length() != 2) {
                continue;
            }

            std::string argName(theOption[0].asChar());
            if (argName == "animation") {
                exportAnimation = (theOption[1].asInt() != 0);
            } else if (argName == "startTime") {
                timeInterval.SetMin(theOption[1].asDouble());
            } else if (argName == "endTime") {
                timeInterval.SetMax(theOption[1].asDouble());
            } else if (argName == "frameStride") {
                frameStride = theOption[1].asDouble();
            } else if (argName == "filterTypes") {
                theOption[1].split(',', filteredTypes);
            } else if (argName == "frameSample") {
                frameSamples.clear();
                MStringArray samplesStrings;
                theOption[1].split(' ', samplesStrings);
                unsigned int nbSams = samplesStrings.length();
                for (unsigned int sam = 0; sam < nbSams; ++sam) {
                    if (samplesStrings[sam].isDouble()) {
                        frameSamples.insert(samplesStrings[sam].asDouble());
                    }
                }
            } else if (argName == "root") {
                std::string exportRootPath = theOption[1].asChar();
                userArgs[argName] = VtValue(exportRootPath);
                if (!exportRootPath.empty()) {
                    MDagPath rootDagPath;
                    UsdMayaUtil::GetDagPathByName(exportRootPath, rootDagPath);
                    if (!rootDagPath.isValid()) {
                        MGlobal::displayError(
                            MString("Invalid dag path provided for root: ") + theOption[1]);
                        return MS::kFailure;
                    }
                }
            } else {
                if (argName == "shadingMode") {
                    TfToken shadingMode(theOption[1].asChar());
                    if (!shadingMode.IsEmpty()
                        && UsdMayaShadingModeRegistry::GetInstance().GetExporter(shadingMode)
                            == nullptr
                        && shadingMode != UsdMayaShadingModeTokens->none) {

                        MGlobal::displayError(
                            TfStringPrintf("No shadingMode '%s' found.", shadingMode.GetText())
                                .c_str());
                        return MS::kFailure;
                    }
                }
                userArgs[argName] = UsdMayaUtil::ParseArgumentValue(
                    argName,
                    theOption[1].asChar(),
                    PXR_NS::UsdMayaJobExportArgs::GetDefaultDictionary());
            }
        }
    }

    // Now resync start and end frame based on export time interval.
    if (exportAnimation) {
        if (timeInterval.IsEmpty()) {
            // If the user accidentally set start > end, resync to the closed
            // interval with the single start point.
            timeInterval = GfInterval(timeInterval.GetMin());
        }
    } else {
        // No animation, so empty interval.
        timeInterval = GfInterval();
    }

    MSelectionList           objSelList;
    UsdMayaUtil::MDagPathSet dagPaths;
    GetFilteredSelectionToExport(
        (mode == MPxFileTranslator::kExportActiveAccessMode), objSelList, dagPaths);

    if (dagPaths.empty()) {
        TF_WARN("No DAG nodes to export. Skipping.");
        return MS::kSuccess;
    }

    const std::vector<double> timeSamples
        = UsdMayaWriteUtil::GetTimeSamples(timeInterval, frameSamples, frameStride);
    PXR_NS::UsdMayaJobExportArgs jobArgs
        = PXR_NS::UsdMayaJobExportArgs::CreateFromDictionary(userArgs, dagPaths, timeSamples);

    unsigned int len = filteredTypes.length();
    for (unsigned int i = 0; i < len; ++i) {
        jobArgs.AddFilteredTypeName(filteredTypes[i].asChar());
    }

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
            if (canConvert) {
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
