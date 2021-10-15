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
#include <mayaUsd/utils/util.h>

#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MSyntax.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

MStatus MayaUSDListJobContextsCommand::doIt(const MArgList& args)
{
    MStatus      status;
    MArgDatabase argData(syntax(), args, &status);

    if (status != MS::kSuccess) {
        return status;
    }

    if (argData.isFlagSet("export")) {
        for (auto const& c : UsdMayaJobContextRegistry::ListExportJobContexts()) {
            auto const& info = UsdMayaJobContextRegistry::GetJobContextInfo(c);
            appendToResult(info.niceName.c_str());
        }
    } else if (argData.isFlagSet("exportOption")) {
        MString contextName;
        status = argData.getFlagArgument("exportOption", 0, contextName);
        if (status != MS::kSuccess) {
            return status;
        }
        for (auto const& c : UsdMayaJobContextRegistry::ListExportJobContexts()) {
            auto const& info = UsdMayaJobContextRegistry::GetJobContextInfo(c);
            if (info.niceName == contextName.asChar()) {
                setResult(c.GetText());
            }
        }
    } else if (argData.isFlagSet("exportAnnotation")) {
        MString contextName;
        status = argData.getFlagArgument("exportAnnotation", 0, contextName);
        if (status != MS::kSuccess) {
            return status;
        }
        for (auto const& c : UsdMayaJobContextRegistry::ListExportJobContexts()) {
            auto const& info = UsdMayaJobContextRegistry::GetJobContextInfo(c);
            if (info.niceName == contextName.asChar()) {
                setResult(info.exportDescription.c_str());
            }
        }
    } else if (argData.isFlagSet("exportArguments")) {
        MString contextName;
        status = argData.getFlagArgument("exportArguments", 0, contextName);
        if (status != MS::kSuccess) {
            return status;
        }
        for (auto const& c : UsdMayaJobContextRegistry::ListExportJobContexts()) {
            auto const& info = UsdMayaJobContextRegistry::GetJobContextInfo(c);
            if (info.niceName == contextName.asChar()) {
                // Would be nice to return a Python dictionary, but we need something MEL-compatible
                // Use the JobContextRegistry Python wrappers to get a dictionary.
                if (!info.exportEnablerCallback) {
                    continue;
                }
                std::ostringstream optionsStream;
                for (const std::pair<std::string, VtValue> keyValue :
                     info.exportEnablerCallback()) {

                    bool        canConvert;
                    std::string valueStr;
                    std::tie(canConvert, valueStr) = UsdMayaUtil::ValueToArgument(keyValue.second);
                    // Options don't handle empty arrays well preventing users from passing actual
                    // values for options with such default value.
                    if (canConvert && valueStr != "[]") {
                        optionsStream << keyValue.first.c_str() << "=" << valueStr.c_str() << ";";
                    }
                }
                setResult(optionsStream.str().c_str());
            }
        }
    }

    return MS::kSuccess;
}

MSyntax MayaUSDListJobContextsCommand::createSyntax()
{
    MSyntax syntax;
    syntax.addFlag("-ex", "-export", MSyntax::kNoArg);
    syntax.addFlag("-eo", "-exportOption", MSyntax::kString);
    syntax.addFlag("-ea", "-exportAnnotation", MSyntax::kString);
    syntax.addFlag("-eg", "-exportArguments", MSyntax::kString);

    syntax.enableQuery(false);
    syntax.enableEdit(false);

    return syntax;
}

void* MayaUSDListJobContextsCommand::creator() { return new MayaUSDListJobContextsCommand(); }

} // namespace MAYAUSD_NS_DEF
