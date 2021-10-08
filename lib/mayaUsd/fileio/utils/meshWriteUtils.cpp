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

#include <mayaUsd/base/debugCodes.h>
#include <mayaUsd/fileio/utils/adaptor.h>
#include <mayaUsd/fileio/utils/meshReadUtils.h>
#include <mayaUsd/fileio/utils/roundTripUtil.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/utils/colorSpace.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/pxr.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/pointBased.h>
#include <pxr/usd/usdGeom/primvar.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdUtils/pipeline.h>

#include <maya/MBoundingBox.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItMeshFaceVertex.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MPoint.h>
#include <maya/MStatus.h>
#include <maya/MUintArray.h>
#include <maya/MVector.h>

static constexpr char kMayaAttrNameInMesh[] = "inMesh";

PXR_NAMESPACE_OPEN_SCOPE

// These tokens are supported Maya attributes used for Mesh surfaces
// clang-format off
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
// clang-format on

namespace {
/// Default value to use when collecting UVs from a UV set and a component
/// has no authored value.
const GfVec2f UnauthoredUV = GfVec2f(0.f);

/// Default values to use when collecting colors based on shader values
/// and an object or component has no assigned shader.
const GfVec3f UnauthoredShaderRGB = GfVec3f(0.5f);
const float   UnauthoredShaderAlpha = 0.0f;

/// Default values to use when collecting colors from a color set and a
/// component has no authored value.
const GfVec3f UnauthoredColorSetRGB = GfVec3f(1.0f);
const float   UnauthoredColorAlpha = 1.0f;
const GfVec4f UnauthoredColorSetRGBA = GfVec4f(
    UnauthoredColorSetRGB[0],
    UnauthoredColorSetRGB[1],
    UnauthoredColorSetRGB[2],
    UnauthoredColorAlpha);

// XXX: Note that this function is not exposed publicly since the USD schema
// has been updated to conform to OpenSubdiv 3. We still look for this attribute
// on Maya nodes specifying this value from OpenSubdiv 2, but we translate the
// value to OpenSubdiv 3. This is to support legacy assets authored against
// OpenSubdiv 2.
static TfToken getOsd2FVInterpBoundary(const MFnMesh& mesh)
{
    TfToken sdFVInterpBound;

    MPlug plug = mesh.findPlug(MString(_meshTokens->USD_faceVaryingInterpolateBoundary.GetText()));
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
            switch (plug.asInt()) {
            case 0: sdFVInterpBound = UsdGeomTokens->all; break;
            case 1: sdFVInterpBound = UsdGeomTokens->cornersPlus1; break;
            case 2: sdFVInterpBound = UsdGeomTokens->none; break;
            case 3: sdFVInterpBound = UsdGeomTokens->boundaries; break;
            default: break;
            }
        }
    }

    return sdFVInterpBound;
}

void compressCreases(
    const std::vector<int>&   inCreaseIndices,
    const std::vector<float>& inCreaseSharpnesses,
    std::vector<int>*         creaseLengths,
    std::vector<int>*         creaseIndices,
    std::vector<float>*       creaseSharpnesses)
{
    // Process vertex pairs.
    for (size_t i = 0; i < inCreaseSharpnesses.size(); i++) {
        const float sharpness = inCreaseSharpnesses[i];
        const int   v0 = inCreaseIndices[i * 2 + 0];
        const int   v1 = inCreaseIndices[i * 2 + 1];
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

/// Sets the primvar \p primvar at time \p usdTime using the given
/// \p indices (can be empty) and \p values.
/// The \p defaultValue is used to pad the \p values array in case
/// \p indices contains unassigned indices (i.e. indices < 0) that need a
/// corresponding value in the array.
///
/// When authoring values at a non-default time, setPrimvar() might
/// unnecessarily pad \p values with \p defaultValue in order to guarantee
/// that the primvar remains valid during the export process. In that case,
/// the expected value of UsdGeomPrimvar::ComputeFlattened() is still
/// correct (there is just some memory wasted).
/// In order to cleanup any extra values and reclaim the wasted memory, call
/// cleanupPrimvars() at the end of the export process.
void setPrimvar(
    const UsdGeomPrimvar&      primvar,
    const VtIntArray&          indices,
    const VtValue&             values,
    const VtValue&             defaultValue,
    const UsdTimeCode&         usdTime,
    UsdUtilsSparseValueWriter* valueWriter)
{
    // Simple case of non-indexed primvars.
    if (indices.empty()) {
        UsdMayaWriteUtil::SetAttribute(primvar.GetAttr(), values, usdTime, valueWriter);
        return;
    }

    // The mesh writer writes primvars only at default time or at time samples,
    // but never both. We depend on that fact here to do different things
    // depending on whether you ever export the default-time data or not.
    if (usdTime.IsDefault()) {
        // If we are only exporting the default values, then we know
        // definitively whether we need to pad the values array with the
        // unassigned value or not.
        if (UsdMayaUtil::containsUnauthoredValues(indices)) {
            primvar.SetUnauthoredValuesIndex(0);

            const VtValue paddedValues = UsdMayaUtil::pushFirstValue(values, defaultValue);
            if (!paddedValues.IsEmpty()) {
                UsdMayaWriteUtil::SetAttribute(
                    primvar.GetAttr(), paddedValues, usdTime, valueWriter);
                UsdMayaWriteUtil::SetAttribute(
                    primvar.CreateIndicesAttr(),
                    UsdMayaUtil::shiftIndices(indices, 1),
                    usdTime,
                    valueWriter);
            } else {
                TF_CODING_ERROR(
                    "Unable to pad values array for <%s>", primvar.GetAttr().GetPath().GetText());
            }
        } else {
            UsdMayaWriteUtil::SetAttribute(primvar.GetAttr(), values, usdTime, valueWriter);
            UsdMayaWriteUtil::SetAttribute(
                primvar.CreateIndicesAttr(), indices, usdTime, valueWriter);
        }
    } else {
        // If we are exporting animation, then we don't know definitively
        // whether we need to set the unauthoredValuesIndex.
        // In order to keep the primvar valid throughout the entire export
        // process, _always_ pad the values array with the unassigned value,
        // then go back and clean it up during the post-export.
        if (primvar.GetUnauthoredValuesIndex() != 0
            && UsdMayaUtil::containsUnauthoredValues(indices)) {
            primvar.SetUnauthoredValuesIndex(0);
        }

        const VtValue paddedValues = UsdMayaUtil::pushFirstValue(values, defaultValue);
        if (!paddedValues.IsEmpty()) {
            UsdMayaWriteUtil::SetAttribute(primvar.GetAttr(), paddedValues, usdTime, valueWriter);
            UsdMayaWriteUtil::SetAttribute(
                primvar.CreateIndicesAttr(),
                UsdMayaUtil::shiftIndices(indices, 1),
                usdTime,
                valueWriter);
        } else {
            TF_CODING_ERROR(
                "Unable to pad values array for <%s>", primvar.GetAttr().GetPath().GetText());
        }
    }
}

UsdGeomPrimvar createUVPrimVar(
    UsdGeomGprim&              primSchema,
    const TfToken&             name,
    const UsdTimeCode&         usdTime,
    const VtArray<GfVec2f>&    data,
    const TfToken&             interpolation,
    const VtIntArray&          assignmentIndices,
    UsdUtilsSparseValueWriter* valueWriter)
{
    const unsigned int numValues = data.size();
    if (numValues == 0) {
        return UsdGeomPrimvar();
    }

    TfToken interp = interpolation;
    if (numValues == 1 && interp == UsdGeomTokens->constant) {
        interp = TfToken();
    }

    SdfValueTypeName uvValueType = (UsdMayaWriteUtil::WriteUVAsFloat2())
        ? (SdfValueTypeNames->Float2Array)
        : (SdfValueTypeNames->TexCoord2fArray);

    UsdGeomPrimvar primVar = primSchema.CreatePrimvar(name, uvValueType, interp);

    setPrimvar(
        primVar, assignmentIndices, VtValue(data), VtValue(UnauthoredUV), usdTime, valueWriter);

    return primVar;
}

// This function condenses distinct indices that point to the same color values
// (the combination of RGB AND Alpha) to all point to the same index for that
// value. This will potentially shrink the data arrays.
void MergeEquivalentColorSetValues(
    VtVec3fArray* colorSetRGBData,
    VtFloatArray* colorSetAlphaData,
    VtIntArray*   colorSetAssignmentIndices)
{
    if (!colorSetRGBData || !colorSetAlphaData || !colorSetAssignmentIndices) {
        return;
    }

    const size_t numValues = colorSetRGBData->size();
    if (numValues == 0) {
        return;
    }

    if (colorSetAlphaData->size() != numValues) {
        TF_CODING_ERROR(
            "Unequal sizes for color (%zu) and alpha (%zu)",
            colorSetRGBData->size(),
            colorSetAlphaData->size());
    }

    // We first combine the separate color and alpha arrays into one GfVec4f
    // array.
    VtArray<GfVec4f> colorsWithAlphasData(numValues);
    for (size_t i = 0; i < numValues; ++i) {
        const GfVec3f color = (*colorSetRGBData)[i];
        const float   alpha = (*colorSetAlphaData)[i];

        colorsWithAlphasData[i][0] = color[0];
        colorsWithAlphasData[i][1] = color[1];
        colorsWithAlphasData[i][2] = color[2];
        colorsWithAlphasData[i][3] = alpha;
    }

    VtIntArray mergedIndices(*colorSetAssignmentIndices);
    UsdMayaUtil::MergeEquivalentIndexedValues(&colorsWithAlphasData, &mergedIndices);

    // If we reduced the number of values by merging, copy the results back,
    // separating the values back out into colors and alphas.
    const size_t newSize = colorsWithAlphasData.size();
    if (newSize < numValues) {
        colorSetRGBData->resize(newSize);
        colorSetAlphaData->resize(newSize);

        for (size_t i = 0; i < newSize; ++i) {
            const GfVec4f colorWithAlpha = colorsWithAlphasData[i];

            (*colorSetRGBData)[i][0] = colorWithAlpha[0];
            (*colorSetRGBData)[i][1] = colorWithAlpha[1];
            (*colorSetRGBData)[i][2] = colorWithAlpha[2];
            (*colorSetAlphaData)[i] = colorWithAlpha[3];
        }
        (*colorSetAssignmentIndices) = mergedIndices;
    }
}

GfVec3f LinearColorFromColorSet(const MColor& mayaColor, bool shouldConvertToLinear)
{
    // we assume all color sets except displayColor are in linear space.
    // if we got a color from colorSetData and we're a displayColor, we
    // need to convert it to linear.
    GfVec3f c(mayaColor[0], mayaColor[1], mayaColor[2]);
    if (shouldConvertToLinear) {
        return MayaUsd::utils::ConvertMayaToLinear(c);
    }
    return c;
}

} // anonymous namespace

MStatus
UsdMayaMeshWriteUtils::getSkinClusterConnectedToMesh(const MObject& mesh, MObject& skinCluster)
{
    // TODO: (yliangsiew) Do we care about multiple skinCluster layers? How do we even want
    //        to deal with that, if at all?
    MStatus stat;
    if (!mesh.hasFn(MFn::kMesh)) {
        return MStatus::kInvalidParameter;
    }

    MFnDependencyNode fnNode(mesh, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);

    MPlug inMeshPlug = fnNode.findPlug(kMayaAttrNameInMesh, false, &stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);

    bool isDest = inMeshPlug.isDestination(&stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    if (!isDest) {
        return MStatus::kFailure;
    }
    MPlug srcPlug = inMeshPlug.source(&stat);
    CHECK_MSTATUS_AND_RETURN_IT(stat);
    if (srcPlug.isNull()) {
        return MStatus::kFailure;
    }

    skinCluster = srcPlug.node(&stat);

    CHECK_MSTATUS_AND_RETURN_IT(stat);
    if (!skinCluster.hasFn(MFn::kSkinClusterFilter)) {
        return MStatus::kFailure;
    }

    return stat;
}

MStatus UsdMayaMeshWriteUtils::getSkinClustersUpstreamOfMesh(
    const MObject& mesh,
    MObjectArray&  skinClusters)
{
    MStatus stat;
    if (mesh.isNull() || !mesh.hasFn(MFn::kMesh)) {
        return MStatus::kInvalidParameter;
    }

    skinClusters.clear();
    MObject            searchObj = MObject(mesh);
    MItDependencyGraph itDg(
        searchObj,
        MFn::kInvalid,
        MItDependencyGraph::kUpstream,
        MItDependencyGraph::kDepthFirst,
        MItDependencyGraph::kNodeLevel,
        &stat);
    while (!itDg.isDone()) {
        MObject curNode = itDg.currentItem();
        if (curNode.hasFn(MFn::kSkinClusterFilter)) {
            skinClusters.append(curNode);
        }
        itDg.next();
    }

    return stat;
}

MBoundingBox UsdMayaMeshWriteUtils::calcBBoxOfMeshes(const MObjectArray& meshes)
{
    unsigned int numMeshes = meshes.length();
    MFnMesh      fnMesh;
    MStatus      stat;
    MVector      a;
    MVector      b;
    for (unsigned int i = 0; i < numMeshes; ++i) {
        MObject curMesh = meshes[i];
        TF_VERIFY(curMesh.hasFn(MFn::kMesh));
        fnMesh.setObject(curMesh);
        unsigned int numVertices = fnMesh.numVertices();
        const float* meshPts = fnMesh.getRawPoints(&stat);
        for (unsigned int j = 0; j < numVertices; ++j) {
            float x = meshPts[j * 3];
            float y = meshPts[(j * 3) + 1];
            float z = meshPts[(j * 3) + 2];

            a.x = x < a.x ? x : a.x;
            b.x = x > b.x ? x : b.x;

            a.y = y < a.y ? y : a.y;
            b.y = y > b.y ? y : b.y;

            a.z = z < a.z ? z : a.z;
            b.z = z > b.z ? z : b.z;
        }
    }

    MBoundingBox result = MBoundingBox(MPoint(a), MPoint(b));
    return result;
}

bool UsdMayaMeshWriteUtils::getMeshNormals(
    const MFnMesh& mesh,
    VtVec3fArray*  normalsArray,
    TfToken*       interpolation)
{
    MStatus status { MS::kSuccess };

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

    // get normal indices for all vertices of faces
    MIntArray normalCounts, normalIndices;
    mesh.getNormalIds(normalCounts, normalIndices);

    for (size_t i = 0; i < normalIndices.length(); ++i) {
        MFloatVector normal = mayaNormals[normalIndices[i]];
        (*normalsArray)[i][0] = normal[0];
        (*normalsArray)[i][1] = normal[1];
        (*normalsArray)[i][2] = normal[2];
    }

    return true;
}

// This can be customized for specific pipelines.
// We first look for the USD string attribute, and if not present we look for
// the RenderMan for Maya int attribute.
// XXX Maybe we should come up with a OSD centric nomenclature ??
TfToken UsdMayaMeshWriteUtils::getSubdivScheme(const MFnMesh& mesh)
{
    // Try grabbing the value via the adaptor first.
    TfToken                 schemeToken;
    UsdMayaSchemaAdaptorPtr meshSchema
        = UsdMayaAdaptor(mesh.object()).GetSchemaOrInheritedSchema<UsdGeomMesh>();
    if (meshSchema) {
        meshSchema->GetAttribute(UsdGeomTokens->subdivisionScheme).Get<TfToken>(&schemeToken);
    }

    // Fall back to the RenderMan for Maya attribute.
    if (schemeToken.IsEmpty()) {
        MPlug plug = mesh.findPlug(MString("rman__torattr___subdivScheme"));
        if (!plug.isNull()) {
            switch (plug.asInt()) {
            case 0: schemeToken = UsdGeomTokens->catmullClark; break;
            case 1: schemeToken = UsdGeomTokens->loop; break;
            default: break;
            }
        }
    }

    if (schemeToken.IsEmpty()) {
        return TfToken();
    } else if (
        schemeToken != UsdGeomTokens->none && schemeToken != UsdGeomTokens->catmullClark
        && schemeToken != UsdGeomTokens->loop && schemeToken != UsdGeomTokens->bilinear) {
        TF_RUNTIME_ERROR(
            "Unsupported subdivision scheme: %s on mesh: %s",
            schemeToken.GetText(),
            mesh.fullPathName().asChar());
        return TfToken();
    }

    return schemeToken;
}

// This can be customized for specific pipelines.
// We first look for the USD string attribute, and if not present we look for
// the RenderMan for Maya int attribute.
// XXX Maybe we should come up with a OSD centric nomenclature ??
TfToken UsdMayaMeshWriteUtils::getSubdivInterpBoundary(const MFnMesh& mesh)
{
    // Try grabbing the value via the adaptor first.
    TfToken                 interpBoundaryToken;
    UsdMayaSchemaAdaptorPtr meshSchema
        = UsdMayaAdaptor(mesh.object()).GetSchemaOrInheritedSchema<UsdGeomMesh>();
    if (meshSchema) {
        meshSchema->GetAttribute(UsdGeomTokens->interpolateBoundary)
            .Get<TfToken>(&interpBoundaryToken);
    }

    // Fall back to the RenderMan for Maya attr.
    if (interpBoundaryToken.IsEmpty()) {
        MPlug plug = mesh.findPlug(MString("rman__torattr___subdivInterp"));
        if (!plug.isNull()) {
            switch (plug.asInt()) {
            case 0: interpBoundaryToken = UsdGeomTokens->none; break;
            case 1: interpBoundaryToken = UsdGeomTokens->edgeAndCorner; break;
            case 2: interpBoundaryToken = UsdGeomTokens->edgeOnly; break;
            default: break;
            }
        }
    }

    if (interpBoundaryToken.IsEmpty()) {
        return TfToken();
    } else if (
        interpBoundaryToken != UsdGeomTokens->none
        && interpBoundaryToken != UsdGeomTokens->edgeAndCorner
        && interpBoundaryToken != UsdGeomTokens->edgeOnly) {
        TF_RUNTIME_ERROR(
            "Unsupported interpolate boundary setting: %s on mesh: %s",
            interpBoundaryToken.GetText(),
            mesh.fullPathName().asChar());
        return TfToken();
    }

    return interpBoundaryToken;
}

TfToken UsdMayaMeshWriteUtils::getSubdivFVLinearInterpolation(const MFnMesh& mesh)
{
    // Try grabbing the value via the adaptor first.
    TfToken                 sdFVLinearInterpolation;
    UsdMayaSchemaAdaptorPtr meshSchema
        = UsdMayaAdaptor(mesh.object()).GetSchemaOrInheritedSchema<UsdGeomMesh>();
    if (meshSchema) {
        meshSchema->GetAttribute(UsdGeomTokens->faceVaryingLinearInterpolation)
            .Get<TfToken>(&sdFVLinearInterpolation);
    }

    // If the OpenSubdiv 3-style face varying linear interpolation value
    // wasn't specified, fall back to the old OpenSubdiv 2-style face
    // varying interpolate boundary value if we have that.
    if (sdFVLinearInterpolation.IsEmpty()) {
        sdFVLinearInterpolation = getOsd2FVInterpBoundary(mesh);
    }

    if (!sdFVLinearInterpolation.IsEmpty() && sdFVLinearInterpolation != UsdGeomTokens->all
        && sdFVLinearInterpolation != UsdGeomTokens->none
        && sdFVLinearInterpolation != UsdGeomTokens->boundaries
        && sdFVLinearInterpolation != UsdGeomTokens->cornersOnly
        && sdFVLinearInterpolation != UsdGeomTokens->cornersPlus1
        && sdFVLinearInterpolation != UsdGeomTokens->cornersPlus2) {
        TF_RUNTIME_ERROR(
            "Unsupported face-varying linear interpolation: %s "
            "on mesh: %s",
            sdFVLinearInterpolation.GetText(),
            mesh.fullPathName().asChar());
        return TfToken();
    }

    return sdFVLinearInterpolation;
}

bool UsdMayaMeshWriteUtils::isMeshValid(const MDagPath& dagPath)
{
    MStatus status { MS::kSuccess };

    // sanity checks
    MFnMesh lMesh(dagPath, &status);

    if (!status) {
        TF_RUNTIME_ERROR(
            "MFnMesh() failed for mesh at DAG path: %s", dagPath.fullPathName().asChar());
        return false;
    }

    const auto numVertices = lMesh.numVertices();
    const auto numPolygons = lMesh.numPolygons();

    if (numVertices < 3 && numVertices > 0) {
        TF_RUNTIME_ERROR(
            "%s is not a valid mesh, because it only has %u points,",
            lMesh.fullPathName().asChar(),
            numVertices);
    }
    if (numPolygons == 0) {
        TF_WARN("%s has no polygons.", lMesh.fullPathName().asChar());
    }
    return true;
}

void UsdMayaMeshWriteUtils::exportReferenceMesh(UsdGeomMesh& primSchema, MObject obj)
{
    MStatus status { MS::kSuccess };

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

    const float*   mayaRawPoints = referenceMesh.getRawPoints(&status);
    const GfVec3f* mayaRawVec3 = reinterpret_cast<const GfVec3f*>(mayaRawPoints);
    const int      numVertices = referenceMesh.numVertices();
    VtVec3fArray   points(mayaRawVec3, mayaRawVec3 + numVertices);

    UsdGeomPrimvar primVar = primSchema.CreatePrimvar(
        UsdUtilsGetPrefName(), SdfValueTypeNames->Point3fArray, UsdGeomTokens->varying);

    if (!primVar) {
        return;
    }

    primVar.GetAttr().Set(VtValue(points));
}

void UsdMayaMeshWriteUtils::assignSubDivTagsToUSDPrim(
    MFnMesh&                   meshFn,
    UsdGeomMesh&               primSchema,
    UsdUtilsSparseValueWriter* valueWriter)
{
    // Vert Creasing
    MUintArray   mayaCreaseVertIds;
    MDoubleArray mayaCreaseVertValues;
    meshFn.getCreaseVertices(mayaCreaseVertIds, mayaCreaseVertValues);
    if (!TF_VERIFY(mayaCreaseVertIds.length() == mayaCreaseVertValues.length())) {
        return;
    }
    if (mayaCreaseVertIds.length() > 0u) {
        VtIntArray   subdCornerIndices(mayaCreaseVertIds.length());
        VtFloatArray subdCornerSharpnesses(mayaCreaseVertIds.length());
        for (unsigned int i = 0u; i < mayaCreaseVertIds.length(); ++i) {
            subdCornerIndices[i] = mayaCreaseVertIds[i];
            subdCornerSharpnesses[i] = mayaCreaseVertValues[i];
        }

        // not animatable
        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetCornerIndicesAttr(),
            &subdCornerIndices,
            UsdTimeCode::Default(),
            valueWriter);

        // not animatable
        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetCornerSharpnessesAttr(),
            &subdCornerSharpnesses,
            UsdTimeCode::Default(),
            valueWriter);
    }

    // Edge Creasing
    int          edgeVerts[2];
    MUintArray   mayaCreaseEdgeIds;
    MDoubleArray mayaCreaseEdgeValues;
    meshFn.getCreaseEdges(mayaCreaseEdgeIds, mayaCreaseEdgeValues);
    if (!TF_VERIFY(mayaCreaseEdgeIds.length() == mayaCreaseEdgeValues.length())) {
        return;
    }
    if (mayaCreaseEdgeIds.length() > 0u) {
        std::vector<int> subdCreaseIndices(mayaCreaseEdgeIds.length() * 2);
        // just construct directly from the array data
        // by moving this out of the loop, you'll leverage SIMD ops here.
        std::vector<float> subdCreaseSharpnesses(
            &mayaCreaseEdgeValues[0], &mayaCreaseEdgeValues[0] + mayaCreaseEdgeValues.length());
        // avoid dso call by taking a copy of length.
        for (unsigned int i = 0u, n = mayaCreaseEdgeIds.length(); i < n; ++i) {
            meshFn.getEdgeVertices(mayaCreaseEdgeIds[i], edgeVerts);
            subdCreaseIndices[i * 2] = edgeVerts[0];
            subdCreaseIndices[i * 2 + 1] = edgeVerts[1];
        }

        std::vector<int>   numCreases;
        std::vector<int>   creases;
        std::vector<float> creaseSharpnesses;
        compressCreases(
            subdCreaseIndices, subdCreaseSharpnesses, &numCreases, &creases, &creaseSharpnesses);

        if (!creases.empty()) {
            VtIntArray creaseIndicesVt(creases.size());
            std::copy(creases.begin(), creases.end(), creaseIndicesVt.begin());
            UsdMayaWriteUtil::SetAttribute(
                primSchema.GetCreaseIndicesAttr(),
                &creaseIndicesVt,
                UsdTimeCode::Default(),
                valueWriter);
        }
        if (!numCreases.empty()) {
            VtIntArray creaseLengthsVt(numCreases.size());
            std::copy(numCreases.begin(), numCreases.end(), creaseLengthsVt.begin());
            UsdMayaWriteUtil::SetAttribute(
                primSchema.GetCreaseLengthsAttr(),
                &creaseLengthsVt,
                UsdTimeCode::Default(),
                valueWriter);
        }
        if (!creaseSharpnesses.empty()) {
            VtFloatArray creaseSharpnessesVt(creaseSharpnesses.size());
            std::copy(
                creaseSharpnesses.begin(), creaseSharpnesses.end(), creaseSharpnessesVt.begin());
            UsdMayaWriteUtil::SetAttribute(
                primSchema.GetCreaseSharpnessesAttr(),
                &creaseSharpnessesVt,
                UsdTimeCode::Default(),
                valueWriter);
        }
    }
}

void UsdMayaMeshWriteUtils::writePointsData(
    const MFnMesh&             meshFn,
    UsdGeomMesh&               primSchema,
    const UsdTimeCode&         usdTime,
    UsdUtilsSparseValueWriter* valueWriter)
{
    MStatus status { MS::kSuccess };

    const uint32_t numVertices = meshFn.numVertices();
    const float*   pointsData = meshFn.getRawPoints(&status);
    if (!status) {
        MGlobal::displayError(
            MString("Unable to access mesh vertices on mesh: ") + meshFn.fullPathName());
        return;
    }

    const GfVec3f* vecData = reinterpret_cast<const GfVec3f*>(pointsData);
    VtVec3fArray   points(vecData, vecData + numVertices);
    VtVec3fArray   extent(2);
    // Compute the extent using the raw points
    UsdGeomPointBased::ComputeExtent(points, &extent);

    UsdMayaWriteUtil::SetAttribute(primSchema.GetPointsAttr(), &points, usdTime, valueWriter);
    UsdMayaWriteUtil::SetAttribute(primSchema.CreateExtentAttr(), &extent, usdTime, valueWriter);
}

void UsdMayaMeshWriteUtils::writeFaceVertexIndicesData(
    const MFnMesh&             meshFn,
    UsdGeomMesh&               primSchema,
    const UsdTimeCode&         usdTime,
    UsdUtilsSparseValueWriter* valueWriter)
{
    const int numFaceVertices = meshFn.numFaceVertices();
    const int numPolygons = meshFn.numPolygons();

    VtIntArray   faceVertexCounts(numPolygons);
    VtIntArray   faceVertexIndices(numFaceVertices);
    MIntArray    mayaFaceVertexIndices; // used in loop below
    unsigned int curFaceVertexIndex = 0;
    for (int i = 0; i < numPolygons; i++) {
        meshFn.getPolygonVertices(i, mayaFaceVertexIndices);
        faceVertexCounts[i] = mayaFaceVertexIndices.length();
        for (unsigned int j = 0; j < mayaFaceVertexIndices.length(); j++) {
            faceVertexIndices[curFaceVertexIndex] = mayaFaceVertexIndices[j]; // push_back
            curFaceVertexIndex++;
        }
    }
    UsdMayaWriteUtil::SetAttribute(
        primSchema.GetFaceVertexCountsAttr(), &faceVertexCounts, usdTime, valueWriter);
    UsdMayaWriteUtil::SetAttribute(
        primSchema.GetFaceVertexIndicesAttr(), &faceVertexIndices, usdTime, valueWriter);
}

void UsdMayaMeshWriteUtils::writeInvisibleFacesData(
    const MFnMesh&             meshFn,
    UsdGeomMesh&               primSchema,
    UsdUtilsSparseValueWriter* valueWriter)
{
    MUintArray     mayaHoles = meshFn.getInvisibleFaces();
    const uint32_t count = mayaHoles.length();
    if (count) {
        VtIntArray subdHoles(count);
        uint32_t*  ptr = &mayaHoles[0];
        // use memcpy() to copy the data. HS April 20, 2019
        memcpy((int32_t*)subdHoles.data(), ptr, count * sizeof(uint32_t));
        // not animatable in Maya, so we'll set default only
        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetHoleIndicesAttr(), &subdHoles, UsdTimeCode::Default(), valueWriter);
    }
}

bool UsdMayaMeshWriteUtils::getMeshUVSetData(
    const MFnMesh& mesh,
    const MString& uvSetName,
    VtVec2fArray*  uvArray,
    TfToken*       interpolation,
    VtIntArray*    assignmentIndices)
{
    // Check first to make sure this UV set even has assigned values before we
    // attempt to do anything with the data. We cannot directly use this data
    // otherwise though since we need a uvId for every face vertex, and the
    // returned uvIds MIntArray may be shorter than that if there are unmapped
    // faces.
    MIntArray uvCounts, uvIds;
    MStatus   status = mesh.getAssignedUVs(uvCounts, uvIds, &uvSetName);
    CHECK_MSTATUS_AND_RETURN(status, false);

    if (uvCounts.length() == 0u || uvIds.length() == 0u) {
        return false;
    }

    // Transfer the UV values directly to USD, in the same order as they are in
    // the Maya mesh.
    MFloatArray uArray;
    MFloatArray vArray;
    status = mesh.getUVs(uArray, vArray, &uvSetName);
    CHECK_MSTATUS_AND_RETURN(status, false);

    if (uArray.length() != vArray.length()) {
        return false;
    }

    uvArray->clear();
    uvArray->reserve(static_cast<size_t>(uArray.length()));
    for (unsigned int uvId = 0u; uvId < uArray.length(); ++uvId) {
#if PXR_VERSION >= 2011
        uvArray->emplace_back(uArray[uvId], vArray[uvId]);
#else
        GfVec2f value(uArray[uvId], vArray[uvId]);
        uvArray->push_back(value);
#endif
    }

    // Now iterate through all the face vertices and fill in the faceVarying
    // assignmentIndices array, again in the same order as in the Maya mesh.
    const unsigned int numFaceVertices = mesh.numFaceVertices(&status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    assignmentIndices->assign(static_cast<size_t>(numFaceVertices), -1);
    *interpolation = UsdGeomTokens->faceVarying;

    MItMeshFaceVertex itFV(mesh.object());
    unsigned int      fvi = 0u;
    for (itFV.reset(); !itFV.isDone(); itFV.next(), ++fvi) {
        if (!itFV.hasUVs(uvSetName)) {
            // No UVs for this faceVertex, so leave it unassigned.
            continue;
        }

        int uvIndex;
        itFV.getUVIndex(uvIndex, &uvSetName);
        if (uvIndex < 0 || static_cast<unsigned int>(uvIndex) >= uArray.length()) {
            return false;
        }

        (*assignmentIndices)[fvi] = uvIndex;
    }

    // We do not merge indexed values or compress indices here in an effort to
    // maintain the same UV shells and connectivity across export/import
    // round-trips.

    return true;
}

bool UsdMayaMeshWriteUtils::writeUVSetsAsVec2fPrimvars(
    const MFnMesh&             meshFn,
    UsdGeomMesh&               primSchema,
    const UsdTimeCode&         usdTime,
    UsdUtilsSparseValueWriter* valueWriter)
{
    MStatus status { MS::kSuccess };

    MStringArray uvSetNames;

    status = meshFn.getUVSetNames(uvSetNames);

    if (!status) {
        return false;
    }

    for (unsigned int i = 0; i < uvSetNames.length(); ++i) {
        VtVec2fArray uvValues;
        TfToken      interpolation;
        VtIntArray   assignmentIndices;

        if (!UsdMayaMeshWriteUtils::getMeshUVSetData(
                meshFn, uvSetNames[i], &uvValues, &interpolation, &assignmentIndices)) {
            continue;
        }

        // All UV sets now get renamed st, st1, st2 in the order returned by getUVSetNames
        MString setName("st");
        if (i) {
            setName += i;
        }

        // create UV PrimVar
        UsdGeomPrimvar primVar = createUVPrimVar(
            primSchema,
            TfToken(setName.asChar()),
            usdTime,
            uvValues,
            interpolation,
            assignmentIndices,
            valueWriter);

        // Save the original name for roundtripping:
        if (primVar) {
            UsdMayaRoundTripUtil::SetPrimVarMayaName(
                primVar.GetAttr(), TfToken(uvSetNames[i].asChar()));
        }
    }

    return true;
}

void UsdMayaMeshWriteUtils::writeSubdivInterpBound(
    MFnMesh&                   meshFn,
    UsdGeomMesh&               primSchema,
    UsdUtilsSparseValueWriter* valueWriter)
{
    TfToken sdInterpBound = UsdMayaMeshWriteUtils::getSubdivInterpBoundary(meshFn);
    if (!sdInterpBound.IsEmpty()) {
        UsdMayaWriteUtil::SetAttribute(
            primSchema.CreateInterpolateBoundaryAttr(),
            sdInterpBound,
            UsdTimeCode::Default(),
            valueWriter);
    }
}

void UsdMayaMeshWriteUtils::writeSubdivFVLinearInterpolation(
    MFnMesh&                   meshFn,
    UsdGeomMesh&               primSchema,
    UsdUtilsSparseValueWriter* valueWriter)
{
    TfToken sdFVLinearInterpolation = UsdMayaMeshWriteUtils::getSubdivFVLinearInterpolation(meshFn);
    if (!sdFVLinearInterpolation.IsEmpty()) {
        UsdMayaWriteUtil::SetAttribute(
            primSchema.CreateFaceVaryingLinearInterpolationAttr(),
            sdFVLinearInterpolation,
            UsdTimeCode::Default(),
            valueWriter);
    }
}

void UsdMayaMeshWriteUtils::writeNormalsData(
    const MFnMesh&             meshFn,
    UsdGeomMesh&               primSchema,
    const UsdTimeCode&         usdTime,
    UsdUtilsSparseValueWriter* valueWriter)
{
    VtVec3fArray meshNormals;
    TfToken      normalInterp;

    if (UsdMayaMeshWriteUtils::getMeshNormals(meshFn, &meshNormals, &normalInterp)) {

        UsdMayaWriteUtil::SetAttribute(
            primSchema.GetNormalsAttr(), &meshNormals, usdTime, valueWriter);

        primSchema.SetNormalsInterpolation(normalInterp);
    }
}

bool UsdMayaMeshWriteUtils::addDisplayPrimvars(
    UsdGeomGprim&                       primSchema,
    const UsdTimeCode&                  usdTime,
    const MFnMesh::MColorRepresentation colorRep,
    const VtVec3fArray&                 RGBData,
    const VtFloatArray&                 AlphaData,
    const TfToken&                      interpolation,
    const VtIntArray&                   assignmentIndices,
    const bool                          clamped,
    const bool                          authored,
    UsdUtilsSparseValueWriter*          valueWriter)
{
    // We are appending the default value to the primvar in the post export function
    // so if the dataset is empty and the assignment indices are not, we still
    // have to set an empty array.
    // If we already have an authored value, don't try to write a new one.
    UsdAttribute colorAttr = primSchema.GetDisplayColorAttr();
    if (!colorAttr.HasAuthoredValue() && (!RGBData.empty() || !assignmentIndices.empty())) {
        UsdGeomPrimvar displayColor = primSchema.CreateDisplayColorPrimvar();
        if (interpolation != displayColor.GetInterpolation()) {
            displayColor.SetInterpolation(interpolation);
        }

        setPrimvar(
            displayColor,
            assignmentIndices,
            VtValue(RGBData),
            VtValue(UnauthoredShaderRGB),
            usdTime,
            valueWriter);

        bool authRGB = authored;
        if (colorRep == MFnMesh::kAlpha) {
            authRGB = false;
        }
        if (authRGB) {
            if (clamped) {
                UsdMayaRoundTripUtil::MarkPrimvarAsClamped(displayColor);
            }
        } else {
            UsdMayaRoundTripUtil::MarkAttributeAsMayaGenerated(colorAttr);
        }
    }

    UsdAttribute alphaAttr = primSchema.GetDisplayOpacityAttr();
    if (!alphaAttr.HasAuthoredValue() && (!AlphaData.empty() || !assignmentIndices.empty())) {
        // we consider a single alpha value that is 1.0 to be the "default"
        // value.  We only want to write values that are not the "default".
        bool hasDefaultAlpha = AlphaData.size() == 1 && GfIsClose(AlphaData[0], 1.0, 1e-9);
        if (!hasDefaultAlpha) {
            UsdGeomPrimvar displayOpacity = primSchema.CreateDisplayOpacityPrimvar();
            if (interpolation != displayOpacity.GetInterpolation()) {
                displayOpacity.SetInterpolation(interpolation);
            }

            setPrimvar(
                displayOpacity,
                assignmentIndices,
                VtValue(AlphaData),
                VtValue(UnauthoredShaderAlpha),
                usdTime,
                valueWriter);

            bool authAlpha = authored;
            if (colorRep == MFnMesh::kRGB) {
                authAlpha = false;
            }
            if (authAlpha) {
                if (clamped) {
                    UsdMayaRoundTripUtil::MarkPrimvarAsClamped(displayOpacity);
                }
            } else {
                UsdMayaRoundTripUtil::MarkAttributeAsMayaGenerated(alphaAttr);
            }
        }
    }

    return true;
}

bool UsdMayaMeshWriteUtils::createRGBPrimVar(
    UsdGeomGprim&              primSchema,
    const TfToken&             name,
    const UsdTimeCode&         usdTime,
    const VtVec3fArray&        data,
    const TfToken&             interpolation,
    const VtIntArray&          assignmentIndices,
    bool                       clamped,
    UsdUtilsSparseValueWriter* valueWriter)
{
    const unsigned int numValues = data.size();
    if (numValues == 0) {
        return false;
    }

    TfToken interp = interpolation;
    if (numValues == 1 && interp == UsdGeomTokens->constant) {
        interp = TfToken();
    }

    UsdGeomPrimvar primVar
        = primSchema.CreatePrimvar(name, SdfValueTypeNames->Color3fArray, interp);

    setPrimvar(
        primVar,
        assignmentIndices,
        VtValue(data),
        VtValue(UnauthoredColorSetRGB),
        usdTime,
        valueWriter);

    if (clamped) {
        UsdMayaRoundTripUtil::MarkPrimvarAsClamped(primVar);
    }

    return true;
}

bool UsdMayaMeshWriteUtils::createRGBAPrimVar(
    UsdGeomGprim&              primSchema,
    const TfToken&             name,
    const UsdTimeCode&         usdTime,
    const VtVec3fArray&        rgbData,
    const VtFloatArray&        alphaData,
    const TfToken&             interpolation,
    const VtIntArray&          assignmentIndices,
    bool                       clamped,
    UsdUtilsSparseValueWriter* valueWriter)
{
    const unsigned int numValues = rgbData.size();
    if (numValues == 0 || numValues != alphaData.size()) {
        return false;
    }

    TfToken interp = interpolation;
    if (numValues == 1 && interp == UsdGeomTokens->constant) {
        interp = TfToken();
    }

    UsdGeomPrimvar primVar
        = primSchema.CreatePrimvar(name, SdfValueTypeNames->Color4fArray, interp);

    VtArray<GfVec4f> rgbaData(numValues);
    for (size_t i = 0; i < rgbaData.size(); ++i) {
        rgbaData[i] = GfVec4f(rgbData[i][0], rgbData[i][1], rgbData[i][2], alphaData[i]);
    }

    setPrimvar(
        primVar,
        assignmentIndices,
        VtValue(rgbaData),
        VtValue(UnauthoredColorSetRGBA),
        usdTime,
        valueWriter);

    if (clamped) {
        UsdMayaRoundTripUtil::MarkPrimvarAsClamped(primVar);
    }

    return true;
}

bool UsdMayaMeshWriteUtils::createAlphaPrimVar(
    UsdGeomGprim&              primSchema,
    const TfToken&             name,
    const UsdTimeCode&         usdTime,
    const VtFloatArray&        data,
    const TfToken&             interpolation,
    const VtIntArray&          assignmentIndices,
    bool                       clamped,
    UsdUtilsSparseValueWriter* valueWriter)
{
    const unsigned int numValues = data.size();
    if (numValues == 0) {
        return false;
    }

    TfToken interp = interpolation;
    if (numValues == 1 && interp == UsdGeomTokens->constant) {
        interp = TfToken();
    }

    UsdGeomPrimvar primVar = primSchema.CreatePrimvar(name, SdfValueTypeNames->FloatArray, interp);
    setPrimvar(
        primVar,
        assignmentIndices,
        VtValue(data),
        VtValue(UnauthoredColorAlpha),
        usdTime,
        valueWriter);

    if (clamped) {
        UsdMayaRoundTripUtil::MarkPrimvarAsClamped(primVar);
    }

    return true;
}

bool UsdMayaMeshWriteUtils::getMeshColorSetData(
    MFnMesh&                       mesh,
    const MString&                 colorSet,
    bool                           isDisplayColor,
    const VtVec3fArray&            shadersRGBData,
    const VtFloatArray&            shadersAlphaData,
    const VtIntArray&              shadersAssignmentIndices,
    VtVec3fArray*                  colorSetRGBData,
    VtFloatArray*                  colorSetAlphaData,
    TfToken*                       interpolation,
    VtIntArray*                    colorSetAssignmentIndices,
    MFnMesh::MColorRepresentation* colorSetRep,
    bool*                          clamped)
{
    // If there are no colors, return immediately as failure.
    if (mesh.numColors(colorSet) == 0) {
        return false;
    }

    MColorArray  colorSetData;
    const MColor unsetColor(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX);
    if (mesh.getFaceVertexColors(colorSetData, &colorSet, &unsetColor) == MS::kFailure) {
        return false;
    }

    if (colorSetData.length() == 0) {
        return false;
    }

    // Get the color set representation and clamping.
    *colorSetRep = mesh.getColorRepresentation(colorSet);
    *clamped = mesh.isColorClamped(colorSet);

    // We'll populate the assignment indices for every face vertex, but we'll
    // only push values into the data if the face vertex has a value. All face
    // vertices are initially unassigned/unauthored.
    colorSetRGBData->clear();
    colorSetAlphaData->clear();
    colorSetAssignmentIndices->assign((size_t)colorSetData.length(), -1);
    *interpolation = UsdGeomTokens->faceVarying;

    // Loop over every face vertex to populate the value arrays.
    MItMeshFaceVertex itFV(mesh.object());
    unsigned int      fvi = 0;
    for (itFV.reset(); !itFV.isDone(); itFV.next(), ++fvi) {
        // If this is a displayColor color set, we may need to fallback on the
        // bound shader colors/alphas for this face in some cases. In
        // particular, if the color set is alpha-only, we fallback on the
        // shader values for the color. If the color set is RGB-only, we
        // fallback on the shader values for alpha only. If there's no authored
        // color for this face vertex, we use both the color AND alpha values
        // from the shader.
        bool useShaderColorFallback = false;
        bool useShaderAlphaFallback = false;
        if (isDisplayColor) {
            if (colorSetData[fvi] == unsetColor) {
                useShaderColorFallback = true;
                useShaderAlphaFallback = true;
            } else if (*colorSetRep == MFnMesh::kAlpha) {
                // The color set does not provide color, so fallback on shaders.
                useShaderColorFallback = true;
            } else if (*colorSetRep == MFnMesh::kRGB) {
                // The color set does not provide alpha, so fallback on shaders.
                useShaderAlphaFallback = true;
            }
        }

        // If we're exporting displayColor and we use the value from the color
        // set, we need to convert it to linear.
        bool convertDisplayColorToLinear = isDisplayColor;

        // Shader values for the mesh could be constant
        // (shadersAssignmentIndices is empty) or uniform.
        int faceIndex = itFV.faceId();
        if (useShaderColorFallback) {
            // There was no color value in the color set to use, so we use the
            // shader color, or the default color if there is no shader color.
            // This color will already be in linear space, so don't convert it
            // again.
            convertDisplayColorToLinear = false;

            int valueIndex = -1;
            if (shadersAssignmentIndices.empty()) {
                if (shadersRGBData.size() == 1) {
                    valueIndex = 0;
                }
            } else if (
                faceIndex >= 0
                && static_cast<size_t>(faceIndex) < shadersAssignmentIndices.size()) {

                int tmpIndex = shadersAssignmentIndices[faceIndex];
                if (tmpIndex >= 0 && static_cast<size_t>(tmpIndex) < shadersRGBData.size()) {
                    valueIndex = tmpIndex;
                }
            }
            if (valueIndex >= 0) {
                colorSetData[fvi][0] = shadersRGBData[valueIndex][0];
                colorSetData[fvi][1] = shadersRGBData[valueIndex][1];
                colorSetData[fvi][2] = shadersRGBData[valueIndex][2];
            } else {
                // No shader color to fallback on. Use the default shader color.
                colorSetData[fvi][0] = UnauthoredShaderRGB[0];
                colorSetData[fvi][1] = UnauthoredShaderRGB[1];
                colorSetData[fvi][2] = UnauthoredShaderRGB[2];
            }
        }
        if (useShaderAlphaFallback) {
            int valueIndex = -1;
            if (shadersAssignmentIndices.empty()) {
                if (shadersAlphaData.size() == 1) {
                    valueIndex = 0;
                }
            } else if (
                faceIndex >= 0
                && static_cast<size_t>(faceIndex) < shadersAssignmentIndices.size()) {
                int tmpIndex = shadersAssignmentIndices[faceIndex];
                if (tmpIndex >= 0 && static_cast<size_t>(tmpIndex) < shadersAlphaData.size()) {
                    valueIndex = tmpIndex;
                }
            }
            if (valueIndex >= 0) {
                colorSetData[fvi][3] = shadersAlphaData[valueIndex];
            } else {
                // No shader alpha to fallback on. Use the default shader alpha.
                colorSetData[fvi][3] = UnauthoredShaderAlpha;
            }
        }

        // If we have a color/alpha value, add it to the data to be returned.
        if (colorSetData[fvi] != unsetColor) {
            GfVec3f rgbValue = UnauthoredColorSetRGB;
            float   alphaValue = UnauthoredColorAlpha;

            if (useShaderColorFallback || (*colorSetRep == MFnMesh::kRGB)
                || (*colorSetRep == MFnMesh::kRGBA)) {
                rgbValue = LinearColorFromColorSet(colorSetData[fvi], convertDisplayColorToLinear);
            }
            if (useShaderAlphaFallback || (*colorSetRep == MFnMesh::kAlpha)
                || (*colorSetRep == MFnMesh::kRGBA)) {
                alphaValue = colorSetData[fvi][3];
            }

            colorSetRGBData->push_back(rgbValue);
            colorSetAlphaData->push_back(alphaValue);
            (*colorSetAssignmentIndices)[fvi] = colorSetRGBData->size() - 1;
        }
    }

    MergeEquivalentColorSetValues(colorSetRGBData, colorSetAlphaData, colorSetAssignmentIndices);

    UsdMayaUtil::CompressFaceVaryingPrimvarIndices(mesh, interpolation, colorSetAssignmentIndices);

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
