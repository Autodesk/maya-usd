//
// Copyright 2023 Autodesk
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
#include "adskMaterialCommands.h"

#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/util.h>

#include <pxr/usd/sdr/registry.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MArgParser.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MStringArray.h>
#include <maya/MSyntax.h>
#include <ufe/path.h>
#include <ufe/pathString.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

const MString ADSKMayaUSDGetMaterialsForRenderersCommand::commandName("mayaUsdGetMaterialsFromRenderers");
const MString ADSKMayaUSDGetMaterialsInStageCommand::commandName("mayaUsdGetMaterialsInStage");

/*
// ADSKMayaUSDGetMaterialsForRenderersCommand
*/

// plug-in callback to create the command object
void* ADSKMayaUSDGetMaterialsForRenderersCommand::creator()
{
    return static_cast<MPxCommand*>(new ADSKMayaUSDGetMaterialsForRenderersCommand());
}

// private argument parsing helper
MStatus ADSKMayaUSDGetMaterialsForRenderersCommand::parseArgs(const MArgList& argList)
{
    return MS::kSuccess;
}

void ADSKMayaUSDGetMaterialsForRenderersCommand::appendMaterialXMaterials() const
{
    // TODO: Replace hard-coded materials with dynamically generated list.
    static const std::vector<std::pair<std::string, std::string>> vettedSurfaces
        = { { "ND_standard_surface_surfaceshader", "Standard Surface" },
            { "ND_gltf_pbr_surfaceshader", "glTF PBR" },
            { "ND_UsdPreviewSurface_surfaceshader", "USD Preview Surface" } };
    auto& sdrRegistry = PXR_NS::SdrRegistry::GetInstance();
    for (auto&& info : vettedSurfaces) {
        auto shaderDef = sdrRegistry.GetShaderNodeByIdentifier(TfToken(info.first));
        if (!shaderDef) {
            continue;
        }
        const MString label = info.second.c_str();
        const MString identifier = info.first.c_str();
        appendToResult("MaterialX/" + label + "|" + identifier);
    }
}

void ADSKMayaUSDGetMaterialsForRenderersCommand::appendArnoldMaterials() const
{
    auto&      sdrRegistry = PXR_NS::SdrRegistry::GetInstance();
    const auto sourceTypes = sdrRegistry.GetAllNodeSourceTypes();
    const bool hasArnoldMaterials
        = std::find(sourceTypes.cbegin(), sourceTypes.cend(), TfToken("arnold"))
        != sourceTypes.cend();

    if (hasArnoldMaterials) {
        // TODO: Replace hard-coded materials with dynamically generated list.
        const MString label = "AI Standard Surface";
        const MString identifier = "arnold:standard_surface";
        appendToResult("Arnold/" + label + "|" + identifier);
    }
}

void ADSKMayaUSDGetMaterialsForRenderersCommand::appendUsdMaterials() const
{
    const MString label = "USD Preview Surface";
    const MString identifier = "UsdPreviewSurface";
    appendToResult("USD/" + label + "|" + identifier);
}

// main MPxCommand execution point
MStatus ADSKMayaUSDGetMaterialsForRenderersCommand::doIt(const MArgList& argList)
{
    clearResult();

    MStatus status;
    if (!status)
        return status;

    // TODO: The list of returned materials is currently hard-coded and only for select,
    // known renderers. We should populate the material lists dynamically based on what the
    // installed renderers report as supported materials.
    appendUsdMaterials();
    appendArnoldMaterials();
    appendMaterialXMaterials();

    return MS::kSuccess;
}

MSyntax ADSKMayaUSDGetMaterialsForRenderersCommand::createSyntax()
{
    MSyntax syntax;
    return syntax;
}

/*
// ADSKMayaUSDGetMaterialsInStageCommand
*/

static const TfToken materialType("Material");

// plug-in callback to create the command object
void* ADSKMayaUSDGetMaterialsInStageCommand::creator()
{
    return static_cast<MPxCommand*>(new ADSKMayaUSDGetMaterialsInStageCommand());
}

// private argument parsing helper
MStatus ADSKMayaUSDGetMaterialsInStageCommand::parseArgs(const MArgList& argList)
{
    return MS::kSuccess;
}

// main MPxCommand execution point
MStatus ADSKMayaUSDGetMaterialsInStageCommand::doIt(const MArgList& argList)
{
    clearResult();

    MStatus      status;
    MArgDatabase args(syntax(), argList, &status);
    if (!status)
        return status;

    MString ufePathString = args.commandArgumentString(0);
    if (ufePathString.length() == 0) {
        MGlobal::displayError("Missing argument 'UFE Path'.");
        throw MS::kFailure;
    }

    const auto  ufePath = Ufe::PathString::path(ufePathString.asChar());
    UsdStagePtr stage = ufe::getStage(ufePath);
    if (stage) {
        for (auto prim : stage->Traverse()) {
            if (UsdShadeMaterial(prim)) {
                appendToResult(MString(prim.GetPath().GetString().c_str()));
            }
        }
    }

    return MS::kSuccess;
}

MSyntax ADSKMayaUSDGetMaterialsInStageCommand::createSyntax()
{
    MSyntax syntax;
    syntax.addArg(MSyntax::kString);
    syntax.enableQuery(false);
    syntax.enableEdit(false);
    return syntax;
}

} // namespace MAYAUSD_NS_DEF