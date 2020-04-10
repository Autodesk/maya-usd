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
// Modifications copyright (C) 2020 Autodesk
//
#include "meshWriteUtils.h"

#include <mayaUsd/fileio/utils/adaptor.h>
#include <mayaUsd/fileio/utils/roundTripUtil.h>
#include <mayaUsd/fileio/utils/writeUtil.h>

#include <maya/MFnSet.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MItMeshFaceVertex.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MStatus.h>
#include <maya/MUintArray.h>

#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/pointBased.h>
#include <pxr/usd/usdGeom/primvar.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdUtils/pipeline.h>

#include <mayaUsd/base/debugCodes.h>
#include <mayaUsd/utils/colorSpace.h>
#include <mayaUsd/utils/util.h>

PXR_NAMESPACE_OPEN_SCOPE

// These tokens are supported Maya attributes used for Mesh surfaces
TF_DEFINE_PRIVATE_TOKENS(
    _meshTokens,

    // we capitalize this because it doesn't correspond to an actual attribute
    (USD_EmitNormals)

    // This is a value for face varying interpolate boundary from OpenSubdiv 2
    // that we translate to face varying linear interpolation for OpenSubdiv 3.
    (alwaysSharp)

    // This token is deprecated as it is from OpenSubdiv 2 and the USD
    // schema now conforms to OpenSubdiv 3, but we continue to look for it
    // and translate to the equivalent new value for backwards compatibility.
    (USD_faceVaryingInterpolateBoundary)
);

namespace
{
    // XXX: Note that this function is not exposed publicly since the USD schema
    // has been updated to conform to OpenSubdiv 3. We still look for this attribute
    // on Maya nodes specifying this value from OpenSubdiv 2, but we translate the
    // value to OpenSubdiv 3. This is to support legacy assets authored against
    // OpenSubdiv 2.
    static
    TfToken
    getOsd2FVInterpBoundary(const MFnMesh& mesh)
    {
        TfToken sdFVInterpBound;

        MPlug plug = mesh.findPlug(MString(
                _meshTokens->USD_faceVaryingInterpolateBoundary.GetText()));
        if (!plug.isNull()) {
            sdFVInterpBound = TfToken(plug.asString().asChar());

            // Translate OSD2 values to OSD3.
            if (sdFVInterpBound == UsdGeomTokens->bilinear) {
                sdFVInterpBound = UsdGeomTokens->all;
            } else if (sdFVInterpBound == UsdGeomTokens->edgeAndCorner) {
                sdFVInterpBound = UsdGeomTokens->cornersPlus1;
            } else if (sdFVInterpBound == _meshTokens->alwaysSharp) {
                sdFVInterpBound = UsdGeomTokens->boundaries;
            } else if (sdFVInterpBound == UsdGeomTokens->edgeOnly) {
                sdFVInterpBound = UsdGeomTokens->none;
            }
        } else {
            plug = mesh.findPlug(MString("rman__torattr___subdivFacevaryingInterp"));
            if (!plug.isNull()) {
                switch(plug.asInt()) {
                    case 0:
                        sdFVInterpBound = UsdGeomTokens->all;
                        break;
                    case 1:
                        sdFVInterpBound = UsdGeomTokens->cornersPlus1;
                        break;
                    case 2:
                        sdFVInterpBound = UsdGeomTokens->none;
                        break;
                    case 3:
                        sdFVInterpBound = UsdGeomTokens->boundaries;
                        break;
                    default:
                        break;
                }
            }
        }

        return sdFVInterpBound;
    }

    void
    compressCreases( const std::vector<int>& inCreaseIndices,
                     const std::vector<float>& inCreaseSharpnesses,
                     std::vector<int>* creaseLengths,
                     std::vector<int>* creaseIndices,
                     std::vector<float>* creaseSharpnesses)
    {
        // Process vertex pairs.
        for (size_t i = 0; i < inCreaseSharpnesses.size(); i++) {
            const float sharpness = inCreaseSharpnesses[i];
            const int v0 = inCreaseIndices[i*2+0];
            const int v1 = inCreaseIndices[i*2+1];
            // Check if this edge represents a continuation of the last crease.
            if (!creaseIndices->empty() && v0 == creaseIndices->back()
                && sharpness == creaseSharpnesses->back()) {
                // Extend the last crease.
                creaseIndices->push_back(v1);
                creaseLengths->back()++;
            } else {
                // Start a new crease.
                creaseIndices->push_back(v0);
                creaseIndices->push_back(v1);
                creaseLengths->push_back(2);
                creaseSharpnesses->push_back(sharpness);
            }
        }
    }

} // anonymous namespace

bool
UsdMayaMeshUtil::getMeshNormals(const MFnMesh& mesh,
                                VtArray<GfVec3f>* normalsArray,
                                TfToken* interpolation)
{
    MStatus status{MS::kSuccess};

    // Sanity check first to make sure we can get this mesh's normals.
    const int numNormals = mesh.numNormals(&status);
    if (status != MS::kSuccess || numNormals == 0) {
        return false;
    }

    // Using itFV.getNormal() does not always give us the right answer, so
    // instead we have to use itFV.normalId() and use that to index into the
    // normals.
    MFloatVectorArray mayaNormals;
    status = mesh.getNormals(mayaNormals);
    if (status != MS::kSuccess) {
        return false;
    }

    const unsigned int numFaceVertices = mesh.numFaceVertices(&status);
    if (status != MS::kSuccess) {
        return false;
    }

    normalsArray->resize(numFaceVertices);
    *interpolation = UsdGeomTokens->faceVarying;

    MItMeshFaceVertex itFV(mesh.object());
    unsigned int fvi = 0;
    for (itFV.reset(); !itFV.isDone(); itFV.next(), ++fvi) {
        int normalId = itFV.normalId();
        if (normalId < 0 ||
                static_cast<size_t>(normalId) >= mayaNormals.length()) {
            return false;
        }

        MFloatVector normal = mayaNormals[normalId];
        (*normalsArray)[fvi][0] = normal[0];
        (*normalsArray)[fvi][1] = normal[1];
        (*normalsArray)[fvi][2] = normal[2];
    }

    return true;
}

// This can be customized for specific pipelines.
// We first look for the USD string attribute, and if not present we look for
// the RenderMan for Maya int attribute.
// XXX Maybe we should come up with a OSD centric nomenclature ??
TfToken
UsdMayaMeshUtil::getSubdivScheme(const MFnMesh& mesh)
{
    // Try grabbing the value via the adaptor first.
    TfToken schemeToken;
    UsdMayaAdaptor(mesh.object())
            .GetSchemaOrInheritedSchema<UsdGeomMesh>()
            .GetAttribute(UsdGeomTokens->subdivisionScheme)
            .Get<TfToken>(&schemeToken);

    // Fall back to the RenderMan for Maya attribute.
    if (schemeToken.IsEmpty()) {
        MPlug plug = mesh.findPlug(MString("rman__torattr___subdivScheme"));
        if (!plug.isNull()) {
            switch (plug.asInt()) {
                case 0:
                    schemeToken = UsdGeomTokens->catmullClark;
                    break;
                case 1:
                    schemeToken = UsdGeomTokens->loop;
                    break;
                default:
                    break;
            }
        }
    }

    if (schemeToken.IsEmpty()) {
        return TfToken();
    } else if (schemeToken != UsdGeomTokens->none &&
               schemeToken != UsdGeomTokens->catmullClark &&
               schemeToken != UsdGeomTokens->loop &&
               schemeToken != UsdGeomTokens->bilinear) {
        TF_RUNTIME_ERROR(
                "Unsupported subdivision scheme: %s on mesh: %s",
                schemeToken.GetText(), mesh.fullPathName().asChar());
        return TfToken();
    }

    return schemeToken;
}

// This can be customized for specific pipelines.
// We first look for the USD string attribute, and if not present we look for
// the RenderMan for Maya int attribute.
// XXX Maybe we should come up with a OSD centric nomenclature ??
TfToken UsdMayaMeshUtil::getSubdivInterpBoundary(const MFnMesh& mesh)
{
    // Try grabbing the value via the adaptor first.
    TfToken interpBoundaryToken;
    UsdMayaAdaptor(mesh.object())
            .GetSchemaOrInheritedSchema<UsdGeomMesh>()
            .GetAttribute(UsdGeomTokens->interpolateBoundary)
            .Get<TfToken>(&interpBoundaryToken);

    // Fall back to the RenderMan for Maya attr.
    if (interpBoundaryToken.IsEmpty()) {
        MPlug plug = mesh.findPlug(MString("rman__torattr___subdivInterp"));
        if (!plug.isNull()) {
            switch (plug.asInt()) {
                case 0:
                    interpBoundaryToken = UsdGeomTokens->none;
                    break;
                case 1:
                    interpBoundaryToken = UsdGeomTokens->edgeAndCorner;
                    break;
                case 2:
                    interpBoundaryToken = UsdGeomTokens->edgeOnly;
                    break;
                default:
                    break;
            }
        }
    }

    if (interpBoundaryToken.IsEmpty()) {
        return TfToken();
    } else if (interpBoundaryToken != UsdGeomTokens->none &&
               interpBoundaryToken != UsdGeomTokens->edgeAndCorner &&
               interpBoundaryToken != UsdGeomTokens->edgeOnly) {
        TF_RUNTIME_ERROR(
                "Unsupported interpolate boundary setting: %s on mesh: %s",
                interpBoundaryToken.GetText(), mesh.fullPathName().asChar());
        return TfToken();
    }

    return interpBoundaryToken;
}

TfToken UsdMayaMeshUtil::getSubdivFVLinearInterpolation(const MFnMesh& mesh)
{
    // Try grabbing the value via the adaptor first.
    TfToken sdFVLinearInterpolation;
    UsdMayaAdaptor(mesh.object())
            .GetSchemaOrInheritedSchema<UsdGeomMesh>()
            .GetAttribute(UsdGeomTokens->faceVaryingLinearInterpolation)
            .Get<TfToken>(&sdFVLinearInterpolation);

    // If the OpenSubdiv 3-style face varying linear interpolation value
    // wasn't specified, fall back to the old OpenSubdiv 2-style face
    // varying interpolate boundary value if we have that.
    if (sdFVLinearInterpolation.IsEmpty()) {
        sdFVLinearInterpolation = getOsd2FVInterpBoundary(mesh);
    }

    if (!sdFVLinearInterpolation.IsEmpty() &&
            sdFVLinearInterpolation != UsdGeomTokens->all &&
            sdFVLinearInterpolation != UsdGeomTokens->none &&
            sdFVLinearInterpolation != UsdGeomTokens->boundaries &&
            sdFVLinearInterpolation != UsdGeomTokens->cornersOnly &&
            sdFVLinearInterpolation != UsdGeomTokens->cornersPlus1 &&
            sdFVLinearInterpolation != UsdGeomTokens->cornersPlus2) {
        TF_RUNTIME_ERROR(
                "Unsupported face-varying linear interpolation: %s "
                "on mesh: %s",
                sdFVLinearInterpolation.GetText(),
                mesh.fullPathName().asChar());
        return TfToken();
    }

    return sdFVLinearInterpolation;
}

bool
UsdMayaMeshUtil::isMeshValid(const MDagPath& dagPath)
{
    MStatus status{MS::kSuccess};

    // sanity checks
    MFnMesh lMesh(dagPath, &status);

    if (!status) {
        TF_RUNTIME_ERROR(
                "MFnMesh() failed for mesh at DAG path: %s",
                dagPath.fullPathName().asChar());
        return false;
    }

    const auto numVertices = lMesh.numVertices();
    const auto numPolygons = lMesh.numPolygons();

    if (numVertices < 3 && numVertices > 0)
    {
        TF_RUNTIME_ERROR(
                "%s is not a valid mesh, because it only has %u points,",
                lMesh.fullPathName().asChar(), numVertices);
    }
    if (numPolygons == 0)
    {
        TF_WARN("%s has no polygons.", lMesh.fullPathName().asChar());
    }
    return true;
}

void
UsdMayaMeshUtil::exportReferenceMesh(UsdGeomMesh& primSchema, MObject obj)
{
    MStatus status{MS::kSuccess};

    MFnDependencyNode dNode(obj, &status);
    if (!status) {
        return;
    }

    MPlug referencePlug = dNode.findPlug("referenceObject", &status);
    if (!status || referencePlug.isNull()) {
        return;
    }

    MPlugArray conns;
    referencePlug.connectedTo(conns, true, false);
    if (conns.length() == 0) {
        return;
    }

    MObject referenceObject = conns[0].node();
    if (!referenceObject.hasFn(MFn::kMesh)) {
        return;
    }

    MFnMesh referenceMesh(referenceObject, &status);
    if (!status) {
        return;
    }

    const float* mayaRawPoints = referenceMesh.getRawPoints(&status);
    const int numVertices = referenceMesh.numVertices();
    VtArray<GfVec3f> points(numVertices);
    for (int i = 0; i < numVertices; ++i) {
        const int floatIndex = i * 3;
        points[i].Set(mayaRawPoints[floatIndex],
                        mayaRawPoints[floatIndex + 1],
                        mayaRawPoints[floatIndex + 2]);
    }

    UsdGeomPrimvar primVar = primSchema.CreatePrimvar(
        UsdUtilsGetPrefName(),
        SdfValueTypeNames->Point3fArray,
        UsdGeomTokens->varying);

    if (!primVar) {
        return;
    }

    primVar.GetAttr().Set(VtValue(points));
}

void
UsdMayaMeshUtil::assignSubDivTagsToUSDPrim(MFnMesh& meshFn,
                                           UsdGeomMesh& primSchema,
                                           UsdUtilsSparseValueWriter& valueWriter)
{
    // Vert Creasing
    MUintArray mayaCreaseVertIds;
    MDoubleArray mayaCreaseVertValues;
    meshFn.getCreaseVertices(mayaCreaseVertIds, mayaCreaseVertValues);
    if (!TF_VERIFY(mayaCreaseVertIds.length() == mayaCreaseVertValues.length())) {
        return;
    }
    if (mayaCreaseVertIds.length() > 0u) {
        VtIntArray subdCornerIndices(mayaCreaseVertIds.length());
        VtFloatArray subdCornerSharpnesses(mayaCreaseVertIds.length());
        for (unsigned int i = 0u; i < mayaCreaseVertIds.length(); ++i) {
            subdCornerIndices[i] = mayaCreaseVertIds[i];
            subdCornerSharpnesses[i] = mayaCreaseVertValues[i];
        }

        // not animatable
        UsdMayaWriteUtil::SetAttribute(primSchema.GetCornerIndicesAttr(), &subdCornerIndices, valueWriter);

        // not animatable
        UsdMayaWriteUtil::SetAttribute(primSchema.GetCornerSharpnessesAttr(), &subdCornerSharpnesses, valueWriter);
    }

    // Edge Creasing
    int edgeVerts[2];
    MUintArray mayaCreaseEdgeIds;
    MDoubleArray mayaCreaseEdgeValues;
    meshFn.getCreaseEdges(mayaCreaseEdgeIds, mayaCreaseEdgeValues);
    if (!TF_VERIFY(mayaCreaseEdgeIds.length() == mayaCreaseEdgeValues.length())) {
        return;
    }
    if (mayaCreaseEdgeIds.length() > 0u) {
        std::vector<int> subdCreaseIndices(mayaCreaseEdgeIds.length() * 2);
        std::vector<float> subdCreaseSharpnesses(mayaCreaseEdgeIds.length());
        for (unsigned int i = 0u; i < mayaCreaseEdgeIds.length(); ++i) {
            meshFn.getEdgeVertices(mayaCreaseEdgeIds[i], edgeVerts);
            subdCreaseIndices[i * 2] = edgeVerts[0];
            subdCreaseIndices[i * 2 + 1] = edgeVerts[1];
            subdCreaseSharpnesses[i] = mayaCreaseEdgeValues[i];
        }

        std::vector<int> numCreases;
        std::vector<int> creases;
        std::vector<float> creaseSharpnesses;
        compressCreases(subdCreaseIndices, subdCreaseSharpnesses,
                &numCreases, &creases, &creaseSharpnesses);

        if (!creases.empty()) {
            VtIntArray creaseIndicesVt(creases.size());
            std::copy(creases.begin(), creases.end(), creaseIndicesVt.begin());
            UsdMayaWriteUtil::SetAttribute(primSchema.GetCreaseIndicesAttr(), &creaseIndicesVt, valueWriter);
        }
        if (!numCreases.empty()) {
            VtIntArray creaseLengthsVt(numCreases.size());
            std::copy(numCreases.begin(), numCreases.end(), creaseLengthsVt.begin());
            UsdMayaWriteUtil::SetAttribute(primSchema.GetCreaseLengthsAttr(), &creaseLengthsVt, valueWriter);
        }
        if (!creaseSharpnesses.empty()) {
            VtFloatArray creaseSharpnessesVt(creaseSharpnesses.size());
            std::copy(
                creaseSharpnesses.begin(),
                creaseSharpnesses.end(),
                creaseSharpnessesVt.begin());
            UsdMayaWriteUtil::SetAttribute(primSchema.GetCreaseSharpnessesAttr(), &creaseSharpnessesVt, valueWriter);
        }
    }
}

void
UsdMayaMeshUtil::writeVertexData(const MFnMesh& meshFn,
                                 UsdGeomMesh& primSchema,
                                 const UsdTimeCode& usdTime,
                                 UsdUtilsSparseValueWriter& valueWriter)
{
    MStatus status{MS::kSuccess};

    const uint32_t numVertices = meshFn.numVertices();
    VtArray<GfVec3f> points(numVertices);
    const float* pointsData = meshFn.getRawPoints(&status);
    if(status)
    {
        // use memcpy() to copy the data. HS April 09, 2020
        memcpy((GfVec3f*)points.data(), pointsData, sizeof(float) * 3 * numVertices);

        VtArray<GfVec3f> extent(2);
        // Compute the extent using the raw points
        UsdGeomPointBased::ComputeExtent(points, &extent);

        UsdMayaWriteUtil::SetAttribute(primSchema.GetPointsAttr(), &points, valueWriter, usdTime);
        UsdMayaWriteUtil::SetAttribute(primSchema.CreateExtentAttr(), &extent, valueWriter,usdTime);
    }
    else
    {
        MGlobal::displayError(MString("Unable to access mesh vertices on mesh: ") + meshFn.fullPathName());
    }
}

void 
UsdMayaMeshUtil::writeFaceVertexIndicesData(const MFnMesh& meshFn, 
                                            UsdGeomMesh& primSchema, 
                                            const UsdTimeCode& usdTime, 
                                            UsdUtilsSparseValueWriter& valueWriter)
{
    const int numFaceVertices = meshFn.numFaceVertices();
    const int numPolygons = meshFn.numPolygons();

    VtArray<int> faceVertexCounts(numPolygons);
    VtArray<int> faceVertexIndices(numFaceVertices);
    MIntArray mayaFaceVertexIndices; // used in loop below
    unsigned int curFaceVertexIndex = 0;
    for (int i = 0; i < numPolygons; i++) {
        meshFn.getPolygonVertices(i, mayaFaceVertexIndices);
        faceVertexCounts[i] = mayaFaceVertexIndices.length();
        for (unsigned int j=0; j < mayaFaceVertexIndices.length(); j++) {
            faceVertexIndices[ curFaceVertexIndex ] = mayaFaceVertexIndices[j]; // push_back
            curFaceVertexIndex++;
        }
    }
    UsdMayaWriteUtil::SetAttribute(primSchema.GetFaceVertexCountsAttr(), &faceVertexCounts, valueWriter, usdTime);
    UsdMayaWriteUtil::SetAttribute(primSchema.GetFaceVertexIndicesAttr(), &faceVertexIndices, valueWriter, usdTime);
}

void 
UsdMayaMeshUtil::writeInvisibleFacesData(const MFnMesh& meshFn, 
                                         UsdGeomMesh& primSchema, 
                                         UsdUtilsSparseValueWriter& valueWriter)
{
    MUintArray mayaHoles = meshFn.getInvisibleFaces();
    const uint32_t count = mayaHoles.length();
    if (count)
    {
        VtArray<int32_t> subdHoles(count);
        uint32_t* ptr = &mayaHoles[0];
        // use memcpy() to copy the date HS November 20, 2019
        memcpy((int32_t*)subdHoles.data(), ptr, count * sizeof(uint32_t));
        // not animatable in Maya, so we'll set default only
        UsdMayaWriteUtil::SetAttribute(primSchema.GetHoleIndicesAttr(), &subdHoles, valueWriter);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
