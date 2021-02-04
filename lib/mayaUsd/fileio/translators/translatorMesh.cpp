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
#include "translatorMesh.h"

#include <mayaUsd/fileio/utils/meshReadUtils.h>
#include <mayaUsd/fileio/utils/meshWriteUtils.h>
#include <mayaUsd/fileio/utils/readUtil.h>
#include <mayaUsd/nodes/pointBasedDeformerNode.h>
#include <mayaUsd/nodes/stageNode.h>
#include <mayaUsd/utils/util.h>

#include <maya/MColor.h>
#include <maya/MColorArray.h>
#include <maya/MDGModifier.h>
#include <maya/MDoubleArray.h>
#include <maya/MFloatArray.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MFnBlendShapeDeformer.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnGeometryFilter.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MItMeshFaceVertex.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPointArray.h>
#include <maya/MString.h>

#include <string>
#include <vector>

namespace MAYAUSD_NS_DEF {

TranslatorMeshRead::TranslatorMeshRead(
    const UsdGeomMesh&        mesh,
    const UsdPrim&            prim,
    const MObject&            transformObj,
    const MObject&            stageNode,
    const GfInterval&         frameRange,
    bool                      wantCacheAnimation,
    UsdMayaPrimReaderContext* context,
    MStatus*                  status)
    : m_wantCacheAnimation(wantCacheAnimation)
    , m_pointsNumTimeSamples(0u)
{
    MStatus stat { MS::kSuccess };

    // ==============================================
    // construct a Maya mesh
    // ==============================================
    VtIntArray faceVertexCounts;
    VtIntArray faceVertexIndices;

    const UsdAttribute fvc = mesh.GetFaceVertexCountsAttr();
    if (fvc.ValueMightBeTimeVarying()) {
        // at some point, it would be great, instead of failing, to create a usd/hydra proxy node
        // for the mesh, perhaps?  For now, better to give a more specific error
        TF_RUNTIME_ERROR(
            "<%s> is a topologically varying Mesh (has animated "
            "faceVertexCounts), which isn't currently supported. "
            "Skipping...",
            prim.GetPath().GetText());
    } else {
        fvc.Get(&faceVertexCounts, UsdTimeCode::EarliestTime());
    }

    const UsdAttribute fvi = mesh.GetFaceVertexIndicesAttr();
    if (fvi.ValueMightBeTimeVarying()) {
        // at some point, it would be great, instead of failing, to create a usd/hydra proxy node
        // for the mesh, perhaps?  For now, better to give a more specific error
        TF_RUNTIME_ERROR(
            "<%s> is a topologically varying Mesh (has animated "
            "faceVertexIndices), which isn't currently supported. "
            "Skipping...",
            prim.GetPath().GetText());
    } else {
        fvi.Get(&faceVertexIndices, UsdTimeCode::EarliestTime());
    }

    // Sanity Checks. If the vertex arrays are empty, skip this mesh
    if (faceVertexCounts.empty() || faceVertexIndices.empty()) {
        TF_RUNTIME_ERROR(
            "faceVertexCounts or faceVertexIndices array is empty "
            "[count: %zu, indices:%zu] on Mesh <%s>. Skipping...",
            faceVertexCounts.size(),
            faceVertexIndices.size(),
            prim.GetPath().GetText());
    }

    // If the USD mesh was left-handed, then the faces had their vertices in left-handed order.
    // Fix them to be in right-handed order, as expected by Maya.
    if (isPrimitiveLeftHanded(mesh)) {
        size_t firstIndex = 0;
        for (int vertexCount : faceVertexCounts) {
            std::reverse(
                faceVertexIndices.begin() + firstIndex,
                faceVertexIndices.begin() + firstIndex + vertexCount);
            firstIndex += vertexCount;
        }
    }

    // Gather points and normals
    // If timeInterval is non-empty, pick the first available sample in the
    // timeInterval or default.
    VtVec3fArray        points;
    VtVec3fArray        normals;
    UsdTimeCode         pointsTimeSample = UsdTimeCode::EarliestTime();
    UsdTimeCode         normalsTimeSample = UsdTimeCode::EarliestTime();
    std::vector<double> pointsTimeSamples;

    if (!frameRange.IsEmpty()) {
        mesh.GetPointsAttr().GetTimeSamplesInInterval(frameRange, &pointsTimeSamples);
        if (!pointsTimeSamples.empty()) {
            m_pointsNumTimeSamples = pointsTimeSamples.size();
            pointsTimeSample = pointsTimeSamples.front();
        }

        std::vector<double> normalsTimeSamples;
        mesh.GetNormalsAttr().GetTimeSamplesInInterval(frameRange, &normalsTimeSamples);
        if (!normalsTimeSamples.empty()) {
            normalsTimeSample = normalsTimeSamples.front();
        }
    }

    mesh.GetPointsAttr().Get(&points, pointsTimeSample);
    mesh.GetNormalsAttr().Get(&normals, normalsTimeSample);

    if (points.empty()) {
        TF_RUNTIME_ERROR(
            "points array is empty on Mesh <%s>. Skipping...", prim.GetPath().GetText());
    }

    std::string reason;
    if (!UsdGeomMesh::ValidateTopology(
            faceVertexIndices, faceVertexCounts, points.size(), &reason)) {
        TF_RUNTIME_ERROR(
            "Skipping Mesh <%s> with invalid topology: %s",
            prim.GetPath().GetText(),
            reason.c_str());
        *status = MS::kFailure;
        return;
    }

    // == Convert data to Maya ( vertices, faces, indices )
    const size_t mayaNumVertices = points.size();
    MPointArray  mayaPoints(mayaNumVertices);
    for (size_t i = 0u; i < mayaNumVertices; ++i) {
        mayaPoints.set(i, points[i][0], points[i][1], points[i][2]);
    }

    MIntArray polygonCounts(faceVertexCounts.cdata(), faceVertexCounts.size());
    MIntArray polygonConnects(faceVertexIndices.cdata(), faceVertexIndices.size());

    // == Create Mesh Shape Node
    MFnMesh meshFn;
    m_meshObj = meshFn.create(
        mayaPoints.length(),
        polygonCounts.length(),
        mayaPoints,
        polygonCounts,
        polygonConnects,
        transformObj,
        &stat);

    if (!stat) {
        *status = stat;
        return;
    }

    // set mesh name
    const auto& primName = prim.GetName().GetString();
    const auto  shapeName = TfStringPrintf("%sShape", primName.c_str());
    meshFn.setName(MString(shapeName.c_str()), false, &stat);

    if (!stat) {
        *status = stat;
        return;
    }

    // store the path
    m_shapePath = prim.GetPath().AppendChild(TfToken(shapeName));

    // Set normals if supplied
    MIntArray normalsFaceIds;
    if (normals.size() == static_cast<size_t>(meshFn.numFaceVertices())) {
        for (size_t i = 0u; i < polygonCounts.length(); ++i) {
            for (int j = 0; j < polygonCounts[i]; ++j) {
                normalsFaceIds.append(i);
            }
        }

        if (normalsFaceIds.length() == static_cast<size_t>(meshFn.numFaceVertices())) {
            MVectorArray mayaNormals(normals.size());
            for (size_t i = 0u; i < normals.size(); ++i) {
                mayaNormals.set(MVector(normals[i][0u], normals[i][1u], normals[i][2u]), i);
            }

            meshFn.setFaceVertexNormals(mayaNormals, normalsFaceIds, polygonConnects);
        }
    }

    // If we are dealing with polys, check if there are normals and set the
    // internal emit-normals tag so that the normals will round-trip.
    // If we are dealing with a subdiv, read additional subdiv tags.
    TfToken subdScheme;
    if (mesh.GetSubdivisionSchemeAttr().Get(&subdScheme) && subdScheme == UsdGeomTokens->none) {
        if (normals.size() == static_cast<size_t>(meshFn.numFaceVertices())
            && mesh.GetNormalsInterpolation() == UsdGeomTokens->faceVarying) {
            UsdMayaMeshReadUtils::setEmitNormalsTag(meshFn, true);
        }
    } else {
        stat = UsdMayaMeshReadUtils::assignSubDivTagsToMesh(mesh, m_meshObj, meshFn);
    }

    // Copy UsdGeomMesh schema attrs into Maya if they're authored.
    UsdMayaReadUtil::ReadSchemaAttributesFromPrim<UsdGeomMesh>(
        prim,
        meshFn.object(),
        { UsdGeomTokens->subdivisionScheme,
          UsdGeomTokens->interpolateBoundary,
          UsdGeomTokens->faceVaryingLinearInterpolation });

    // ==================================================
    // construct blendshape object, PointBasedDeformer
    // ==================================================
    // Code below this point is for handling deforming meshes, so if we don't
    // have time samples to deal with, we're done.
    if (m_pointsNumTimeSamples == 0u) {
        return;
    }

    if (m_wantCacheAnimation) {
        *status = setPointBasedDeformerForMayaNode(m_meshObj, stageNode, prim);
        return;
    }

    // Use blendShapeDeformer so that all the points for a frame are contained in a single node.
    MPointArray mayaAnimPoints(mayaNumVertices);
    MObject     meshAnimObj;

    MFnBlendShapeDeformer blendFn;
    m_meshBlendObj = blendFn.create(m_meshObj);

    for (unsigned int ti = 0u; ti < m_pointsNumTimeSamples; ++ti) {
        mesh.GetPointsAttr().Get(&points, pointsTimeSamples[ti]);

        for (unsigned int i = 0u; i < mayaNumVertices; ++i) {
            mayaAnimPoints.set(i, points[i][0], points[i][1], points[i][2]);
        }

        // == Create Mesh Shape Node
        MFnMesh meshFn;
        if (meshAnimObj.isNull()) {
            meshAnimObj = meshFn.create(
                mayaAnimPoints.length(),
                polygonCounts.length(),
                mayaAnimPoints,
                polygonCounts,
                polygonConnects,
                transformObj,
                &stat);

            if (!stat) {
                continue;
            }
        } else {
            // Reuse the already created mesh by copying it and then setting the points
            meshAnimObj = meshFn.copy(meshAnimObj, transformObj, &stat);
            meshFn.setPoints(mayaAnimPoints);
        }

        // Set normals if supplied
        //
        // NOTE: This normal information is not propagated through the blendShapes, only the
        // controlPoints.
        //
        mesh.GetNormalsAttr().Get(&normals, pointsTimeSamples[ti]);
        if (normals.size() == static_cast<size_t>(meshFn.numFaceVertices())
            && normalsFaceIds.length() == static_cast<size_t>(meshFn.numFaceVertices())) {
            MVectorArray mayaNormals(normals.size());
            for (size_t i = 0; i < normals.size(); ++i) {
                mayaNormals.set(MVector(normals[i][0u], normals[i][1u], normals[i][2u]), i);
            }

            meshFn.setFaceVertexNormals(mayaNormals, normalsFaceIds, polygonConnects);
        }

        // Add as target and set as an intermediate object. We do *not*
        // register the mesh object for undo/redo, since it will be handled
        // automatically by deleting the blend shape deformer object.
        blendFn.addTarget(m_meshObj, ti, meshAnimObj, 1.0);
        meshFn.setIntermediateObject(true);
    }

    // Animate the weights so that mesh0 has a weight of 1 at frame 0, etc.
    MFnAnimCurve animFn;

    // Get the values needed to convert time to the current maya scenes framerate
    MTime::Unit timeUnit = MTime::uiUnit();
    double timeSampleMultiplier = (context != nullptr) ? context->GetTimeSampleMultiplier() : 1.0;

    // Construct the time array to be used for all the keys
    MTimeArray timeArray;
    timeArray.setLength(m_pointsNumTimeSamples);
    for (unsigned int ti = 0u; ti < m_pointsNumTimeSamples; ++ti) {
        timeArray.set(MTime(pointsTimeSamples[ti] * timeSampleMultiplier, timeUnit), ti);
    }

    // Key/Animate the weights
    MPlug plgAry = blendFn.findPlug("weight");
    if (!plgAry.isNull() && plgAry.isArray()) {
        for (unsigned int ti = 0u; ti < m_pointsNumTimeSamples; ++ti) {
            MPlug        plg = plgAry.elementByLogicalIndex(ti, &stat);
            MDoubleArray valueArray(m_pointsNumTimeSamples, 0.0);
            valueArray[ti] = 1.0; // Set the time value where this mesh's weight should be 1.0
            MObject animObj = animFn.create(plg, nullptr, &stat);
            animFn.addKeys(&timeArray, &valueArray);
            // We do *not* register the anim curve object for undo/redo,
            // since it will be handled automatically by deleting the blend
            // shape deformer object.
        }
    }

    *status = stat;
}

bool TranslatorMeshRead::isPrimitiveLeftHanded(const UsdGeomGprim& prim)
{
    TfToken orientation;
    if (!prim.GetOrientationAttr().Get(&orientation)) {
        return false;
    }

    return orientation == UsdGeomTokens->leftHanded;
}

MStatus TranslatorMeshRead::setPointBasedDeformerForMayaNode(
    const MObject& mayaObj,
    const MObject& stageNode,
    const UsdPrim& prim)
{
    MStatus status { MS::kSuccess };

    // Get the output time plug and node for Maya's global time object.
    MPlug timePlug = UsdMayaUtil::GetMayaTimePlug();
    if (timePlug.isNull()) {
        status = MS::kFailure;
    }

    MObject timeNode = timePlug.node(&status);
    CHECK_MSTATUS(status);

    // Clear the selection list so that the deformer command doesn't try to add
    // anything to the new deformer's set. We'll do that manually afterwards.
    status = MGlobal::clearSelectionList();
    CHECK_MSTATUS(status);

    // Create the point based deformer node for this prim.
    const std::string pointBasedDeformerNodeName = TfStringPrintf(
        "usdPointBasedDeformerNode%s",
        TfStringReplace(prim.GetPath().GetString(), SdfPathTokens->childDelimiter.GetString(), "_")
            .c_str());

    const std::string deformerCmd = TfStringPrintf(
        "from maya import cmds; cmds.deformer(name=\'%s\', type=\'%s\')[0]",
        pointBasedDeformerNodeName.c_str(),
        UsdMayaPointBasedDeformerNodeTokens->MayaTypeName.GetText());
    status = MGlobal::executePythonCommand(deformerCmd.c_str(), m_newPointBasedDeformerName);
    CHECK_MSTATUS(status);

    // Get the newly created point based deformer node.
    status = UsdMayaUtil::GetMObjectByName(
        m_newPointBasedDeformerName.asChar(), m_pointBasedDeformerNode);
    CHECK_MSTATUS(status);

    MFnDependencyNode depNodeFn(m_pointBasedDeformerNode, &status);
    CHECK_MSTATUS(status);

    MDGModifier dgMod;

    // Set the prim path on the deformer node.
    MPlug primPathPlug
        = depNodeFn.findPlug(UsdMayaPointBasedDeformerNode::primPathAttr, true, &status);
    CHECK_MSTATUS(status);

    status = dgMod.newPlugValueString(primPathPlug, prim.GetPath().GetText());
    CHECK_MSTATUS(status);

    // Connect the stage node's stage output to the deformer node.
    status = dgMod.connect(
        stageNode,
        UsdMayaStageNode::outUsdStageAttr,
        m_pointBasedDeformerNode,
        UsdMayaPointBasedDeformerNode::inUsdStageAttr);
    CHECK_MSTATUS(status);

    // Connect the global Maya time to the deformer node.
    status = dgMod.connect(
        timeNode,
        timePlug.attribute(),
        m_pointBasedDeformerNode,
        UsdMayaPointBasedDeformerNode::timeAttr);
    CHECK_MSTATUS(status);

    status = dgMod.doIt();
    CHECK_MSTATUS(status);

    // Add the Maya object to the point based deformer node's set.
    const MFnGeometryFilter geomFilterFn(m_pointBasedDeformerNode, &status);
    CHECK_MSTATUS(status);

    MObject deformerSet = geomFilterFn.deformerSet(&status);
    CHECK_MSTATUS(status);

    MFnSet setFn(deformerSet, &status);
    CHECK_MSTATUS(status);

    status = setFn.addMember(mayaObj);
    CHECK_MSTATUS(status);

    // When we created the point based deformer, Maya will have automatically
    // created a tweak deformer and put it *before* the point based deformer in
    // the deformer chain. We don't want that, since any component edits made
    // interactively in Maya will appear to have no effect since they'll be
    // overridden by the point based deformer. Instead, we want the tweak to go
    // *after* the point based deformer. To do this, we need to dig for the
    // name of the tweak deformer node that Maya created to be able to pass it
    // to the reorderDeformers command.
    const MFnDagNode dagNodeFn(mayaObj, &status);
    CHECK_MSTATUS(status);

    // XXX: This seems to be the "most sane" way of finding the tweak deformer
    // node's name...
    const std::string findTweakCmd = TfStringPrintf(
        "from maya import cmds; [x for x in cmds.listHistory(\'%s\') if cmds.nodeType(x) == "
        "\'tweak\'][0]",
        dagNodeFn.fullPathName().asChar());

    MString tweakDeformerNodeName;
    status = MGlobal::executePythonCommand(findTweakCmd.c_str(), tweakDeformerNodeName);
    CHECK_MSTATUS(status);

    // Do the reordering.
    const std::string reorderDeformersCmd = TfStringPrintf(
        "from maya import cmds; cmds.reorderDeformers(\'%s\', \'%s\', \'%s\')",
        tweakDeformerNodeName.asChar(),
        m_newPointBasedDeformerName.asChar(),
        dagNodeFn.fullPathName().asChar());
    status = MGlobal::executePythonCommand(reorderDeformersCmd.c_str());
    CHECK_MSTATUS(status);

    return status;
}

MObject TranslatorMeshRead::meshObject() const { return m_meshObj; }

MObject TranslatorMeshRead::blendObject() const { return m_meshBlendObj; }

MObject TranslatorMeshRead::pointBasedDeformerNode() const { return m_pointBasedDeformerNode; }

MString TranslatorMeshRead::pointBasedDeformerName() const { return m_newPointBasedDeformerName; }

size_t TranslatorMeshRead::pointsNumTimeSamples() const { return m_pointsNumTimeSamples; }

SdfPath TranslatorMeshRead::shapePath() const { return m_shapePath; }

TranslatorMeshWrite::TranslatorMeshWrite(
    const MFnDependencyNode& depNodeFn,
    const UsdStageRefPtr&    stage,
    const SdfPath&           usdPath,
    const MDagPath&          dagPath)
{
    if (!TF_VERIFY(dagPath.isValid())) {
        return;
    }

    if (!UsdMayaMeshWriteUtils::isMeshValid(dagPath)) {
        return;
    }

    m_usdMesh = UsdGeomMesh::Define(stage, usdPath);
    if (!TF_VERIFY(m_usdMesh, "Could not define UsdGeomMesh at path '%s'\n", usdPath.GetText())) {
        return;
    }
}

UsdGeomMesh TranslatorMeshWrite::usdMesh() const { return m_usdMesh; }

} // namespace MAYAUSD_NS_DEF
