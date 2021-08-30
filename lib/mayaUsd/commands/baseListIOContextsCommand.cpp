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
#include "baseListIOContextsCommand.h"

#include <mayaUsd/fileio/exportContextRegistry.h>

#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MSyntax.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

MStatus MayaUSDListIOContextsCommand::doIt(const MArgList& args)
{
    MStatus      status;
    MArgDatabase argData(syntax(), args, &status);

    if (status != MS::kSuccess) {
        return status;
    }

    if (argData.isFlagSet("export")) {
        for (auto const& c : UsdMayaExportContextRegistry::ListExportContexts()) {
            auto const& info = UsdMayaExportContextRegistry::GetExportContextInfo(c);
            appendToResult(info.niceName.c_str());
        }
    } else if (argData.isFlagSet("exportOption")) {
        MString contextName;
        status = argData.getFlagArgument("exportOption", 0, contextName);
        if (status != MS::kSuccess) {
            return status;
        }
        for (auto const& c : UsdMayaExportContextRegistry::ListExportContexts()) {
            auto const& info = UsdMayaExportContextRegistry::GetExportContextInfo(c);
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
        for (auto const& c : UsdMayaExportContextRegistry::ListExportContexts()) {
            auto const& info = UsdMayaExportContextRegistry::GetExportContextInfo(c);
            if (info.niceName == contextName.asChar()) {
                setResult(info.description.c_str());
            }
        }
    }

    return MS::kSuccess;
}

MSyntax MayaUSDListIOContextsCommand::createSyntax()
{
    MSyntax syntax;
    syntax.addFlag("-ex", "-export", MSyntax::kNoArg);
    syntax.addFlag("-eo", "-exportOption", MSyntax::kString);
    syntax.addFlag("-ea", "-exportAnnotation", MSyntax::kString);

    syntax.enableQuery(false);
    syntax.enableEdit(false);

    return syntax;
}

void* MayaUSDListIOContextsCommand::creator() { return new MayaUSDListIOContextsCommand(); }

} // namespace MAYAUSD_NS_DEF
