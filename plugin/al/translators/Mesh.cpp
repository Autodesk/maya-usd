
//
// Copyright 2017 Animal Logic
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

#include "Mesh.h"

#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/Metadata.h"
#include "AL/usdmaya/fileio/AnimationTranslator.h"
#include "AL/usdmaya/fileio/translators/DagNodeTranslator.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/utils/DiffPrimVar.h"
#include "AL/usdmaya/utils/MeshUtils.h"
#include "AL/usdmaya/utils/Utils.h"
#include "CommonTranslatorOptions.h"

#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/mesh.h>

#include <maya/MFileIO.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MIntArray.h>
#include <maya/MNodeClass.h>
#include <maya/MProfiler.h>
#include <maya/MVectorArray.h>

namespace {
const int _meshProfilerCategory = MProfiler::addCategory(
#if MAYA_API_VERSION >= 20190000
    "Mesh",
    "Mesh"
#else
    "Mesh"
#endif
);
} // namespace

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

MObject Mesh::m_visible = MObject::kNullObj;

AL_USDMAYA_DEFINE_TRANSLATOR(Mesh, PXR_NS::UsdGeomMesh)

//----------------------------------------------------------------------------------------------------------------------
MStatus Mesh::initialize()
{
    MNodeClass fn("transform");
    MStatus    status;

    m_visible = fn.attribute("v", &status);
    AL_MAYA_CHECK_ERROR(status, "Unable to add `visibility` attribute");

    // Initialise all the class plugs
    return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus Mesh::import(const UsdPrim& prim, MObject& parent, MObject& createdObj)
{
    MProfilingScope profilerScope(_meshProfilerCategory, MProfiler::kColorE_L3, "Import");

    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("Mesh::import prim=%s\n", prim.GetPath().GetText());

    const UsdGeomMesh mesh(prim);

    TranslatorContextPtr ctx = context();
    UsdTimeCode          timeCode = (ctx && ctx->getForceDefaultRead()) ? UsdTimeCode::Default()
                                                               : UsdTimeCode::EarliestTime();

    bool    parentUnmerged = false;
    TfToken val;
    if (prim.GetParent().GetMetadata(AL::usdmaya::Metadata::mergedTransform, &val)) {
        parentUnmerged = (val == AL::usdmaya::Metadata::unmerged);
    }
    MString dagName = prim.GetName().GetString().c_str();
    if (!parentUnmerged) {
        dagName += "Shape";
    }

    AL::usdmaya::utils::MeshImportContext importContext(mesh, parent, dagName, timeCode);
    importContext.applyVertexNormals();
    importContext.applyHoleFaces();
    importContext.applyVertexCreases();
    importContext.applyEdgeCreases();

    MObject initialShadingGroup;
    DagNodeTranslator::initialiseDefaultShadingGroup(initialShadingGroup);
    // Apply default material to Shape
    MStatus status;
    MFnSet  fn(initialShadingGroup, &status);
    AL_MAYA_CHECK_ERROR(status, "Unable to attach MfnSet to initialShadingGroup");

    createdObj = importContext.getPolyShape();
    fn.addMember(createdObj);
    importContext.applyUVs();
    importContext.applyColourSetData();

    if (ctx) {
        ctx->addExcludedGeometry(prim.GetPath());
        ctx->insertItem(prim, createdObj);
    }

    TfToken vis = mesh.ComputeVisibility(timeCode);
    // if the visibility token is not `invisible` then, make it visible
    DgNodeTranslator::setBool(parent, m_visible, vis != UsdGeomTokens->invisible);

    return MStatus::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
UsdPrim Mesh::exportObject(
    UsdStageRefPtr        stage,
    MDagPath              dagPath,
    const SdfPath&        usdPath,
    const ExporterParams& params)
{
    if (!params.getBool(GeometryExportOptions::kMeshes))
        return UsdPrim();

    UsdGeomMesh mesh = UsdGeomMesh::Define(stage, usdPath);

    auto compaction = (AL::usdmaya::utils::MeshExportContext::CompactionLevel)params.getInt(
        GeometryExportOptions::kCompactionLevel);

    AL::usdmaya::utils::MeshExportContext context(
        dagPath,
        mesh,
        params.m_timeCode,
        false,
        compaction,
        params.getBool(GeometryExportOptions::kReverseOppositeNormals));
    if (context) {
        UsdAttribute pointsAttr = mesh.GetPointsAttr();
        if (params.m_animTranslator && AnimationTranslator::isAnimatedMesh(dagPath)) {
            params.m_animTranslator->addMesh(dagPath, pointsAttr);
        }

        if (params.getBool(GeometryExportOptions::kMeshPoints)) {
            context.copyVertexData(context.timeCode());
        }
        if (params.getBool(GeometryExportOptions::kMeshExtents)) {
            context.copyExtentData(context.timeCode());
        }
        if (params.getBool(GeometryExportOptions::kMeshConnects)) {
            context.copyFaceConnectsAndPolyCounts();
        }
        if (params.getBool(GeometryExportOptions::kMeshHoles)) {
            context.copyInvisibleHoles();
        }
        if (params.getBool(GeometryExportOptions::kMeshUvs)) {
            context.copyUvSetData();
        }
        if (params.getBool(GeometryExportOptions::kMeshNormals)) {
            context.copyNormalData(
                context.timeCode(), params.getBool(GeometryExportOptions::kNormalsAsPrimvars));
        }
        if (params.getBool(GeometryExportOptions::kMeshColours)) {
            context.copyColourSetData();
        }
        if (params.getBool(GeometryExportOptions::kMeshVertexCreases)) {
            context.copyCreaseVertices();
        }
        if (params.getBool(GeometryExportOptions::kMeshEdgeCreases)) {
            context.copyCreaseEdges();
        }
        if (params.getBool(GeometryExportOptions::kMeshPointsAsPref)) {
            context.copyBindPoseData(context.timeCode());
        }

        // pick up any additional attributes attached to the mesh node (these will be added
        // alongside the transform attributes)
        if (params.m_dynamicAttributes) {
            UsdPrim prim = mesh.GetPrim();
            DgNodeTranslator::copyDynamicAttributes(dagPath.node(), prim);
        }
    }
    return mesh.GetPrim();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus Mesh::tearDown(const SdfPath& path)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MeshTranslator::tearDown prim=%s\n", path.GetText());

    context()->removeItems(path);
    context()->removeExcludedGeometry(path);
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus Mesh::update(const UsdPrim& path) { return MS::kSuccess; }

//----------------------------------------------------------------------------------------------------------------------
MStatus Mesh::preTearDown(UsdPrim& prim)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg("MeshTranslator::preTearDown prim=%s\n", prim.GetPath().GetText());
    if (!prim.IsValid()) {
        TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("Mesh::preTearDown prim invalid\n");
        return MS::kFailure;
    }
    TranslatorBase::preTearDown(prim);

    /* TODO
     * This block was put in since writeEdits modifies USD and thus triggers the OnObjectsChanged
     * callback which will then tearDown the this Mesh prim. The writeEdits method will then
     * continue attempting to copy maya mesh data to USD but will end up crashing since the maya
     * mesh has now been removed by the tearDown.
     *
     * I have tried turning off the TfNotice but I get the 'Detected usd threading violation.
     * Concurrent changes to layer(s) composed' error.
     *
     * This crash and error seems to be happening mainly when switching out a variant that contains
     * a Mesh, and that Mesh has been force translated into Maya.
     */
    AL::usdmaya::utils::BlockNotifications
        blockNow; // don't use TfNotice::Block, render delegates need to know about the change
    // Write the overrides back to the path it was imported at
    MObjectHandle obj;
    context()->getMObject(prim, obj, MFn::kInvalid);
    if (obj.isValid()) {
        MFnDagNode fn(obj.object());
        MDagPath   path;
        fn.getPath(path);
        MStatus status;
        MFnMesh fnMesh(path, &status);
        AL_MAYA_CHECK_ERROR(
            status, MString("unable to attach function set to mesh: ") + path.fullPathName());

        UsdGeomMesh geomPrim(prim);
        writeEdits(path, geomPrim, kPerformDiff | kDynamicAttributes);
    } else {
        TF_DEBUG(ALUSDMAYA_TRANSLATORS)
            .Msg(
                "Unable to find the corresponding Maya Handle at prim path '%s'\n",
                prim.GetPath().GetText());
        return MS::kFailure;
    }
    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
void Mesh::writeEdits(MDagPath& dagPath, UsdGeomMesh& geomPrim, uint32_t options)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg("MeshTranslator::writing edits to prim='%s'\n", geomPrim.GetPath().GetText());
    UsdTimeCode                           t = UsdTimeCode::Default();
    AL::usdmaya::utils::MeshExportContext context(dagPath, geomPrim, t, options & kPerformDiff);
    if (context) {
        context.copyVertexData(t);
        context.copyExtentData(t);
        context.copyNormalData(t);
        context.copyFaceConnectsAndPolyCounts();
        context.copyInvisibleHoles();
        context.copyCreaseVertices();
        context.copyCreaseEdges();
        context.copyUvSetData();
        context.copyColourSetData();
        context.copyBindPoseData(t);
        if (options & kDynamicAttributes) {
            UsdPrim prim = geomPrim.GetPrim();
            DgNodeTranslator::copyDynamicAttributes(dagPath.node(), prim);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
