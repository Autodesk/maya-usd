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
#include "usdMaya/listShadingModesCommand.h"

#include <mayaUsd/fileio/registryHelper.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>

#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MSyntax.h>

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

UsdMayaListShadingModesCommand::UsdMayaListShadingModesCommand() {

}

UsdMayaListShadingModesCommand::~UsdMayaListShadingModesCommand() {

}

MStatus
UsdMayaListShadingModesCommand::doIt(const MArgList& args) {    
    MStatus status;
    MArgDatabase argData(syntax(), args, &status);

    if (status != MS::kSuccess) {
        return status;
    }

    TfTokenVector v;
    if (argData.isFlagSet("export")) {
        v = UsdMayaShadingModeRegistry::ListExporters();
    } else if (argData.isFlagSet("import")) {
        v = UsdMayaShadingModeRegistry::ListImporters();
    }

    // Always include the "none" shading mode.
    appendToResult(UsdMayaShadingModeTokens->none.GetText());

    for (const auto& e : v) {
        appendToResult(e.GetText());
    }

    return MS::kSuccess;
}

MSyntax
UsdMayaListShadingModesCommand::createSyntax() {
    MSyntax syntax;
    syntax.addFlag("-ex", "-export", MSyntax::kNoArg);
    syntax.addFlag("-im", "-import", MSyntax::kNoArg);

    syntax.enableQuery(false);
    syntax.enableEdit(false);

    return syntax;
}

void* UsdMayaListShadingModesCommand::creator() {
    return new UsdMayaListShadingModesCommand();
}

PXR_NAMESPACE_CLOSE_SCOPE
