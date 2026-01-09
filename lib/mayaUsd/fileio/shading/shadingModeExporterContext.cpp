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
#include <mayaUsd/fileio/utils/meshReadUtils.h>
#include <mayaUsd/fileio/utils/roundTripUtil.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/json.h>
#include <mayaUsd/utils/util.h>

#include <usdUfe/utils/Utils.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/envSetting.h>
#include <pxr/base/tf/iterator.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/types.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdGeom/subset.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdUtils/pipeline.h>

#include <maya/MCommandResult.h>
#include <maya/MDGContext.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnSet.h>
#include <maya/MGlobal.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MNamespace.h>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MUuid.h>

#include <regex>
#include <string>
#include <utility>

// Certain applications do not currently support reading
// TexCoord names via connections, and instead expect a default value.
TF_DEFINE_ENV_SETTING(
    MAYAUSD_PROVIDE_DEFAULT_TEXCOORD_PRIMVAR_NAME,
    false,
    "Add a default value for texcoord names in addition to connections");

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (surfaceShader)
    (volumeShader)
    (displacementShader)
    (varname)
    (varnameStr)
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
        const bool hasExportRootsMapping = !GetExportArgs().rootMapFunction.IsNull();
        auto       findBindableRootFn
            = [this, hasExportRootsMapping](const MDagPath& inDagPath, SdfPath& outPath) {
                  auto iter = _dagPathToUsdMap.find(inDagPath);
                  if (iter != _dagPathToUsdMap.end()) {
                      outPath = iter->second;
                      return true;
                  }

                  // We may be only exporting some roots from under the selected hierarchy.
                  // Search for root but only if export root mapping is set to save time.
                  if (hasExportRootsMapping) {
                      for (auto pair : _dagPathToUsdMap) {
                          if (MFnDagNode(pair.first).hasParent(inDagPath.node())) {
                              outPath = pair.second;
                              return true;
                          }
                      }
                  }

                  return false;
              };

        TF_FOR_ALL(bindableRootIter, GetExportArgs().dagPaths)
        {
            const MDagPath& bindableRootDagPath = *bindableRootIter;

            SdfPath usdPath;
            if (!findBindableRootFn(bindableRootDagPath, usdPath)) {
                // Geometry w/ this material bound doesn't seem to exist in USD.
                continue;
            }

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

UsdMayaShadingModeExportContext::AssignmentsInfo
UsdMayaShadingModeExportContext::GetAssignments() const
{
    AssignmentsInfo ret;

    MStatus           status;
    MFnDependencyNode seDepNode(_shadingEngine, &status);
    if (!status) {
        return ret;
    }

#if MAYA_HAS_GET_MEMBER_PATHS
    MFnSet fnSet(_shadingEngine, &status);
    if (!status) {
        return ret;
    }

    // Get all the dagPaths using this shadingEngine...
    MDagPathArray dagPaths;
    fnSet.getMemberPaths(dagPaths, true); // get all the dagPath related to shading
    SdfPathSet seenBoundPrimPaths;

    for (auto& dagPath : dagPaths) {
        unsigned int instanceNumber = dagPath.instanceNumber();
#else
    // Maya 2022 and older use this version
    MPlug dsmPlug = seDepNode.findPlug("dagSetMembers", true, &status);
    if (!status) {
        return ret;
    }

    SdfPathSet seenBoundPrimPaths;
    for (unsigned int i = 0; i < dsmPlug.numConnectedElements(); i++) {
        MPlug dsmElemPlug(dsmPlug.connectionByPhysicalIndex(i));
        MPlug connectedPlug = UsdMayaUtil::GetConnected(dsmElemPlug);

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
#endif
        MFnDagNode dagNode(dagPath, &status);
        if (!status) {
            continue;
        }

        ret.hasAnyAssignment = true;

        // Note: the connections will be to the mesh, but the selection often
        //       contains the transform above the mesh or a higher-up ancestor,
        //       so also check the ancestors of the mesh.
        for (MDagPath walkUpPath(dagPath); walkUpPath.length() > 0; walkUpPath.pop())
            if (GetExportArgs().fullObjectList.hasItem(walkUpPath))
                ret.hasAnyAssignmentInSelection = true;

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
            ret.assignments.push_back(Assignment {
                usdPath, faceIndices, TfToken(dagNode.name().asChar()), dagNode.object() });
        }
    }
    return ret;
}

static SdfPath _GetCommonAncestor(
    const UsdStageRefPtr&                                    stage,
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
    return commonAncestor;
}

static UsdPrim _GetLegacyMaterialParent(
    const UsdStageRefPtr&                                    stage,
    const TfToken&                                           materialsScopeName,
    const UsdMayaShadingModeExportContext::AssignmentVector& assignments)
{
    SdfPath shaderExportLocation = _GetCommonAncestor(stage, assignments);

    if (shaderExportLocation.IsEmpty()) {
        return UsdPrim();
    }

    if (shaderExportLocation == SdfPath::AbsoluteRootPath()) {
        return stage->GetPseudoRoot();
    }

    while (!shaderExportLocation.IsRootPrimPath()) {
        shaderExportLocation = shaderExportLocation.GetParentPath();
    }

    shaderExportLocation = shaderExportLocation.AppendChild(materialsScopeName);

    return UsdGeomScope::Define(stage, shaderExportLocation).GetPrim();
}

static UsdPrim _GetMaterialParent(
    const UsdStageRefPtr& stage,
    const std::string&    defaultPrim,
    const TfToken&        materialsScopeName)
{
    SdfPath shaderExportLocation = SdfPath::AbsoluteRootPath();

    if (!defaultPrim.empty() && defaultPrim != materialsScopeName && defaultPrim != "None")
        shaderExportLocation = shaderExportLocation.AppendChild(TfToken(defaultPrim));

    if (!materialsScopeName.IsEmpty())
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

namespace {
// Detect a name that was generated directly from a dg node typename:
const std::regex kTemplatedRegex("^([a-zA-Z]+)([0-9]*)(SG)?$");

bool isSurfaceNodeType(const std::string& nodeType)
{
    static std::vector<std::string> sKnownSurfaces;

    if (sKnownSurfaces.empty()) {
        MString     listSurfCmd("stringArrayToString(`listNodeTypes \"shader/surface\"`, \" \");");
        std::string cmdResult = MGlobal::executeCommandStringResult(listSurfCmd).asChar();
        sKnownSurfaces = TfStringTokenize(cmdResult);
        // O(logN) will be close to O(N) for searches since N will usually be small, but with enough
        // plugin surface nodes added it could start to matter, so let's sort the vector.
        std::sort(sKnownSurfaces.begin(), sKnownSurfaces.end());
    }

    return std::binary_search(sKnownSurfaces.cbegin(), sKnownSurfaces.cend(), nodeType);
}

std::string getMaterialName(
    const std::string& materialName,
    const MObject&     shadingEngine,
    const MObject&     surfaceShader)
{
    if (!materialName.empty())
        return materialName;

    MFnDependencyNode fnDepNode;
    if (fnDepNode.setObject(shadingEngine) != MS::kSuccess)
        return materialName;

    std::string sgName = fnDepNode.name().asChar();
    std::smatch sgMatch;
    // Is the SG name following the standard Maya naming protocol for a known surface nodeType?
    if (!std::regex_match(sgName, sgMatch, kTemplatedRegex))
        return sgName;

    if (!isSurfaceNodeType(sgMatch[1].str()))
        return sgName;

    // Check if the surface shader has a more descriptive name
    if (fnDepNode.setObject(surfaceShader) == MS::kSuccess) {
        std::string surfName = fnDepNode.name().asChar();
        std::smatch surfMatch;
        if (std::regex_match(surfName, surfMatch, kTemplatedRegex)) {
            // Surface node name is also templated. Check the nodeType part.
            if (!isSurfaceNodeType(surfMatch[1].str())) {
                // The surface is not named after a standard nodeType, so its name is more
                // interesting:
                sgName = surfName + "SG";
            }
        } else {
            // Surface node is definitely more interesting since it does not follow a
            // templated name:
            sgName = surfName + "SG";
        }
    }

    return sgName;
}

bool shouldExportMaterial(
    const UsdMayaShadingModeExportContext::AssignmentsInfo& assignmentsInfo,
    const MObject&                                          surfaceShader,
    const UsdMayaJobExportArgs&                             exportArgs)
{
    // The export-assigned-materials flag means to remove materials
    // that are not assigned to any meshes. (We take into consideration
    // even meshes that are not exported.)
    if (exportArgs.exportAssignedMaterials) {
        if (!assignmentsInfo.hasAnyAssignment) {
            return false;
        }
    }

    // In export-selected mode, ony export selected materials.
    // Materials of exported meshes are also added to the selection.
    if (exportArgs.exportSelected) {
        if (assignmentsInfo.hasAnyAssignmentInSelection) {
            return true;
        }
        return exportArgs.fullObjectList.hasItem(surfaceShader);
    } else {
        // In export-all mode, export all materials by default.
        return true;
    }
}

} // namespace

UsdPrim UsdMayaShadingModeExportContext::MakeStandardMaterialPrim(
    const AssignmentsInfo& assignmentsInfo,
    const std::string&     name) const
{
    const UsdMayaJobExportArgs& exportArgs = GetExportArgs();

    if (!shouldExportMaterial(assignmentsInfo, GetSurfaceShader(), exportArgs))
        return UsdPrim();

    const std::string materialName
        = UsdUfe::sanitizeName(getMaterialName(name, _shadingEngine, GetSurfaceShader()));
    if (materialName.empty())
        return UsdPrim();

    UsdStageRefPtr stage = GetUsdStage();
    UsdPrim        materialParent = exportArgs.legacyMaterialScope
        ? _GetLegacyMaterialParent(
            stage, exportArgs.materialsScopeName, assignmentsInfo.assignments)
        : _GetMaterialParent(stage, exportArgs.defaultPrim, exportArgs.materialsScopeName);
    if (!materialParent)
        return UsdPrim();

    SdfPath          materialPath = materialParent.GetPath().AppendChild(TfToken(materialName));
    UsdShadeMaterial material = UsdShadeMaterial::Define(GetUsdStage(), materialPath);

    return material.GetPrim();
}

namespace {
/// We can have multiple mesh with differing UV channel names and we need to make sure the exported
/// material has varname or varnameStr inputs that match the texcoords exported by the shape
class _UVMappingManager
{
public:
    _UVMappingManager(
        const UsdShadeMaterial&                                  material,
        const UsdMayaShadingModeExportContext::AssignmentVector& assignmentsToBind,
        const UsdMayaJobExportArgs&                              exportArgs,
        const std::string&                                       materialType)
        : _material(material)
        , _exportArgs(exportArgs)
    {
        // Find out the nodes requiring mapping:
        //
        // The following naming convention is used on UsdShadeMaterial inputs to declare Maya
        // shader nodes contained in the material that have UV inputs that requires mapping:
        //
        //      token inputs:node_with_uv_input:varname = "st"
        //      string inputs:node_with_uv_input:varnameStr = "st"
        //
        // The "node_with_uv_input" is a dependency node which is a valid target for the Maya
        // "uvLink" command, which describes UV linkage for all shapes in the scene that reference
        // the material containing the exported shader node.
        //
        // See lib\usd\translators\shading\usdFileTextureWriter.cpp for an example of an exporter
        // declaring UV inputs that require linking.
        for (const UsdShadeInput& input : material.GetInputs()) {
            // Split the input name along ':' (that is what SplitName does)
            // and if the last element is "varname" or "varnameStr" then
            // the Maya node name is added to _nodesWithUVInput. (The Maya
            // node name is the middle part of the split name.)
            const UsdAttribute&      usdAttr = input.GetAttr();
            std::vector<std::string> splitName = usdAttr.SplitName();
            if (splitName.back() != _tokens->varname.GetString()
                && splitName.back() != _tokens->varnameStr.GetString()) {
                continue;
            }

            // Maya node names can be inside a Maya namespace. The Maya namespace
            // separator is *also* ':'. So we need to check if the split name
            // has more than 3 parts, and if so, we need to concatenate the parts.
            switch (splitName.size()) {
            case 3: {
                _nodesWithUVInput.push_back(TfToken(splitName[1]));
                break;
            }
            default: {
                std::string mayaNodeName = splitName[1];
                for (size_t i = 2; i < splitName.size() - 1; ++i) {
                    mayaNodeName += ":" + splitName[i];
                }
                _nodesWithUVInput.push_back(TfToken(mayaNodeName));
                break;
            }
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
        for (size_t i = 0; i < _nodesWithUVInput.size(); ++i) {
            const TfToken& nodeName = _nodesWithUVInput[i];

            // The MaterialXSurfaceShader does not support the uvLink command for now.
            // LookDevX is only looking at the first UV set, do the same here.
            if (materialType == "MaterialXSurfaceShader") {
                for (const auto& iter : assignmentsToBind) {
                    MObject shapeObj = iter.shapeObj.object();
                    MStatus status;
                    // Create an MFnMesh to work with the mesh object
                    MFnMesh meshFn(shapeObj, &status);
                    if (!status) {
                        TF_WARN(
                            "Failed to initialize MFnMesh for shape object \"%s\".",
                            iter.shapeName.GetText());
                        continue;
                    }
                    // Get the UV set names
                    MStringArray uvSetNames;
                    status = meshFn.getUVSetNames(uvSetNames);
                    if (!status || uvSetNames.length() == 0) {
                        TF_WARN(
                            "Failed to get UV set names for shape object \"%s\".",
                            iter.shapeName.GetText());
                        continue;
                    }

                    // UV Renaming based on options
                    MString uvSetName = UsdMayaWriteUtil::UVSetExportedName(
                        uvSetNames, _exportArgs.preserveUVSetNames, _exportArgs.remapUVSetsTo, 0);

                    _shapeNameToUVNames[iter.shapeName].push_back(TfToken(uvSetName.asChar()));
                }
            } else {
                MString uvLinkCmd;
                uvLinkCmd.format(
                    "stringArrayToString(`uvLink -q -t \"^1s\"`, \" \");", nodeName.GetText());
                std::string uvLinkResult = MGlobal::executeCommandStringResult(uvLinkCmd).asChar();
                std::vector<std::string> uvSets = TfStringTokenize(uvLinkResult);
                // If there are not UV sets, then the node has no valid UV input we must remove it.
                // Otherwise, assumptions in the code below won't be satisfied.
                if (uvSets.empty()) {
                    TF_WARN(
                        "Could not determine the UV link of the UV input of the node \"%s\".",
                        nodeName.GetText());
                    _nodesWithUVInput.erase(_nodesWithUVInput.begin() + i);
                    --i;
                    continue;
                }

                for (std::string uvSetRef : uvSets) {
                    // NOTE: If the mesh shape has the same name as the transform, then we will get
                    //       a complete path like
                    //           |mesh|mesh.uvSet[0].uvSetName
                    //       the best way to prevent confusion is to move to the object model
                    //       immediately and process from there.
                    MSelectionList selList;
                    selList.add(uvSetRef.c_str());
                    MPlug uvNamePlug;
                    selList.getPlug(0, uvNamePlug);

                    MFnMesh meshFn(uvNamePlug.node());
                    TfToken shapeName(meshFn.name().asChar());
                    if (!exportedShapes.count(shapeName)) {
                        continue;
                    }

                    MString uvSetName = uvNamePlug.asString();

                    // UV set renaming still exists. See if the UV was renamed:
                    MStringArray uvSets;
                    meshFn.getUVSetNames(uvSets);
                    for (unsigned int i = 0; i < uvSets.length(); i++) {
                        if (uvSets[i] == uvSetName) {
                            uvSetName = UsdMayaWriteUtil::UVSetExportedName(
                                uvSets,
                                _exportArgs.preserveUVSetNames,
                                _exportArgs.remapUVSetsTo,
                                i);
                            break;
                        }
                    }

                    _shapeNameToUVNames[shapeName].push_back(TfToken(uvSetName.asChar()));
                }
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
            // TODO: why is it assumed that the largest set has the same size as the nodes?
            //       Could the UV sets not be disjoint?
            TfTokenVector::const_iterator itNode = _nodesWithUVInput.cbegin();
            TfTokenVector::const_iterator itName = largestSet.cbegin();
            for (; itNode != _nodesWithUVInput.cend(); ++itNode, ++itName) {
                TF_VERIFY(itName != largestSet.cend());
                std::string inputName(
                    TfStringPrintf("%s:%s", itNode->GetText(), _tokens->varname.GetText()));
                UsdShadeInput materialInput = material.GetInput(TfToken(inputName.c_str()));
                if (materialInput) {
                    // varname becomes a std::string in USD 20.11
                    if (materialInput.GetTypeName() == SdfValueTypeNames->Token) {
                        materialInput.Set(*itName);
                    } else {
                        materialInput.Set(itName->GetString());
                    }
                }
                inputName
                    = TfStringPrintf("%s:%s", itNode->GetText(), _tokens->varnameStr.GetText());
                materialInput = material.GetInput(TfToken(inputName.c_str()));
                if (materialInput) {
                    materialInput.Set(itName->GetString());
                }
            }
            _uvNamesToMaterial[largestSet] = material;
        }

        if (TfGetEnvSetting(MAYAUSD_PROVIDE_DEFAULT_TEXCOORD_PRIMVAR_NAME)) {
            // We'll traverse the material prims children and look for items
            // with connections, and use that to set the value
            TfToken readerType { "UsdPrimvarReader_float2" };
            TfToken shaderID;

            UsdShadeConnectableAPI source;
            TfToken                sourceInputName;
            UsdShadeAttributeType  sourceType;
            VtValue                varnameValue;
            auto                   materialPrim = material.GetPrim();
            for (const auto child : materialPrim.GetDescendants()) {
                if (!child.IsA<UsdShadeShader>()) {
                    continue;
                }

                auto shader = UsdShadeShader(child);
                if (shader.GetShaderId(&shaderID) && shaderID != readerType) {
                    continue;
                }

                auto varnameInput = shader.GetInput(TfToken("varname"));
                if (!varnameInput) {
                    continue;
                }

                if (!UsdShadeConnectableAPI::GetConnectedSource(
                        varnameInput, &source, &sourceInputName, &sourceType)) {
                    continue;
                }

                auto targetVarname = material.GetInput(sourceInputName);
                if (!targetVarname) {
                    continue;
                }

                targetVarname.Get(&varnameValue);
                varnameInput.Set(varnameValue);
            }
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
            std::string inputName(
                TfStringPrintf("%s:%s", itNode->GetText(), _tokens->varname.GetText()));
            UsdShadeInput materialInput = newMaterial.GetInput(TfToken(inputName.c_str()));
            if (materialInput) {
                // varname becomes a std::string in USD 20.11
                if (materialInput.GetTypeName() == SdfValueTypeNames->Token) {
                    materialInput.Set(*itName);
                } else {
                    materialInput.Set(itName->GetString());
                }
            }
            inputName = TfStringPrintf("%s:%s", itNode->GetText(), _tokens->varnameStr.GetText());
            materialInput = newMaterial.GetInput(TfToken(inputName.c_str()));
            if (materialInput) {
                materialInput.Set(itName->GetString());
            }
        }
        auto insertResult
            = _uvNamesToMaterial.insert(MaterialMappings::value_type { uvNames, newMaterial });
        return insertResult.first->second;
    }

private:
    /// The original material:
    const UsdShadeMaterial& _material;
    /// The export args (for UV set remapping)
    const UsdMayaJobExportArgs& _exportArgs;
    /// Helper structures for UV set mappings:
    TfTokenVector _nodesWithUVInput;
    using ShapeToStreams = std::map<TfToken, TfTokenVector>;
    ShapeToStreams _shapeNameToUVNames;
    using MaterialMappings = std::map<TfTokenVector, UsdShadeMaterial>;
    MaterialMappings _uvNamesToMaterial;
};

/// We might have one or more component tags that have roundtrip data that covers this
// set of faces. Assign them to those tags and return the remaining unhandled faces.
VtIntArray _AssignComponentTags(
    const UsdMayaShadingModeExportContext* ctx,
    const UsdShadeMaterial&                materialToBind,
    const MObjectHandle&                   geomHandle,
    const std::vector<UsdGeomSubset>&      currentGeomSubsets,
    const VtIntArray&                      faceIndices,
    SdfPathSet* const                      boundPrimPaths)
{
    if (currentGeomSubsets.empty() || faceIndices.empty() || !geomHandle.isValid()) {
        return faceIndices;
    }

    JsValue info;
    if (UsdMayaMeshReadUtils::getGeomSubsetInfo(geomHandle.object(), info) && info) {
        auto subsetInfoDict = info.GetJsObject();

        std::set<VtIntArray::ElementType> faceSet, handledSet;
        faceSet.insert(faceIndices.cbegin(), faceIndices.cend());

        MFnDependencyNode shadingGroupDepFn(ctx->GetShadingEngine());
        std::string       shadingGroupUUID = shadingGroupDepFn.uuid().asString().asChar();

        for (auto&& geomSubset : currentGeomSubsets) {
            std::string ssName = geomSubset.GetPrim().GetName();
            const auto  infoIt = subsetInfoDict.find(ssName);
            if (infoIt != subsetInfoDict.cend()) {
                // Check candidate:
                if (!infoIt->second.IsObject()) {
                    TF_RUNTIME_ERROR(
                        "Invalid GeomSubset roundtrip info on node '%s': "
                        "info for subset '%s' is not a dictionary.",
                        UsdMayaUtil::GetMayaNodeName(geomHandle.object()).c_str(),
                        ssName.c_str());
                    continue;
                }
                const auto& geomSubsetInfo = infoIt->second.GetJsObject();
                const auto  uuidIt = geomSubsetInfo.find(UsdMayaGeomSubsetTokens->MaterialUuidKey);
                if (uuidIt == geomSubsetInfo.cend()) {
                    continue;
                }
                if (!uuidIt->second.IsString()) {
                    TF_RUNTIME_ERROR(
                        "Invalid GeomSubset roundtrip info on node '%s': "
                        "material info for subset '%s' is not a string.",
                        UsdMayaUtil::GetMayaNodeName(geomHandle.object()).c_str(),
                        ssName.c_str());
                    continue;
                }
                if (uuidIt->second.GetString() != shadingGroupUUID) {
                    continue;
                }

                VtIntArray subsetIndices;
                geomSubset.GetIndicesAttr().Get(&subsetIndices);

                // We bind if the componentTag is a subset of the material indices
                bool bindMaterial = true;
                for (auto it = subsetIndices.cbegin(); it != subsetIndices.cend(); ++it) {
                    if (faceSet.count(*it) == 0) {
                        // The componentTag is out of sync with the material assignment.
                        // We currently do not try to reconcile as we have no idea how
                        // the situation happened.
                        bindMaterial = false;
                        break;
                    }
                }

                if (!bindMaterial) {
                    continue;
                }

                if (!ctx->GetExportArgs().exportCollectionBasedBindings) {
                    UsdShadeMaterialBindingAPI subsetBindingAPI
                        = UsdMayaTranslatorUtil::GetAPISchemaForAuthoring<
                            UsdShadeMaterialBindingAPI>(geomSubset.GetPrim());
                    subsetBindingAPI.Bind(materialToBind);
                }

                if (boundPrimPaths) {
                    boundPrimPaths->insert(geomSubset.GetPath());
                }

                // Mark those faces as handled
                handledSet.insert(subsetIndices.cbegin(), subsetIndices.cend());
            }
        }
        if (!handledSet.empty()) {
            // Prune handled faces:
            for (auto&& handledFace : handledSet) {
                faceSet.erase(handledFace);
            }
            VtIntArray unhandledFaces;
            unhandledFaces.assign(faceSet.begin(), faceSet.end());
            return unhandledFaces;
        }
    }

    return faceIndices;
}

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

    _UVMappingManager uvMappingManager(
        material,
        assignmentsToBind,
        GetExportArgs(),
        MFnDependencyNode(GetSurfaceShader()).typeName().asChar());

    UsdStageRefPtr stage = GetUsdStage();
    TfToken        materialNameToken(materialPrim.GetName());
    for (const auto& iter : assignmentsToBind) {
        const SdfPath&    boundPrimPath = iter.boundPrimPath;
        const VtIntArray& faceIndices = iter.faceIndices;
        const TfToken&    shapeName = iter.shapeName;

        const UsdShadeMaterial& materialToBind = uvMappingManager.getMaterial(shapeName);

        // Determine whether we need to create UsdGeomSubsets to handle per-face
        // material assignments. This is only necessary if an assignment is
        // bound to some, but not all, of the faces on the mesh.
        const bool perFaceMaterialAssignment = std::invoke([&]() -> bool {
            if (faceIndices.empty()) {
                return false;
            }

            if (UsdGeomMesh mesh = UsdGeomMesh(stage->GetPrimAtPath(boundPrimPath))) {
                VtIntArray faceVertexCounts;
                if (mesh.GetFaceVertexCountsAttr().Get(&faceVertexCounts)) {
                    return (faceIndices.size() < faceVertexCounts.size());
                }
            }

            return false;
        });

        // In the standard material binding case, skip if we're authoring
        // direct (non-collection-based) bindings and we're an instance
        // proxy.
        // In the case of per-face bindings, un-instance the prim in order
        // to author the append face sets or create a geom subset, since
        // collection-based bindings won't help us here.
        if (!perFaceMaterialAssignment) {
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

            const auto currentGeomSubsets = bindingAPI.GetMaterialBindSubsets();
            VtIntArray unhandledFaceIndices = _AssignComponentTags(
                this,
                materialToBind,
                iter.shapeObj,
                currentGeomSubsets,
                faceIndices,
                boundPrimPaths);

            // If we assigned all faces to component tags, then we are done for this
            // material.
            if (unhandledFaceIndices.empty()) {
                continue;
            }

            UsdGeomSubset faceSubset;

            // Try to re-use existing subset if any:
            for (auto subset : currentGeomSubsets) {
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
                uniqueIndices.insert(unhandledFaceIndices.cbegin(), unhandledFaceIndices.cend());
                mergedIndices.assign(uniqueIndices.cbegin(), uniqueIndices.cend());
                indicesAttribute.Set(mergedIndices);
                continue;
            }

            faceSubset = bindingAPI.CreateMaterialBindSubset(
                /* subsetName */ materialNameToken,
                unhandledFaceIndices,
                /* elementType */ UsdGeomTokens->face);
            UsdMayaRoundTripUtil::MarkPrimAsMayaGenerated(faceSubset.GetPrim());

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
