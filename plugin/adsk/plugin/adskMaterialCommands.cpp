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

#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/utils/Utils.h>
#ifdef UFE_V4_FEATURES_AVAILABLE
#include <mayaUsd/ufe/UsdShaderNodeDef.h>
#endif
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/query.h>
#include <mayaUsd/utils/util.h>

#include <pxr/usd/sdr/registry.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MArgParser.h>
#include <maya/MFnDagNode.h>
#include <maya/MGlobal.h>
#include <maya/MStringArray.h>
#include <maya/MSyntax.h>
#include <ufe/hierarchy.h>
#include <ufe/path.h>
#include <ufe/pathString.h>

#include <algorithm>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

const MString
              ADSKMayaUSDGetMaterialsForRenderersCommand::commandName("mayaUsdGetMaterialsFromRenderers");
const MString ADSKMayaUSDGetMaterialsInStageCommand::commandName("mayaUsdGetMaterialsInStage");
const MString ADSKMayaUSDMaterialBindingsCommand::commandName("mayaUsdMaterialBindings");

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

#ifndef UFE_V4_FEATURES_AVAILABLE
void ADSKMayaUSDGetMaterialsForRenderersCommand::appendMaterialXMaterials() const
{
    // TODO: Replace hard-coded materials with dynamically generated list.
    static const std::vector<std::pair<std::string, std::string>> vettedSurfaces
        = { { "ND_standard_surface_surfaceshader", "Standard Surface" },
            { "ND_gltf_pbr_surfaceshader", "glTF PBR" },
            { "ND_UsdPreviewSurface_surfaceshader", "USD Preview Surface" },
            { "ND_open_pbr_surface_surfaceshader", "OpenPBR Surface" } };
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
    auto& sdrRegistry = PXR_NS::SdrRegistry::GetInstance();
#if PXR_VERSION >= 2505
    const auto sourceTypes = sdrRegistry.GetAllShaderNodeSourceTypes();
#else
    const auto sourceTypes = sdrRegistry.GetAllNodeSourceTypes();
#endif
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
#endif

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

#ifdef UFE_V4_FEATURES_AVAILABLE
    const auto shaderNodeDefs = UsdMayaUtil::GetSurfaceShaderNodeDefs();
    for (const auto& nodeDef : shaderNodeDefs) {
        // To make use of ufe classifications
        auto ufeNodeDef = ufe::UsdShaderNodeDef::create(nodeDef);
        auto familyName = ufeNodeDef->classification(0);
        auto sourceType = ufeNodeDef->classification(ufeNodeDef->nbClassifications() - 1);
        appendToResult(MString(TfStringPrintf(
                                   "%s/%s|%s",
                                   sourceType.c_str(),
                                   UsdUfe::prettifyName(familyName).c_str(),
                                   nodeDef->GetIdentifier().GetText())
                                   .c_str()));
    }
#else
    appendUsdMaterials();
    appendArnoldMaterials();
    appendMaterialXMaterials();
#endif

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

/*
// ADSKMayaUSDMaterialBindingsCommand
*/

// plug-in callback to create the command object
void* ADSKMayaUSDMaterialBindingsCommand::creator()
{
    return static_cast<MPxCommand*>(new ADSKMayaUSDMaterialBindingsCommand());
}

// private argument parsing helper
MStatus ADSKMayaUSDMaterialBindingsCommand::parseArgs(const MArgList& argList)
{
    return MS::kSuccess;
}

static bool isNodeTypeInList(
    const Ufe::SceneItem::Ptr&      sceneItem,
    const std::vector<std::string>& nodeTypeList,
    bool                            checkAllAncestors)
{
    const auto canonicalName
        = TfType::Find<UsdSchemaBase>().FindDerivedByName(sceneItem->nodeType().c_str());

    if (canonicalName.IsUnknown()) {
        return false;
    }

    if (std::find(nodeTypeList.begin(), nodeTypeList.end(), canonicalName.GetTypeName().c_str())
        != nodeTypeList.end()) {
        // Our nodeType matches one in the given list.
        return true;
    }

    const auto& ancestors = sceneItem->ancestorNodeTypes();
    for (const auto& ancestorType : ancestors) {
        const auto canonicalAncestorName
            = TfType::Find<UsdSchemaBase>().FindDerivedByName(ancestorType.c_str());

        // Make sure we see at least one actual ancestor: For some types the first reported
        // ancestor is the same as the node type itself.
        if (canonicalAncestorName == canonicalName) {
            continue;
        }

        // Check whether an ancestor of our node matches one of the listed node types.
        if (std::find(
                nodeTypeList.begin(),
                nodeTypeList.end(),
                canonicalAncestorName.GetTypeName().c_str())
            != nodeTypeList.end()) {
            return true;
        }

        // Do we only care about the immediate parent or all ancestors?
        if (!checkAllAncestors)
            return false;
    }

    return false;
}

// main MPxCommand execution point
MStatus ADSKMayaUSDMaterialBindingsCommand::doIt(const MArgList& argList)
{
    clearResult();

    MStatus      status;
    MArgParser   parser(syntax(), argList);
    MArgDatabase args(syntax(), argList, &status);
    if (!status)
        return status;

    MString ufePathString = args.commandArgumentString(0);
    if (ufePathString.length() == 0) {
        MGlobal::displayError("Missing argument 'UFE Path'.");
        throw MS::kFailure;
    }

    const auto                ufePath = Ufe::PathString::path(ufePathString.asChar());
    const Ufe::SceneItem::Ptr sceneItem = Ufe::Hierarchy::createItem(ufePath);
    if (!sceneItem) {
        MGlobal::displayError("Could not find SceneItem:" + ufePathString);
        throw MS::kFailure;
    }

    if (parser.isFlagSet(kHasMaterialBindingFlag)) {
        auto usdSceneItem = UsdUfe::downcast(sceneItem);
        if (!usdSceneItem) {
            MGlobal::displayError("Invalid SceneItem:" + ufePathString);
            throw MS::kFailure;
        }
        const auto prim = usdSceneItem->prim();
        bool       hasMaterialBindingAPI = prim.HasAPI<UsdShadeMaterialBindingAPI>();
        if (!hasMaterialBindingAPI) {
            setResult(false);
        } else {
            auto bindingAPI = UsdShadeMaterialBindingAPI(prim);
            auto materialPath = bindingAPI.GetDirectBinding().GetMaterialPath();
            setResult(!materialPath.IsEmpty());
        }
    } else if (parser.isFlagSet(kCanAssignMaterialToNodeType)) {

        // Nodes or those with ancestors of these types allow material assignment:
        const std::vector<std::string> allowNodeTypes = { "UsdGeomImageable", "UsdGeomSubset" };
        // Unless the node (or an ancestor) is one of these types:
        const std::vector<std::string> rejectNodeTypes = { "MayaUsd_SchemasMayaReference",
                                                           "MayaUsd_SchemasALMayaReference",
                                                           "UsdGeomCamera",
                                                           "UsdMediaSpatialAudio",
                                                           "UsdProcGenerativeProcedural",
                                                           "UsdPhysicsJoint",
                                                           "UsdSkelRoot",
                                                           "UsdSkelSkeleton",
                                                           "UsdVolField3DAsset",
                                                           "UsdVolFieldAsset",
                                                           "UsdVolFieldBase",
                                                           "UsdVolOpenVDBAsset" };

        if (isNodeTypeInList(sceneItem, rejectNodeTypes, false)) {
            setResult(false);
        } else if (isNodeTypeInList(sceneItem, allowNodeTypes, true)) {
            setResult(true);
        } else {
            setResult(false);
        }
    }

    return MS::kSuccess;
}

MSyntax ADSKMayaUSDMaterialBindingsCommand::createSyntax()
{
    MSyntax syntax;
    syntax.addArg(MSyntax::kString);
    syntax.addFlag(kHasMaterialBindingFlag, kHasMaterialBindingFlagLong, MSyntax::kBoolean);
    syntax.addFlag(
        kCanAssignMaterialToNodeType, kCanAssignMaterialToNodeTypeLong, MSyntax::kBoolean);
    syntax.enableQuery(false);
    syntax.enableEdit(false);
    return syntax;
}

} // namespace MAYAUSD_NS_DEF