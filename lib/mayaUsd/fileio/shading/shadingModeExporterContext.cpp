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
#include "shadingModeExporterContext.h"

#include "pxr/usd/usd/specializes.h"

#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/fileio/translators/translatorUtil.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/envSetting.h>
#include <pxr/base/tf/iterator.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/types.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdGeom/subset.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdUtils/pipeline.h>

#include <maya/MDGContext.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MNamespace.h>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <string>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (surfaceShader)
    (volumeShader)
    (displacementShader)
    (varname)
    (map1)
);
// clang-format on

UsdMayaShadingModeExportContext::UsdMayaShadingModeExportContext(
    const MObject&                           shadingEngine,
    UsdMayaWriteJobContext&                  writeJobContext,
    const UsdMayaUtil::MDagPathMap<SdfPath>& dagPathToUsdMap)
    : _shadingEngine(shadingEngine)
    , _stage(writeJobContext.GetUsdStage())
    , _dagPathToUsdMap(dagPathToUsdMap)
    , _writeJobContext(writeJobContext)
    , _surfaceShaderPlugName(_tokens->surfaceShader)
    , _volumeShaderPlugName(_tokens->volumeShader)
    , _displacementShaderPlugName(_tokens->displacementShader)
{
    if (GetExportArgs().dagPaths.empty()) {
        // if none specified, push back '/' which encompasses all
        _bindableRoots.insert(SdfPath::AbsoluteRootPath());
    } else {
        TF_FOR_ALL(bindableRootIter, GetExportArgs().dagPaths)
        {
            const MDagPath& bindableRootDagPath = *bindableRootIter;

            auto iter = _dagPathToUsdMap.find(bindableRootDagPath);
            if (iter == _dagPathToUsdMap.end()) {
                // Geometry w/ this material bound doesn't seem to exist in USD.
                continue;
            }

            SdfPath usdPath = iter->second;

            // If usdModelRootOverridePath is not empty, replace the root
            // namespace with it.
            if (!GetExportArgs().usdModelRootOverridePath.IsEmpty()) {
                usdPath = usdPath.ReplacePrefix(
                    usdPath.GetPrefixes()[0], GetExportArgs().usdModelRootOverridePath);
            }

            _bindableRoots.insert(usdPath);
        }
    }
}

void UsdMayaShadingModeExportContext::SetSurfaceShaderPlugName(const TfToken& surfaceShaderPlugName)
{
    _surfaceShaderPlugName = surfaceShaderPlugName;
}

void UsdMayaShadingModeExportContext::SetVolumeShaderPlugName(const TfToken& volumeShaderPlugName)
{
    _volumeShaderPlugName = volumeShaderPlugName;
}

void UsdMayaShadingModeExportContext::SetDisplacementShaderPlugName(
    const TfToken& displacementShaderPlugName)
{
    _displacementShaderPlugName = displacementShaderPlugName;
}

static MPlug
_GetShaderPlugFromShadingEngine(const MObject& shadingEngine, const TfToken& shaderPlugName)
{
    MStatus status;

    const MFnDependencyNode seDepNodeFn(shadingEngine, &status);
    if (status != MS::kSuccess) {
        return MPlug();
    }

    const MPlug shaderPlug = seDepNodeFn.findPlug(
        shaderPlugName.GetText(),
        /* wantNetworkedPlug = */ true,
        &status);
    if (status != MS::kSuccess) {
        return MPlug();
    }

    return shaderPlug;
}

static MObject
_GetShaderFromShadingEngine(const MObject& shadingEngine, const TfToken& shaderPlugName)
{
    MStatus status;

    const MPlug shaderPlug = _GetShaderPlugFromShadingEngine(shadingEngine, shaderPlugName);
    if (shaderPlug.isNull()) {
        return MObject();
    }

    MObject shaderObj = shaderPlug.asMObject(&status);
    if (status != MS::kSuccess || shaderObj.isNull()) {
        return MObject();
    }

    return UsdMayaUtil::GetConnected(shaderPlug).node();
}

MPlug UsdMayaShadingModeExportContext::GetSurfaceShaderPlug() const
{
    return _GetShaderPlugFromShadingEngine(_shadingEngine, _surfaceShaderPlugName);
}

MObject UsdMayaShadingModeExportContext::GetSurfaceShader() const
{
    return _GetShaderFromShadingEngine(_shadingEngine, _surfaceShaderPlugName);
}

MPlug UsdMayaShadingModeExportContext::GetVolumeShaderPlug() const
{
    return _GetShaderPlugFromShadingEngine(_shadingEngine, _volumeShaderPlugName);
}

MObject UsdMayaShadingModeExportContext::GetVolumeShader() const
{
    return _GetShaderFromShadingEngine(_shadingEngine, _volumeShaderPlugName);
}

MPlug UsdMayaShadingModeExportContext::GetDisplacementShaderPlug() const
{
    return _GetShaderPlugFromShadingEngine(_shadingEngine, _displacementShaderPlugName);
}

MObject UsdMayaShadingModeExportContext::GetDisplacementShader() const
{
    return _GetShaderFromShadingEngine(_shadingEngine, _displacementShaderPlugName);
}

UsdMayaShadingModeExportContext::AssignmentVector
UsdMayaShadingModeExportContext::GetAssignments() const
{
    AssignmentVector ret;

    MStatus           status;
    MFnDependencyNode seDepNode(_shadingEngine, &status);
    if (!status) {
        return ret;
    }

    MPlug dsmPlug = seDepNode.findPlug("dagSetMembers", true, &status);
    if (!status) {
        return ret;
    }

    SdfPathSet seenBoundPrimPaths;
    for (unsigned int i = 0; i < dsmPlug.numConnectedElements(); i++) {
        MPlug   dsmElemPlug(dsmPlug.connectionByPhysicalIndex(i));
        MStatus status = MS::kFailure;
        MPlug   connectedPlug = UsdMayaUtil::GetConnected(dsmElemPlug);

        // Maya connects shader bindings for instances based on element indices
        // of the instObjGroups[x] or instObjGroups[x].objectGroups[y] plugs.
        // The instance number is the index of instObjGroups[x]; the face set
        // (if any) is the index of objectGroups[y].
        if (connectedPlug.isElement() && connectedPlug.array().isChild()) {
            // connectedPlug is instObjGroups[x].objectGroups[y] (or its
            // equivalent), so go up two levels to get to instObjGroups[x].
            MPlug objectGroups = connectedPlug.array();
            MPlug instObjGroupsElem = objectGroups.parent();
            connectedPlug = instObjGroupsElem;
        }
        // connectedPlug should be instObjGroups[x] here. Get the index.
        unsigned int instanceNumber = connectedPlug.logicalIndex();

        // Get the correct DAG path for this instance number.
        MDagPathArray allDagPaths;
        MDagPath::getAllPathsTo(connectedPlug.node(), allDagPaths);
        if (instanceNumber >= allDagPaths.length()) {
            TF_RUNTIME_ERROR(
                "Instance number is %d (from plug '%s') but node only has "
                "%d paths",
                instanceNumber,
                connectedPlug.name().asChar(),
                allDagPaths.length());
            continue;
        }

        MDagPath dagPath = allDagPaths[instanceNumber];
        TF_VERIFY(dagPath.instanceNumber() == instanceNumber);
        MFnDagNode dagNode(dagPath, &status);
        if (!status) {
            continue;
        }

        auto iter = _dagPathToUsdMap.find(dagPath);
        if (iter == _dagPathToUsdMap.end()) {
            // Geometry w/ this material bound doesn't seem to exist in USD.
            continue;
        }
        SdfPath usdPath = iter->second;

        // If usdModelRootOverridePath is not empty, replace the
        // root namespace with it.
        if (!GetExportArgs().usdModelRootOverridePath.IsEmpty()) {
            usdPath = usdPath.ReplacePrefix(
                usdPath.GetPrefixes()[0], GetExportArgs().usdModelRootOverridePath);
        }

        // If this path has already been processed, skip it.
        if (!seenBoundPrimPaths.insert(usdPath).second) {
            continue;
        }

        // If the bound prim's path is not below a bindable root, skip it.
        if (SdfPathFindLongestPrefix(_bindableRoots, usdPath) == _bindableRoots.end()) {
            continue;
        }

        MObjectArray sgObjs, compObjs;
        status = dagNode.getConnectedSetsAndMembers(instanceNumber, sgObjs, compObjs, true);
        if (status != MS::kSuccess) {
            continue;
        }

        for (unsigned int j = 0u; j < sgObjs.length(); ++j) {
            // If the shading group isn't the one we're interested in, skip it.
            if (sgObjs[j] != _shadingEngine) {
                continue;
            }

            VtIntArray faceIndices;
            if (!compObjs[j].isNull()) {
                MItMeshPolygon faceIt(dagPath, compObjs[j]);
                faceIndices.reserve(faceIt.count());
                for (faceIt.reset(); !faceIt.isDone(); faceIt.next()) {
                    faceIndices.push_back(faceIt.index());
                }
            }
            ret.push_back(Assignment { usdPath, faceIndices, TfToken(dagNode.name().asChar()) });
        }
    }
    return ret;
}

static UsdPrim _GetMaterialParent(
    const UsdStageRefPtr&                                    stage,
    const TfToken&                                           materialsScopeName,
    const UsdMayaShadingModeExportContext::AssignmentVector& assignments)
{
    SdfPath commonAncestor;
    TF_FOR_ALL(iter, assignments)
    {
        const SdfPath& assn = iter->boundPrimPath;
        if (stage->GetPrimAtPath(assn)) {
            if (commonAncestor.IsEmpty()) {
                commonAncestor = assn;
            } else {
                commonAncestor = commonAncestor.GetCommonPrefix(assn);
            }
        }
    }

    if (commonAncestor.IsEmpty()) {
        return UsdPrim();
    }

    if (commonAncestor == SdfPath::AbsoluteRootPath()) {
        return stage->GetPseudoRoot();
    }

    SdfPath shaderExportLocation = commonAncestor;
    while (!shaderExportLocation.IsRootPrimPath()) {
        shaderExportLocation = shaderExportLocation.GetParentPath();
    }

    shaderExportLocation = shaderExportLocation.AppendChild(materialsScopeName);

    return UsdGeomScope::Define(stage, shaderExportLocation).GetPrim();
}

/// Determines if the \p path would be an instance proxy path on \p stage if
/// it existed, i.e., if any of its ancestor paths are instances.
/// (Note that if \p path itself is an instance, then it is _not_ an instance
/// proxy path.)
static bool _IsInstanceProxyPath(const UsdStageRefPtr& stage, const SdfPath& path)
{
    for (const SdfPath& prefix : path.GetParentPath().GetPrefixes()) {
        if (const UsdPrim prim = stage->GetPrimAtPath(prefix)) {
            if (prim.IsInstance()) {
                return true;
            }
        }
    }

    return false;
}

/// Ensures that a prim exists at \p path on \p stage and that the prim is
/// neither an instance nor an instance proxy.
static UsdPrim
_UninstancePrim(const UsdStageRefPtr& stage, const SdfPath& path, const std::string& reason)
{
    bool didUninstance = false;
    for (const SdfPath& prefix : path.GetPrefixes()) {
        if (const UsdPrim prim = stage->GetPrimAtPath(prefix)) {
            if (prim.IsInstance()) {
                prim.SetInstanceable(false);
                didUninstance = true;
            }
        } else {
            break;
        }
    }

    if (didUninstance) {
        TF_WARN("Uninstanced <%s> (and ancestors) because: %s", path.GetText(), reason.c_str());
    }

    return stage->OverridePrim(path);
}

UsdPrim UsdMayaShadingModeExportContext::MakeStandardMaterialPrim(
    const AssignmentVector& assignmentsToBind,
    const std::string&      name) const
{
    UsdPrim ret;

    std::string materialName = name;
    if (materialName.empty()) {
        MStatus           status;
        MFnDependencyNode seDepNode(_shadingEngine, &status);
        if (!status) {
            return ret;
        }
        MString seName = seDepNode.name();
        materialName = MNamespace::stripNamespaceFromName(seName).asChar();
    }

    materialName = UsdMayaUtil::SanitizeName(materialName);
    UsdStageRefPtr stage = GetUsdStage();
    if (UsdPrim materialParent
        = _GetMaterialParent(stage, GetExportArgs().materialsScopeName, assignmentsToBind)) {
        SdfPath          materialPath = materialParent.GetPath().AppendChild(TfToken(materialName));
        UsdShadeMaterial material = UsdShadeMaterial::Define(GetUsdStage(), materialPath);

        UsdPrim materialPrim = material.GetPrim();

        return materialPrim;
    }

    return UsdPrim();
}

namespace {
/// We can have multiple mesh with differing UV channel names and we need to make sure the
/// exported material has varname inputs that match the texcoords exported by the shape
class _UVMappingManager
{
public:
    _UVMappingManager(
        const UsdShadeMaterial&                                  material,
        const UsdMayaShadingModeExportContext::AssignmentVector& assignmentsToBind)
        : _material(material)
    {
        // Find out the nodes requiring mapping:
        //
        // The following naming convention is used on UsdShadeMaterial inputs to declare Maya
        // shader nodes contained in the material that have UV inputs that requires mapping:
        //
        //      token inputs:node_with_uv_input:varname = "st"
        //
        // The "node_with_uv_input" is a dependency node which is a valid target for the Maya
        // "uvLink" command, which describes UV linkage for all shapes in the scene that reference
        // the material containing the exported shader node.
        //
        // See lib\usd\translators\shading\usdFileTextureWriter.cpp for an example of an exporter
        // declaring UV inputs that require linking.
        for (const UsdShadeInput& input : material.GetInputs()) {
            const UsdAttribute&      usdAttr = input.GetAttr();
            std::vector<std::string> splitName = usdAttr.SplitName();
            if (splitName.back() != _tokens->varname.GetString()) {
                continue;
            }

            switch (splitName.size()) {
            case 3: _nodesWithUVInput.push_back(TfToken(splitName[1])); break;
            case 4: // NOTE: (yliangsiew) Means we have a Maya node already with a namespace.
            {
                std::string nodeName = splitName[1] + ":" + splitName[2];
                _nodesWithUVInput.push_back(TfToken(nodeName));
                break;
            }
            default: break;
            }
        }

        if (_nodesWithUVInput.empty()) {
            return;
        }

        std::set<TfToken> exportedShapes;
        for (const auto& iter : assignmentsToBind) {
            exportedShapes.insert(iter.shapeName);
        }

        // Ask Maya about UV linkage:
        for (const TfToken& nodeName : _nodesWithUVInput) {
            MString uvLinkCmd;
            uvLinkCmd.format(
                "stringArrayToString(`uvLink -q -t \"^1s\"`, \" \");", nodeName.GetText());
            std::string uvLinkResult = MGlobal::executeCommandStringResult(uvLinkCmd).asChar();
            for (std::string uvSetRef : TfStringTokenize(uvLinkResult)) {
                TfToken shapeName(uvSetRef.substr(0, uvSetRef.find('.')).c_str());
                if (!exportedShapes.count(shapeName)) {
                    continue;
                }
                MString getAttrCmd;
                getAttrCmd.format("getAttr \"^1s\";", uvSetRef.c_str());
                TfToken getAttrResult(MGlobal::executeCommandStringResult(getAttrCmd).asChar());

                // Check if map1 should export as st:
                if (getAttrResult == _tokens->map1 && UsdMayaWriteUtil::WriteMap1AsST()) {
                    getAttrResult = UsdUtilsGetPrimaryUVSetName();
                }

                _shapeNameToUVNames[shapeName].push_back(getAttrResult);
            }
        }

        // Group the shapes by UV mappings:
        using MappingGroups = std::map<TfTokenVector, TfTokenVector>;
        MappingGroups mappingGroups;
        for (const auto& iter : _shapeNameToUVNames) {
            const TfToken&       shapeName = iter.first;
            const TfTokenVector& streams = iter.second;
            mappingGroups[streams].push_back(shapeName);
        }

        // Find out the most common one, which will take over the unspecialized material:
        size_t        largestSize = 0;
        TfTokenVector largestSet;
        for (const auto& iter : mappingGroups) {
            if (iter.second.size() > largestSize) {
                largestSize = iter.second.size();
                largestSet = iter.first;
            }
        }

        // Update the original material with the most common mapping:
        if (largestSize) {
            TfTokenVector::const_iterator itNode = _nodesWithUVInput.cbegin();
            TfTokenVector::const_iterator itName = largestSet.cbegin();
            for (; itNode != _nodesWithUVInput.cend(); ++itNode, ++itName) {
                TfToken inputName(
                    TfStringPrintf("%s:%s", itNode->GetText(), _tokens->varname.GetText()));
                UsdShadeInput materialInput = material.GetInput(inputName);
                materialInput.Set(*itName);
            }
            _uvNamesToMaterial[largestSet] = material;
        }
    }

    const UsdShadeMaterial& getMaterial(const TfToken& shapeName)
    {
        // Look for the UV set names linked to this shape.
        ShapeToStreams::const_iterator shapeStreamIter = _shapeNameToUVNames.find(shapeName);
        if (shapeStreamIter == _shapeNameToUVNames.cend()) {
            // No UV sets linked to this shape, so just use the original
            // material.
            return _material;
        }

        // Look for an existing material for the requested shape based on its
        // UV sets.
        const TfTokenVector&             uvNames = shapeStreamIter->second;
        MaterialMappings::const_iterator iter = _uvNamesToMaterial.find(uvNames);
        if (iter != _uvNamesToMaterial.end()) {
            return iter->second;
        }

        // Create a specialized material:
        std::string newName = _material.GetPrim().GetName();
        for (const TfToken& t : uvNames) {
            newName += "_";
            newName += t.GetString();
        }
        SdfPath newPath
            = _material.GetPrim().GetPath().GetParentPath().AppendChild(TfToken(newName.c_str()));
        UsdShadeMaterial newMaterial
            = UsdShadeMaterial::Define(_material.GetPrim().GetStage(), newPath);
        newMaterial.SetBaseMaterial(_material);

        TfTokenVector::const_iterator itNode = _nodesWithUVInput.cbegin();
        TfTokenVector::const_iterator itName = uvNames.cbegin();
        for (; itNode != _nodesWithUVInput.cend(); ++itNode, ++itName) {
            TfToken inputName(
                TfStringPrintf("%s:%s", itNode->GetText(), _tokens->varname.GetText()));
            UsdShadeInput materialInput
                = newMaterial.CreateInput(inputName, SdfValueTypeNames->Token);
            materialInput.Set(*itName);
        }
        auto insertResult
            = _uvNamesToMaterial.insert(MaterialMappings::value_type { uvNames, newMaterial });
        return insertResult.first->second;
    }

private:
    /// The original material:
    const UsdShadeMaterial& _material;
    /// Helper structures for UV set mappings:
    TfTokenVector _nodesWithUVInput;
    using ShapeToStreams = std::map<TfToken, TfTokenVector>;
    ShapeToStreams _shapeNameToUVNames;
    using MaterialMappings = std::map<TfTokenVector, UsdShadeMaterial>;
    MaterialMappings _uvNamesToMaterial;
};
} // namespace

void UsdMayaShadingModeExportContext::BindStandardMaterialPrim(
    const UsdPrim&          materialPrim,
    const AssignmentVector& assignmentsToBind,
    SdfPathSet* const       boundPrimPaths) const
{
    UsdShadeMaterial material(materialPrim);
    if (!material) {
        TF_RUNTIME_ERROR("Invalid material prim.");
        return;
    }

    _UVMappingManager uvMappingManager(material, assignmentsToBind);

    UsdStageRefPtr stage = GetUsdStage();
    TfToken        materialNameToken(materialPrim.GetName());
    for (const auto& iter : assignmentsToBind) {
        const SdfPath&    boundPrimPath = iter.boundPrimPath;
        const VtIntArray& faceIndices = iter.faceIndices;
        const TfToken&    shapeName = iter.shapeName;

        const UsdShadeMaterial& materialToBind = uvMappingManager.getMaterial(shapeName);
        // In the standard material binding case, skip if we're authoring
        // direct (non-collection-based) bindings and we're an instance
        // proxy.
        // In the case of per-face bindings, un-instance the prim in order
        // to author the append face sets or create a geom subset, since
        // collection-based bindings won't help us here.
        if (faceIndices.empty()) {
            if (!GetExportArgs().exportCollectionBasedBindings) {
                if (_IsInstanceProxyPath(stage, boundPrimPath)) {
                    // XXX: If we wanted to, we could try to author the
                    // binding on the parent prim instead if it's an
                    // instance prim with only one child (i.e. if it's the
                    // transform prim corresponding to our shape prim).
                    TF_WARN(
                        "Can't author direct material binding on "
                        "instance proxy <%s>; try enabling "
                        "collection-based material binding",
                        boundPrimPath.GetText());
                } else {
                    UsdPrim                    boundPrim = stage->OverridePrim(boundPrimPath);
                    UsdShadeMaterialBindingAPI bindingAPI
                        = UsdMayaTranslatorUtil::GetAPISchemaForAuthoring<
                            UsdShadeMaterialBindingAPI>(boundPrim);
                    bindingAPI.Bind(materialToBind);
                }
            }

            if (boundPrimPaths) {
                boundPrimPaths->insert(boundPrimPath);
            }
        } else {
            UsdPrim boundPrim
                = _UninstancePrim(stage, boundPrimPath, "authoring per-face materials");
            UsdShadeMaterialBindingAPI bindingAPI
                = UsdMayaTranslatorUtil::GetAPISchemaForAuthoring<UsdShadeMaterialBindingAPI>(
                    boundPrim);

            UsdGeomSubset faceSubset;

            // Try to re-use existing subset if any:
            for (auto subset : bindingAPI.GetMaterialBindSubsets()) {
                TfToken elementType;
                if (subset.GetPrim().GetName() == materialNameToken
                    && subset.GetElementTypeAttr().Get(&elementType)
                    && elementType == UsdGeomTokens->face) {
                    faceSubset = subset;
                    break;
                }
            }

            if (faceSubset) {
                // Update and continue:
                VtIntArray   mergedIndices;
                UsdAttribute indicesAttribute = faceSubset.GetIndicesAttr();
                indicesAttribute.Get(&mergedIndices);
                std::set<int> uniqueIndices(mergedIndices.cbegin(), mergedIndices.cend());
                uniqueIndices.insert(faceIndices.cbegin(), faceIndices.cend());
                mergedIndices.assign(uniqueIndices.cbegin(), uniqueIndices.cend());
                indicesAttribute.Set(mergedIndices);
                continue;
            }

            faceSubset = bindingAPI.CreateMaterialBindSubset(
                /* subsetName */ materialNameToken,
                faceIndices,
                /* elementType */ UsdGeomTokens->face);

            if (!GetExportArgs().exportCollectionBasedBindings) {
                UsdShadeMaterialBindingAPI subsetBindingAPI
                    = UsdMayaTranslatorUtil::GetAPISchemaForAuthoring<UsdShadeMaterialBindingAPI>(
                        faceSubset.GetPrim());
                subsetBindingAPI.Bind(materialToBind);
            }

            if (boundPrimPaths) {
                boundPrimPaths->insert(faceSubset.GetPath());
            }

            bindingAPI.SetMaterialBindSubsetsFamilyType(UsdGeomTokens->partition);
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
