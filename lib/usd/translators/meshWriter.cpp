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
#include "meshWriter.h"

#include <mayaUsd/fileio/primWriter.h>
#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/translators/translatorMesh.h>
#include <mayaUsd/fileio/utils/adaptor.h>
#include <mayaUsd/fileio/utils/jointWriteUtils.h>
#include <mayaUsd/fileio/utils/meshReadUtils.h>
#include <mayaUsd/fileio/utils/meshWriteUtils.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/pointBased.h>
#include <pxr/usd/usdGeom/primvar.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdSkel/root.h>
#include <pxr/usd/usdUtils/pipeline.h>

#include <maya/MFloatArray.h>
#include <maya/MFnBlendShapeDeformer.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnGeometryFilter.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MUintArray.h>

#include <cfloat>
#include <set>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

MObject mayaFindOrigMeshFromBlendShapeTarget(const MObject& mesh, MObjectArray* intermediates)
{
    TF_VERIFY(mesh.hasFn(MFn::kMesh));
    MStatus stat;

    // NOTE: (yliangsiew) If there's a skinCluster, find that first since that
    // will be the intermediate to the blendShape node. If not, just search for any
    // blendshape deformers upstream of the mesh.
    MObject searchObject;
    MObject skinCluster;
    stat = UsdMayaMeshWriteUtils::getSkinClusterConnectedToMesh(mesh, skinCluster);
    if (stat) {
        searchObject = MObject(skinCluster);
    } else {
        searchObject = MObject(mesh);
    }

    // NOTE: (yliangsiew) Problem: if there are _intermediate deformers between blendshapes,
    // then oh-no: what do we do? Like blendshape1 -> wrap -> blendshape2. This won't find
    // that correctly...so we just tell the client what we found and let them decide how to
    // handle it.
    MFnGeometryFilter fnGeoFilter;
    TF_VERIFY(MObjectHandle(searchObject).isValid());
    MFnBlendShapeDeformer fnBlendShape;
    MItDependencyGraph    itDg(
        searchObject,
        MFn::kBlendShape,
        MItDependencyGraph::kUpstream,
        MItDependencyGraph::kDepthFirst,
        MItDependencyGraph::kPlugLevel,
        &stat);
    MFnDependencyNode fnNode;
    if (intermediates == nullptr) {
        for (; !itDg.isDone(); itDg.next()) {
            MObject curBlendShape = itDg.currentItem();
            TF_VERIFY(curBlendShape.hasFn(MFn::kBlendShape));
            MPlug outputGeomPlug = itDg.thisPlug();
            TF_VERIFY(outputGeomPlug.isElement() == true);
            unsigned int outputGeomPlugIdx = outputGeomPlug.logicalIndex();

            // NOTE: (yliangsiew) Because we can have multiple output
            // deformed meshes from a single blendshape deformer, we have
            // to walk back up the graph using the connected index to find
            // out what the _actual_ base mesh was.
            MFnGeometryFilter fnGeoFilter;
            fnGeoFilter.setObject(curBlendShape);
            MObject inputGeo = fnGeoFilter.inputShapeAtIndex(outputGeomPlugIdx, &stat);
            if (inputGeo.hasFn(MFn::kMesh)) {
                return inputGeo;
            }
        }
    } else {
        intermediates->clear();
        for (; !itDg.isDone(); itDg.next()) {
            MObject curBlendShape = itDg.currentItem();
            TF_VERIFY(curBlendShape.hasFn(MFn::kBlendShape));
            MPlug outputGeomPlug = itDg.thisPlug();
            TF_VERIFY(outputGeomPlug.isElement() == true);

            // Find the corresponding "inputGeometry" plug:
            MFnDependencyNode blendShapeNode(curBlendShape);
            MPlug inputGeomPlug(curBlendShape, blendShapeNode.attribute("inputGeometry"));
            inputGeomPlug.selectAncestorLogicalIndex(
                outputGeomPlug.logicalIndex(), blendShapeNode.attribute("input"));
            MItDependencyGraph itDgBS(
                inputGeomPlug,
                MFn::kInvalid,
                MItDependencyGraph::kUpstream,
                MItDependencyGraph::kDepthFirst,
                MItDependencyGraph::kNodeLevel,
                &stat);
            for (itDgBS.next(); !itDgBS.isDone();
                 itDgBS.next()) { // NOTE: (yliangsiew) Skip the first node which starts at the
                // root, which is the blendshape deformer itself.
                MObject curNode = itDgBS.thisNode();
                if (curNode.hasFn(MFn::kMesh)) {
                    return curNode;
                }
                intermediates->append(curNode);
            }
        }
    }

    return mesh;
}

MStatus mayaCheckIntermediateNodesForMeshEdits(const MObjectArray& intermediates)
{
    // TODO: (yliangsiew) In future, have this function not be responsible for printing diagnostic
    // info and just return results instead if necessary.
    MStatus      status;
    unsigned int numIntermediates = intermediates.length();
    for (unsigned int i = 0; i < numIntermediates; ++i) {
        MObject curIntermediate = intermediates[i];
        if (curIntermediate.hasFn(MFn::kGroupParts)) {
            continue;
        } else if (curIntermediate.hasFn(MFn::kGeometryFilt)) {
            // NOTE: (yliangsiew) We make sure the tweak node is empty first, since
            // that could potentially affect deformation of the origShape before it hits the
            // blendshape node.
            MFnGeometryFilter fnGeoFilt(curIntermediate, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);
            float envelope = fnGeoFilt.envelope();
            if (fabs(envelope - 0.0f) < FLT_EPSILON) {
                continue; // deformer has no effect
            }

            if (curIntermediate.hasFn(MFn::kTweak)) {
                // NOTE: (yliangsiew) Make sure tweak really has no effect even if it's on
                MPlug plgVLists = fnGeoFilt.findPlug("vlist");
                TF_VERIFY(plgVLists.isArray());
                unsigned int numPlgVlists = plgVLists.numElements();
                for (unsigned int j = 0; j < numPlgVlists; ++j) {
                    MPlug plgVlist = plgVLists.elementByPhysicalIndex(j); // vlist[0]
                    TF_VERIFY(plgVlist.isCompound());
                    unsigned int plgVlistNumChildren = plgVlist.numChildren();
                    for (unsigned int k = 0; k < plgVlistNumChildren; ++k) {
                        MPlug plgVlistChild = plgVlist.child(k); // vlist[0].vertex
                        TF_VERIFY(plgVlistChild.isArray());
                        unsigned int numPlgVlistChildElements = plgVlistChild.numElements();
                        for (unsigned int x = 0; x < numPlgVlistChildElements; ++x) {
                            MPlug plgVertex
                                = plgVlistChild.elementByPhysicalIndex(x); // vlist[0].vertex[0]
                            TF_VERIFY(plgVertex.isCompound());
                            unsigned int plgVertexNumChildren = plgVertex.numChildren();
                            for (unsigned int y = 0; y < plgVertexNumChildren; ++y) {
                                MPlug plgVertexComponent
                                    = plgVertex.child(y); // vlist[0].vertex[0].xVertex
                                float vertexComponent = plgVertexComponent.asFloat();
                                if (fabs(vertexComponent - 0.0f) > FLT_EPSILON) {
                                    MFnDependencyNode fnNode(curIntermediate, &status);
                                    CHECK_MSTATUS_AND_RETURN_IT(status);
                                    TF_RUNTIME_ERROR(
                                        "Could not determine the original blendshape "
                                        "source mesh due to a non-empty tweak node: %s. "
                                        "Please either bake it down or remove the "
                                        "edits and attempt the export process again, or "
                                        "specify -ignoreWarnings.",
                                        fnNode.name().asChar());
                                    return MStatus::kFailure;
                                }
                            }
                        }
                    }
                }
                continue; // NOTE: (yliangsiew) If the tweak node has no effect, go check
                // the next intermediate.
            }
            TF_RUNTIME_ERROR(
                "USDSkelBlendShape does not support animated blend shapes and a node: %s was found "
                "that could potentially cause it. Please bake "
                "down deformer history before attempting an export, or specify "
                "-ignoreWarnings during the export process.",
                fnGeoFilt.name().asChar());
            return MStatus::kFailure;

        } else if (curIntermediate.hasFn(MFn::kMesh)) {
            // NOTE: (yliangsiew) Need to check that the mesh itself does not include any
            // tweaks.
            MFnDependencyNode fnNode(curIntermediate, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);
            MPlug plgPnts = fnNode.findPlug("pnts");
            TF_VERIFY(plgPnts.isArray());
            for (unsigned int j = 0; j < plgPnts.numElements(); ++j) {
                MPlug plgPnt = plgPnts.elementByPhysicalIndex(j);
                TF_VERIFY(plgPnt.isCompound());
                for (unsigned int k = 0; k < plgPnt.numChildren(); ++k) {
                    float tweakValue = plgPnt.child(k).asFloat();
                    if (fabs(tweakValue - 0.0f) > FLT_EPSILON) {
                        TF_RUNTIME_ERROR(
                            "The mesh: %s has local tweak data on its .pnts attribute. "
                            "Please remove it before attempting an export, or specify "
                            "-ignoreWarnings during the export process.",
                            fnNode.name().asChar());
                        return MStatus::kFailure;
                    }
                }
            }

        } else {
            MFnDependencyNode fnNode(curIntermediate, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);
            TF_RUNTIME_ERROR(
                "Unrecognized node encountered in blendshape deformation chain: %s. Please "
                "bake down deformer history before attempting an export, or specify "
                "-ignoreWarnings during the export process.",
                fnNode.name().asChar());
            return MStatus::kFailure;
        }
    }
    return MStatus::kSuccess;
}

PXRUSDMAYA_REGISTER_WRITER(mesh, PxrUsdTranslators_MeshWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(mesh, UsdGeomMesh);

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((skelJointIndices, "skel:jointIndices"))
    ((skelJointWeights, "skel:jointWeights"))
    ((skelGeomBindTransform, "skel:geomBindTransform"))
);
// clang-format on

MPlugArray PxrUsdTranslators_MeshWriter::mBlendShapesAnimWeightPlugs;

PxrUsdTranslators_MeshWriter::PxrUsdTranslators_MeshWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : UsdMayaPrimWriter(depNodeFn, usdPath, jobCtx)
{
    MayaUsd::TranslatorMeshWrite meshWriter(depNodeFn, GetUsdStage(), GetUsdPath(), GetDagPath());
    _usdPrim = meshWriter.usdMesh().GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdGeomMesh at path '%s'\n",
            meshWriter.usdMesh().GetPath().GetText())) {
        return;
    }
}

void PxrUsdTranslators_MeshWriter::PostExport()
{
    cleanupPrimvars();
    if (this->mBlendShapesAnimWeightPlugs.length() != 0) {
        // NOTE: (yliangsiew) Really, clearing it once is enough, but due to the constraints on what
        // should go in the WriteJobContext, there's not really a better place to put this cache for
        // now.
        this->mBlendShapesAnimWeightPlugs.clear();
    }
}

bool PxrUsdTranslators_MeshWriter::writeAnimatedMeshExtents(
    const MObject&     deformedMesh,
    const UsdTimeCode& usdTime)
{
    // NOTE: (yliangsiew) We also cache the animated extents out here; this
    // will be written at the SkelRoot level later on.
    TF_VERIFY(!deformedMesh.isNull());
    TF_VERIFY(deformedMesh.hasFn(MFn::kMesh));
    MStatus stat;
    MFnMesh fnMesh(deformedMesh, &stat);
    CHECK_MSTATUS_AND_RETURN(stat, false);
    unsigned int numVertices = fnMesh.numVertices();
    const float* meshPts = fnMesh.getRawPoints(&stat);
    CHECK_MSTATUS_AND_RETURN(stat, false);

    const GfVec3f* pVtMeshPts = reinterpret_cast<const GfVec3f*>(meshPts);
    VtVec3fArray   vtMeshPts(pVtMeshPts, pVtMeshPts + numVertices);
    VtVec3fArray   meshBBox(2);
    UsdGeomPointBased::ComputeExtent(vtMeshPts, &meshBBox);
    bool bStat = true;
    if (meshBBox != this->_prevMeshExtentsSample) {
        bStat = this->_writeJobCtx.UpdateSkelBindingsWithExtent(
            this->GetUsdStage(), meshBBox, usdTime);
    }
    this->_prevMeshExtentsSample = meshBBox;

    return bStat;
}

void PxrUsdTranslators_MeshWriter::Write(const UsdTimeCode& usdTime)
{
    UsdMayaPrimWriter::Write(usdTime);

    UsdGeomMesh primSchema(_usdPrim);
    writeMeshAttrs(usdTime, primSchema);
}

bool PxrUsdTranslators_MeshWriter::writeMeshAttrs(
    const UsdTimeCode& usdTime,
    UsdGeomMesh&       primSchema)
{
    MStatus status { MS::kSuccess };

    const UsdMayaJobExportArgs& exportArgs = _GetExportArgs();

    // Exporting reference object only once
    if (usdTime.IsDefault() && exportArgs.referenceObjectMode != UsdMayaJobExportArgsTokens->none) {
        UsdMayaMeshWriteUtils::exportReferenceMesh(
            primSchema,
            GetMayaObject(),
            exportArgs.referenceObjectMode == UsdMayaJobExportArgsTokens->defaultToMesh);
    }

    // Write UsdSkel skeletal skinning data first, since this will
    // determine whether we use the "input" or "final" mesh when exporting
    // mesh geometry. This should only be run once at default time.
    if (usdTime.IsDefault()) {
        const TfToken& exportSkin = exportArgs.exportSkin;
        if (exportSkin != UsdMayaJobExportArgsTokens->auto_
            && exportSkin != UsdMayaJobExportArgsTokens->explicit_) {

            _skelInputMesh = MObject();

        } else if (
            exportSkin == UsdMayaJobExportArgsTokens->explicit_
            && !UsdSkelRoot::Find(primSchema.GetPrim())) {

            _skelInputMesh = MObject();

        } else {

            SdfPath skelPath;
            _skelInputMesh = UsdMayaJointUtil::writeSkinningData(
                primSchema,
                GetUsdPath(),
                GetDagPath(),
                skelPath,
                exportArgs.stripNamespaces,
                _GetSparseValueWriter());

            if (!_skelInputMesh.isNull()) {
                // Add all skel primvars to the exclude set.
                // We don't want later processing to stomp on any of our data.
                _excludeColorSets.insert({ _tokens->skelJointIndices,
                                           _tokens->skelJointWeights,
                                           _tokens->skelGeomBindTransform });

                // Mark the bindings for post processing.
                _writeJobCtx.MarkSkelBindings(primSchema.GetPrim().GetPath(), skelPath, exportSkin);
            }
        }
    }

    // This is the mesh that "lives" at the end of this dag node. We should
    // always pull user-editable "sidecar" data like color sets and tags from
    // this mesh.
    MFnMesh finalMesh(GetDagPath(), &status);
    if (!status) {
        TF_RUNTIME_ERROR(
            "Failed to get final mesh at DAG path: %s", GetDagPath().fullPathName().asChar());
        return false;
    }

    // NOTE: (yliangsiew) We decide early-on if the mesh needs to have blendshapes exported, or not.
    // Since a user usually exports multiple meshes at the same time, it is inevitable that some
    // meshes will have blendshape export requested even though they do not have any blendshape
    // deformers driving them. So we double-check here first. Additionally, we check `finalMesh`
    // instead of `_skelInputMesh`, since the latter can end up being of type `kMeshData` (since it
    // could be a portion of the mesh rather than the full MObject node itself), which will segfault
    // MItDependencyGraph when initialized with it.
    bool shouldExportBlendShapes = exportArgs.exportBlendShapes;
    if (shouldExportBlendShapes
        && !UsdMayaUtil::CheckMeshUpstreamForBlendShapes(finalMesh.object())) {
        TF_WARN(
            "Blendshapes were requested to be exported for: %s, but none could be found.",
            GetDagPath().fullPathName().asChar());
        shouldExportBlendShapes = false;
    }

    // If exporting skinning, then geomMesh and finalMesh will be different
    // meshes. The general rule is to use geomMesh only for geometric data such
    // as vertices, faces, normals, but use finalMesh for UVs, color sets,
    // and user-defined tagging (e.g. subdiv tags).
    MObject geomMeshObj = _skelInputMesh.isNull() ? finalMesh.object() : _skelInputMesh;
    // do not pass these to functions that need access to geomMeshObj!
    // geomMesh.object() returns nil for meshes of type kMeshData.

    // NOTE: (yliangsiew) Because we need to write out the _actual_ base mesh,
    // not the deformed mesh as as result of blendshapes, if there is a blendshape
    // in the deform stack here, we walk past it to the original shape instead.
    // Also check if the mesh is a valid DG node (mesh geo subsets are
    // kMeshData in cases where a single mesh has multiple face
    // assignments to materials.) This also reduces the chance of something going wrong
    // by meshes that do not have blendshapes being affected by the wrong code path (such
    // as when exporting sparse frame ranges).
    if (shouldExportBlendShapes && geomMeshObj.hasFn(MFn::kDependencyNode)) {
        if (exportArgs.ignoreWarnings) {
            geomMeshObj = mayaFindOrigMeshFromBlendShapeTarget(geomMeshObj, nullptr);
        } else {
            MObjectArray intermediates;
            geomMeshObj = mayaFindOrigMeshFromBlendShapeTarget(geomMeshObj, &intermediates);
            status = mayaCheckIntermediateNodesForMeshEdits(intermediates);
            if (!status) {
                TF_RUNTIME_ERROR(
                    "Blendshapes failed pre-export checks at DAG path: %s",
                    GetDagPath().fullPathName().asChar());
                return false;
            }
        }
    }
    MFnMesh geomMesh(geomMeshObj, &status);
    if (!status) {
        TF_RUNTIME_ERROR(
            "Failed to get geom mesh at DAG path: %s", GetDagPath().fullPathName().asChar());
        return false;
    }

    // Write UsdSkelBlendShape data next. This also expands the _unionBBox member as
    // needed to encompass all the target blendshapes and writes it to the SkelRoot.
    bool bStat;
    if (shouldExportBlendShapes) {
        if (usdTime.IsDefault()) {
            _skelInputMesh = this->writeBlendShapeData(primSchema);
            if (_skelInputMesh.isNull()) {
                TF_WARN(
                    "Failed to write out initial blendshape data for the following: %s.",
                    GetDagPath().fullPathName().asChar());
                if (!exportArgs.ignoreWarnings) {
                    return false;
                }
            }
        } else {
            // NOTE: (yliangsiew) This is going to get called once for each time sampled.
            // Why do we do this later? Currently, it's because the block above needs to
            // run across _all_ meshes first, so that we build the entire array of
            // blendshapes being exported ahead of time (the above block is run for each
            // prim at the default time sample before running it on each anim. time
            // sample) and the plugs that they're associated with. Then here, now knowing
            // the entirety of the shapes that are meant to be exported, we can go ahead
            // and write the animation for each of them.
            if (!_skelInputMesh.isNull()) {
                bStat = this->writeBlendShapeAnimation(usdTime);
                if (!bStat) {
                    TF_WARN(
                        "Failed to write out blendshape animation for the following: %s.",
                        GetDagPath().fullPathName().asChar());
                    if (!exportArgs.ignoreWarnings) {
                        return bStat;
                    }
                }
                // NOTE: (yliangsiew) Also write out the "default" weights for the blendshapes,
                // to cover static blendshapes (i.e. non-animated targets.)
                bStat = this->writeBlendShapeAnimation(UsdTimeCode::Default());
            }
        }
    }

    // NOTE: (yliangsiew) Write out the final deformed mesh extents for each frame here.
    MDagPath deformedMeshDagPath = this->GetDagPath();
    MObject  deformedMesh = deformedMeshDagPath.node();
    bStat = this->writeAnimatedMeshExtents(deformedMesh, usdTime);
    if (!bStat) {
        return false;
    }

    // Return if usdTime does not match if shape is animated.
    if (usdTime.IsDefault() == isMeshAnimated()) {
        // If the shape is animated (based on the check above), only export time
        // samples. If the shape is non-animated, only export at the default
        // time.
        return true;
    }

    // Set mesh attrs ==========
    // Write points
    /*
     NOTE: (yliangsiew) Because we cannot assume that the first frame of export
     will have no blendshape targets activated, and we want to write out the
     points _without_ the influence of any blendshapes, (or any other deformers,
     for that matter; just that we haven't implemented support for other
     deformers yet, so until we do that, we can leave the effect of other
     deformers "baked" into the base/"pref" pose) we need to deactivate all the
     blendshape targets here _before_ writing out the data.
    */
    if (shouldExportBlendShapes) {
        // NOTE: (yliangsiew) Basically at this point: we have the deformed mesh, so
        // to find the "pref" pose (but _only_ taking blendshapes into account) we walk
        // the DG from the deformed mesh upstream to the end of the first blendshape deformer
        // and query the mesh data from its inputGeom plug.
        MItDependencyGraph itDg(
            deformedMesh,
            MFn::kInvalid,
            MItDependencyGraph::kUpstream,
            MItDependencyGraph::kDepthFirst,
            MItDependencyGraph::kPlugLevel,
            &status);
        MObject      upstreamBlendShape = MObject::kNullObj;
        unsigned int idxGeo = 0;
        for (; !itDg.isDone(); itDg.next()) {
            MObject curNode = itDg.thisNode();
            if (!curNode.hasFn(MFn::kBlendShape)) {
                itDg.next();
                continue;
            }
            upstreamBlendShape = curNode;
            MPlug curPlug = itDg.thisPlug(); // NOTE: (yliangsiew) This _should_ be the
                                             // outputGeometry[x] plug that it's connected to.
            TF_VERIFY(curPlug.isElement());
            idxGeo = curPlug.logicalIndex();
            break;
        }

        if (!upstreamBlendShape.hasFn(MFn::kBlendShape)) {
            TF_WARN("Blendshapes were requested to be exported, but no upstream blendshapes could "
                    "be found.");
            UsdMayaMeshWriteUtils::writePointsData(
                geomMesh, primSchema, usdTime, _GetSparseValueWriter());
        } else {
            MFnDependencyNode fnNode(upstreamBlendShape, &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            TF_VERIFY(fnNode.hasAttribute("input"));
            MPlug plgBlendShapeInputs = fnNode.findPlug("input", &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            MPlug plgBlendShapeInput = plgBlendShapeInputs.elementByLogicalIndex(idxGeo);
            MPlug plgBlendShapeInputGeometry
                = UsdMayaUtil::FindChildPlugWithName(plgBlendShapeInput, "inputGeometry");
            MDataHandle dhInputGeo
                = plgBlendShapeInputGeometry
                      .asMDataHandle(); // NOTE: (yliangsiew) This should be the pref mesh.
            TF_VERIFY(dhInputGeo.type() == MFnData::kMesh);
            MObject inputGeo = dhInputGeo.asMesh();
            TF_VERIFY(inputGeo.hasFn(MFn::kMesh));

            // NOTE: (yliangsiew) Because the `geomMesh` fnset cached the previous MObject (from the
            // inputGeom skinCluster plug), the point positions reported will be out-of-date even
            // after we disable blendshape deformers. So this code re-acquires the mesh in question
            // to write out the points for, and then we actually write it out.
            MFnMesh fnMesh(inputGeo, &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            UsdMayaMeshWriteUtils::writePointsData(
                fnMesh, primSchema, usdTime, _GetSparseValueWriter());
        }
    } else {
        // TODO: (yliangsiew) Any other deformers that get implemented in the future will have to
        // make sure that they don't just enter this scope; otherwise, their deformed point
        // positions will get "baked" into the pref pose as well.
        UsdMayaMeshWriteUtils::writePointsData(
            geomMesh, primSchema, usdTime, _GetSparseValueWriter());
    }

    // Write faceVertexIndices
    UsdMayaMeshWriteUtils::writeFaceVertexIndicesData(
        geomMesh, primSchema, usdTime, _GetSparseValueWriter());

    // Read subdiv scheme tagging. If not set, we default to defaultMeshScheme
    // flag (this is specified by the job args but defaults to catmullClark).
    TfToken sdScheme = UsdMayaMeshWriteUtils::getSubdivScheme(finalMesh);
    if (sdScheme.IsEmpty()) {
        sdScheme = exportArgs.defaultMeshScheme;
    }
    primSchema.CreateSubdivisionSchemeAttr(VtValue(sdScheme), true);

    if (sdScheme == UsdGeomTokens->none) {
        // Polygonal mesh - export normals.
        bool emitNormals = true; // Default to emitting normals if no tagging.
        UsdMayaMeshReadUtils::getEmitNormalsTag(finalMesh, &emitNormals);
        if (emitNormals) {
            UsdMayaMeshWriteUtils::writeNormalsData(
                geomMesh, primSchema, usdTime, _GetSparseValueWriter());
        }
    } else {
        // Subdivision surface - export subdiv-specific attributes.
        UsdMayaMeshWriteUtils::writeSubdivInterpBound(
            finalMesh, primSchema, _GetSparseValueWriter());

        UsdMayaMeshWriteUtils::writeSubdivFVLinearInterpolation(
            finalMesh, primSchema, _GetSparseValueWriter());

        UsdMayaMeshWriteUtils::assignSubDivTagsToUSDPrim(
            finalMesh, primSchema, _GetSparseValueWriter());
    }

    // Holes - we treat InvisibleFaces as holes
    UsdMayaMeshWriteUtils::writeInvisibleFacesData(finalMesh, primSchema, _GetSparseValueWriter());

    // == Write UVSets as Vec2f Primvars
    if (exportArgs.exportMeshUVs) {
        UsdMayaMeshWriteUtils::writeUVSetsAsVec2fPrimvars(
            finalMesh,
            primSchema,
            usdTime,
            _GetSparseValueWriter(),
            exportArgs.preserveUVSetNames,
            exportArgs.remapUVSetsTo);
    }

    // == Gather ColorSets
    std::vector<std::string> colorSetNames;
    if (exportArgs.exportColorSets) {
        MStringArray mayaColorSetNames;
        status = finalMesh.getColorSetNames(mayaColorSetNames);
        colorSetNames.reserve(mayaColorSetNames.length());
        for (unsigned int i = 0; i < mayaColorSetNames.length(); i++) {
            colorSetNames.emplace_back(mayaColorSetNames[i].asChar());
        }
    }

    std::set<std::string> colorSetNamesSet(colorSetNames.begin(), colorSetNames.end());

    VtArray<GfVec3f> shadersRGBData;
    VtArray<float>   shadersAlphaData;
    TfToken          shadersInterpolation;
    VtArray<int>     shadersAssignmentIndices;

    // If we're exporting displayColor or we have color sets, gather colors and
    // opacities from the shaders assigned to the mesh and/or its faces.
    // If we find a displayColor color set, the shader colors and opacities
    // will be used to fill in unauthored/unpainted faces in the color set.
    if (exportArgs.exportDisplayColor || !colorSetNames.empty()) {
        UsdMayaUtil::GetLinearShaderColor(
            finalMesh,
            &shadersRGBData,
            &shadersAlphaData,
            &shadersInterpolation,
            &shadersAssignmentIndices);
    }

    for (const std::string& colorSetName : colorSetNames) {

        if (_excludeColorSets.count(colorSetName) > 0)
            continue;

        bool isDisplayColor = false;

        if (colorSetName == UsdMayaMeshPrimvarTokens->DisplayColorColorSetName.GetString()) {
            if (!exportArgs.exportDisplayColor) {
                continue;
            }
            isDisplayColor = true;
        }

        if (colorSetName == UsdMayaMeshPrimvarTokens->DisplayOpacityColorSetName.GetString()) {
            TF_WARN(
                "Mesh \"%s\" has a color set named \"%s\", "
                "which is a reserved Primvar name in USD. Skipping...",
                finalMesh.fullPathName().asChar(),
                UsdMayaMeshPrimvarTokens->DisplayOpacityColorSetName.GetText());
            continue;
        }

        VtArray<GfVec3f>              RGBData;
        VtArray<float>                AlphaData;
        TfToken                       interpolation;
        VtArray<int>                  assignmentIndices;
        MFnMesh::MColorRepresentation colorSetRep;
        bool                          clamped = false;

        if (!UsdMayaMeshWriteUtils::getMeshColorSetData(
                finalMesh,
                MString(colorSetName.c_str()),
                isDisplayColor,
                shadersRGBData,
                shadersAlphaData,
                shadersAssignmentIndices,
                &RGBData,
                &AlphaData,
                &interpolation,
                &assignmentIndices,
                &colorSetRep,
                &clamped)) {
            TF_WARN(
                "Unable to retrieve colorSet data: %s on mesh: %s. "
                "Skipping...",
                colorSetName.c_str(),
                finalMesh.fullPathName().asChar());
            continue;
        }

        if (isDisplayColor) {
            // We tag the resulting displayColor/displayOpacity primvar as
            // authored to make sure we reconstruct the color set on import.
            UsdMayaMeshWriteUtils::addDisplayPrimvars(
                primSchema,
                usdTime,
                colorSetRep,
                RGBData,
                AlphaData,
                interpolation,
                assignmentIndices,
                clamped,
                true,
                _GetSparseValueWriter());
        } else {
            const std::string sanitizedName = UsdMayaUtil::SanitizeColorSetName(colorSetName);
            // if our sanitized name is different than our current one and the
            // sanitized name already exists, it means 2 things are trying to
            // write to the same primvar.  warn and continue.
            if (colorSetName != sanitizedName && colorSetNamesSet.count(sanitizedName) > 0) {
                TF_WARN(
                    "Skipping colorSet '%s' as the colorSet '%s' exists as "
                    "well.",
                    colorSetName.c_str(),
                    sanitizedName.c_str());
                continue;
            }

            TfToken colorSetNameToken = TfToken(sanitizedName);
            if (colorSetRep == MFnMesh::kAlpha) {
                UsdMayaMeshWriteUtils::createAlphaPrimVar(
                    primSchema,
                    colorSetNameToken,
                    usdTime,
                    AlphaData,
                    interpolation,
                    assignmentIndices,
                    clamped,
                    _GetSparseValueWriter());
            } else if (colorSetRep == MFnMesh::kRGB) {
                UsdMayaMeshWriteUtils::createRGBPrimVar(
                    primSchema,
                    colorSetNameToken,
                    usdTime,
                    RGBData,
                    interpolation,
                    assignmentIndices,
                    clamped,
                    _GetSparseValueWriter());
            } else if (colorSetRep == MFnMesh::kRGBA) {
                UsdMayaMeshWriteUtils::createRGBAPrimVar(
                    primSchema,
                    colorSetNameToken,
                    usdTime,
                    RGBData,
                    AlphaData,
                    interpolation,
                    assignmentIndices,
                    clamped,
                    _GetSparseValueWriter());
            }
        }
    }

    // UsdMayaMeshWriteUtils::addDisplayPrimvars() will only author displayColor and displayOpacity
    // if no authored opinions exist, so the code below only has an effect if
    // we did NOT find a displayColor color set above.
    if (exportArgs.exportDisplayColor) {
        // Using the shader default values (an alpha of zero, in particular)
        // results in Gprims rendering the same way in usdview as they do in
        // Maya (i.e. unassigned components are invisible).
        //
        // Since these colors come from the shaders and not a colorset, we are
        // not adding the clamp attribute as custom data. We also don't need to
        // reconstruct a color set from them on import since they originated
        // from the bound shader(s), so the authored flag is set to false.
        UsdMayaMeshWriteUtils::addDisplayPrimvars(
            primSchema,
            usdTime,
            MFnMesh::kRGBA,
            shadersRGBData,
            shadersAlphaData,
            shadersInterpolation,
            shadersAssignmentIndices,
            false,
            false,
            _GetSparseValueWriter());
    }

#if MAYA_API_VERSION >= 20220000

    if (exportArgs.exportComponentTags) {
        UsdMayaMeshWriteUtils::exportComponentTags(primSchema, GetMayaObject());
    }

#endif

    return true;
}

bool PxrUsdTranslators_MeshWriter::ExportsGprims() const { return true; }

bool PxrUsdTranslators_MeshWriter::isMeshAnimated() const
{
    // Note that _HasAnimCurves() as computed by UsdMayaTransformWriter is
    // whether the finalMesh is animated.
    return _skelInputMesh.isNull() ? _HasAnimCurves() : false;
}

void PxrUsdTranslators_MeshWriter::cleanupPrimvars()
{
    if (!isMeshAnimated()) {
        // Based on how setPrimvar() works, the cleanup phase doesn't apply to
        // non-animated meshes.
        return;
    }

    // On animated meshes, we forced an extra value (the "unassigned" or
    // "unauthored" value) into index 0 of any indexed primvar's values array.
    // If the indexed primvar doesn't need the unassigned value (because all
    // of the indices are assigned), then we can remove the unassigned value
    // and shift all the indices down.
    const UsdGeomPrimvarsAPI pvAPI(GetUsdPrim());
    for (const UsdGeomPrimvar& primvar : pvAPI.GetPrimvars()) {
        if (!primvar) {
            continue;
        }

        // Cleanup phase applies only to indexed primvars.
        // Unindexed primvars were written directly without modification.
        if (!primvar.IsIndexed()) {
            continue;
        }

        // If the unauthoredValueIndex is 0, that means we purposefully set it
        // to indicate that at least one time sample has unauthored values.
        const int unauthoredValueIndex = primvar.GetUnauthoredValuesIndex();
        if (unauthoredValueIndex == 0) {
            continue;
        }

        // If the unauthoredValueIndex wasn't 0 above, it must be -1 (the
        // fallback value in USD).
        if (!TF_VERIFY(unauthoredValueIndex == -1)) {
            return;
        }

        // Since the unauthoredValueIndex is -1, we never explicitly set it,
        // meaning that none of the samples contain an unassigned value.
        // Since we authored the unassigned value as index 0 in each primvar,
        // we can eliminate it now from all time samples.
        if (const UsdAttribute attr = primvar.GetAttr()) {
            VtValue val;
            if (attr.Get(&val, UsdTimeCode::Default())) {
                const VtValue newVal = UsdMayaUtil::popFirstValue(val);
                if (!newVal.IsEmpty()) {
                    attr.Set(newVal, UsdTimeCode::Default());
                }
            }
            std::vector<double> timeSamples;
            if (attr.GetTimeSamples(&timeSamples)) {
                for (const double& t : timeSamples) {
                    if (attr.Get(&val, t)) {
                        const VtValue newVal = UsdMayaUtil::popFirstValue(val);
                        if (!newVal.IsEmpty()) {
                            attr.Set(newVal, t);
                        }
                    }
                }
            }
        }

        // We then need to shift all the indices down one to account for index
        // 0 being eliminated.
        if (const UsdAttribute attr = primvar.GetIndicesAttr()) {
            VtIntArray val;
            if (attr.Get(&val, UsdTimeCode::Default())) {
                attr.Set(UsdMayaUtil::shiftIndices(val, -1), UsdTimeCode::Default());
            }
            std::vector<double> timeSamples;
            if (attr.GetTimeSamples(&timeSamples)) {
                for (const double& t : timeSamples) {
                    if (attr.Get(&val, t)) {
                        attr.Set(UsdMayaUtil::shiftIndices(val, -1), t);
                    }
                }
            }
        }
    }

    // The function checks within itself if it is required to be called, so no conditional check
    // here
    MakeSingleSamplesStatic();
}

PXR_NAMESPACE_CLOSE_SCOPE
