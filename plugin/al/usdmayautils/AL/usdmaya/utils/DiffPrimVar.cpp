//
// Copyright 2018 Animal Logic
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
#include "AL/usdmaya/utils/DiffPrimVar.h"

#include <mayaUsdUtils/DiffCore.h>
#include <mayaUsdUtils/SIMD.h>

#include <maya/MDoubleArray.h>
#include <maya/MFloatArray.h>
#include <maya/MIntArray.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MUintArray.h>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace MayaUsdUtils;

namespace AL {
namespace usdmaya {
namespace utils {

//----------------------------------------------------------------------------------------------------------------------
uint32_t diffGeom(UsdGeomPointBased& geom, MFnMesh& mesh, UsdTimeCode timeCode, uint32_t exportMask)
{
    uint32_t result = 0;
    if (exportMask & kPoints) {
        VtArray<GfVec3f> pointData;
        geom.GetPointsAttr().Get(&pointData, timeCode);

        MStatus            status;
        const float* const usdPoints = (const float* const)pointData.cdata();
        const float* const mayaPoints = (const float* const)mesh.getRawPoints(&status);
        const size_t       usdPointsCount = pointData.size();
        const size_t       mayaPointsCount = mesh.numVertices();
        if (mayaPointsCount != usdPointsCount) {
            result |= kPoints;
        } else if (!MayaUsdUtils::compareArray(
                       usdPoints, mayaPoints, usdPointsCount * 3, mayaPointsCount * 3)) {
            result |= kPoints;
        }
    }

    if (exportMask & kExtent) {
        MStatus      status;
        const float* pointsData = mesh.getRawPoints(&status);
        if (status) {
            const uint32_t   numVertices = mesh.numVertices();
            const GfVec3f*   vecData = reinterpret_cast<const GfVec3f*>(pointsData);
            VtArray<GfVec3f> points(vecData, vecData + numVertices);

            VtArray<GfVec3f> mayaExtent(2);
            UsdGeomPointBased::ComputeExtent(points, &mayaExtent);

            VtArray<GfVec3f> usdExtent(2);
            geom.GetExtentAttr().Get(&usdExtent, timeCode);

            const GfRange3f mayaRange(mayaExtent[0], mayaExtent[1]);
            const GfRange3f usdRange(usdExtent[0], usdExtent[1]);

            if (mayaRange != usdRange)
                result |= kExtent;
        }
    }

    if (exportMask & kNormals) {
        VtArray<GfVec3f> normalData;
        geom.GetNormalsAttr().Get(&normalData, timeCode);

        if (geom.GetNormalsInterpolation() == UsdGeomTokens->vertex) {
            VtArray<int32_t> indexData;
            UsdGeomMesh(geom.GetPrim()).GetFaceVertexIndicesAttr().Get(&indexData, timeCode);

            MStatus              status;
            const float* const   usdNormals = (const float*)normalData.cdata();
            const int32_t* const usdNormalIndices = (const int32_t*)indexData.cdata();
            if (usdNormals && usdNormalIndices) {
                if (size_t(mesh.numNormals()) != normalData.size()) {
                    result |= kNormals;
                } else {
                    MIntArray normalIds, normalCounts;
                    mesh.getNormalIds(normalCounts, normalIds);
                    const float* const mayaNormals = (const float*)mesh.getRawNormals(&status);
                    if (normalIds.length()) {
                        int32_t* const ptr = &normalIds[0];
                        const uint32_t n = indexData.size();
                        for (uint32_t i = 0; i < n; ++i) {
                            const float* const n0 = usdNormals + 3 * usdNormalIndices[i];
                            const float* const n1 = mayaNormals + 3 * ptr[i];
                            const float        dx = n0[0] - n1[0];
                            const float        dy = n0[1] - n1[1];
                            const float        dz = n0[2] - n1[2];
                            if (std::abs(dx) > 1e-5f || std::abs(dy) > 1e-5f
                                || std::abs(dz) > 1e-5f) {
                                result |= kNormals;
                                break;
                            }
                        }
                    }
                }
            }
        } else {
            MStatus            status;
            const float* const usdNormals = (const float* const)normalData.cdata();
            const float* const mayaNormals = (const float* const)mesh.getRawNormals(&status);
            const size_t       usdNormalsCount = normalData.size();
            const size_t       mayaNormalsCount = mesh.numNormals();
            if (!MayaUsdUtils::compareArray(
                    usdNormals, mayaNormals, usdNormalsCount * 3, mayaNormalsCount * 3)) {
                result |= kNormals;
            }
        }
    }
    return result;
}

//----------------------------------------------------------------------------------------------------------------------
uint32_t
diffFaceVertices(UsdGeomMesh& geom, MFnMesh& mesh, UsdTimeCode timeCode, uint32_t exportMask)
{
    uint32_t result = 0;

    if (exportMask & (kFaceVertexCounts | kFaceVertexIndices)) {
        const uint32_t numPolygons = uint32_t(mesh.numPolygons());
        const uint32_t numFaceVerts = uint32_t(mesh.numFaceVertices());

        VtArray<int> faceVertexCounts;
        VtArray<int> faceVertexIndices;
        UsdAttribute fvc = geom.GetFaceVertexCountsAttr();
        UsdAttribute fvi = geom.GetFaceVertexIndicesAttr();

        fvc.Get(&faceVertexCounts, timeCode);
        fvi.Get(&faceVertexIndices, timeCode);

        if (numPolygons == faceVertexCounts.size() && numFaceVerts == faceVertexIndices.size()) {
            const int* const pFaceVertexCounts = faceVertexCounts.cdata();
            MIntArray        vertexCount;
            MIntArray        vertexList;
            if (mesh.getVertices(vertexCount, vertexList)) { }

            if (numPolygons
                && !MayaUsdUtils::compareArray(
                    &vertexCount[0], pFaceVertexCounts, numPolygons, numPolygons)) {
                result |= kFaceVertexCounts;
            }

            const int* const pFaceVertexIndices = faceVertexIndices.cdata();
            if (numFaceVerts
                && !MayaUsdUtils::compareArray(
                    &vertexList[0], pFaceVertexIndices, numFaceVerts, numFaceVerts)) {
                result |= kFaceVertexIndices;
            }
        } else if (
            numPolygons != faceVertexCounts.size() && numFaceVerts == faceVertexIndices.size()) {
            // I'm going to test this, but I suspect it's impossible
            result |= (kFaceVertexIndices | kFaceVertexCounts);
        } else if (
            numPolygons == faceVertexCounts.size() && numFaceVerts != faceVertexIndices.size()) {
            // If the number of face verts have changed, but the number of polygons remains the
            // same, then since numFaceVerts = sum(faceVertexCounts), we can assume that one of the
            // faceVertexCounts elements must have changed.
            result |= (kFaceVertexIndices | kFaceVertexCounts);
        } else {
            // counts differ, no point in checking actual values, we'll just update the new values
            result |= (kFaceVertexIndices | kFaceVertexCounts);
        }
    }

    //
    if (exportMask & kHoleIndices) {
        VtArray<int> holeIndices;
        UsdAttribute holesAttr = geom.GetHoleIndicesAttr();
        holesAttr.Get(&holeIndices, timeCode);

        MUintArray mayaHoleIndices = mesh.getInvisibleFaces();

        const uint32_t numHoleIndices = holeIndices.size();
        const uint32_t numMayaHoleIndices = mayaHoleIndices.length();
        if (numMayaHoleIndices != numHoleIndices) {
            result |= kHoleIndices;
        } else if (
            numMayaHoleIndices
            && !MayaUsdUtils::compareArray(
                (int32_t*)&mayaHoleIndices[0],
                holeIndices.cdata(),
                numMayaHoleIndices,
                numHoleIndices)) {
            result |= kHoleIndices;
        }
    }

    if (exportMask & (kCreaseWeights | kCreaseIndices)) {
        MUintArray   mayaEdgeCreaseIndices;
        MDoubleArray mayaCreaseWeights;
        mesh.getCreaseEdges(mayaEdgeCreaseIndices, mayaCreaseWeights);

        if (exportMask & kCreaseIndices) {
            VtArray<int> creasesIndices;
            UsdAttribute creasesAttr = geom.GetCreaseIndicesAttr();
            creasesAttr.Get(&creasesIndices, timeCode);

            const uint32_t numCreaseIndices = creasesIndices.size();
            MUintArray     mayaCreaseIndices;
            mayaCreaseIndices.setLength(mayaEdgeCreaseIndices.length() * 2);
            for (uint32_t i = 0, n = mayaEdgeCreaseIndices.length(); i < n; ++i) {
                int2 edge;
                mesh.getEdgeVertices(mayaEdgeCreaseIndices[i], edge);
                mayaCreaseIndices[2 * i] = edge[0];
                mayaCreaseIndices[2 * i + 1] = edge[1];
            }

            const uint32_t numMayaCreaseIndices = mayaCreaseIndices.length();
            if (numMayaCreaseIndices != numCreaseIndices) {
                result |= kCreaseIndices;
            } else if (
                numMayaCreaseIndices
                && !MayaUsdUtils::compareArray(
                    (const int32_t*)&mayaCreaseIndices[0],
                    creasesIndices.cdata(),
                    numMayaCreaseIndices,
                    numCreaseIndices)) {
                result |= kCreaseIndices;
            }
        }

        if (exportMask & kCreaseWeights) {
            VtArray<float> creasesWeights;
            UsdAttribute   creasesAttr = geom.GetCreaseSharpnessesAttr();
            creasesAttr.Get(&creasesWeights, timeCode);

            const uint32_t numCreaseWeights = creasesWeights.size();
            const uint32_t numMayaCreaseWeights = mayaCreaseWeights.length();
            if (numMayaCreaseWeights != numCreaseWeights) {
                result |= kCreaseWeights;
            } else if (
                numMayaCreaseWeights
                && !MayaUsdUtils::compareArray(
                    &mayaCreaseWeights[0],
                    creasesWeights.cdata(),
                    numMayaCreaseWeights,
                    numCreaseWeights)) {
                result |= kCreaseWeights;
            }
        }
    }

    //
    if (exportMask & (kCornerIndices | kCornerSharpness)) {
        UsdAttribute cornerIndices = geom.GetCornerIndicesAttr();
        UsdAttribute cornerSharpness = geom.GetCornerSharpnessesAttr();

        VtArray<int32_t> vertexIdValues;
        VtArray<float>   creaseValues;
        cornerIndices.Get(&vertexIdValues);
        cornerSharpness.Get(&creaseValues);

        MUintArray   mayaVertexIdValues;
        MDoubleArray mayaCreaseValues;
        mesh.getCreaseVertices(mayaVertexIdValues, mayaCreaseValues);

        const uint32_t numVertexIds = vertexIdValues.size();
        const uint32_t numMayaVertexIds = mayaVertexIdValues.length();

        if (numVertexIds != numMayaVertexIds) {
            result |= kCornerIndices;
        } else {
            if (numMayaVertexIds
                && !MayaUsdUtils::compareArray(
                    (const int32_t*)&mayaVertexIdValues[0],
                    vertexIdValues.cdata(),
                    numVertexIds,
                    numMayaVertexIds)) {
                result |= kCornerIndices;
            }
        }

        const uint32_t numCreaseValues = creaseValues.size();
        const uint32_t numMayaCreaseValues = mayaCreaseValues.length();
        if (numCreaseValues != numMayaCreaseValues) {
            result |= kCornerSharpness;
        } else {
            if (numMayaCreaseValues
                && !MayaUsdUtils::compareArray(
                    &mayaCreaseValues[0],
                    creaseValues.cdata(),
                    numCreaseValues,
                    numMayaCreaseValues)) {
                result |= kCornerSharpness;
            }
        }
    }
    return result;
}

//----------------------------------------------------------------------------------------------------------------------
///
//----------------------------------------------------------------------------------------------------------------------
struct UsdColourSetDefinition
{
    UsdColourSetDefinition(const UsdGeomPrimvar& primvar)
        : m_primVar(primvar)
    {
        primvar.GetDeclarationInfo(&m_name, &m_typeName, &m_interpolation, &m_elementSize);
        m_mayaInterpolation = m_interpolation;
    }

    void extractColourDataFromMaya(const MFnMesh& mesh, MString* mayaSetNamePtr)
    {
        MFnMesh::MColorRepresentation representation = mesh.getColorRepresentation(*mayaSetNamePtr);
        isRGB = MFnMesh::kRGB == representation;
        MIntArray faceCounts, pointIndices;
        mesh.getVertices(faceCounts, pointIndices);
        MItMeshPolygon it(mesh.object());
        while (!it.isDone()) {
            MColorArray faceColours;
            it.getColors(faceColours, mayaSetNamePtr);
            it.next();
            // Append face colours
            uint32_t offset = m_colours.length();
            m_colours.setLength(offset + faceColours.length());
            for (uint32_t j = 0, n = faceColours.length(); j < n; ++j)
                m_colours[offset + j] = faceColours[j];
        }
        m_mayaInterpolation = guessColourSetInterpolationTypeExtensive(
            &m_colours[0].r,
            m_colours.length(),
            mesh.numVertices(),
            pointIndices,
            faceCounts,
            m_indicesToExtract);

        // if we have an array of colours, modify them now
        if (!m_indicesToExtract.empty()) {
            MColorArray newColours;
            newColours.setLength(m_indicesToExtract.size());
            MColor* optr = &newColours[0];
            MColor* iptr = &m_colours[0];
            auto    indices = m_indicesToExtract.data();
            for (size_t i = 0, n = m_indicesToExtract.size(); i < n; ++i) {
                optr[i] = iptr[indices[i]];
            }
        }
        if (m_mayaInterpolation == UsdGeomTokens->constant) {
            m_colours.setLength(1);
        }
    }

    UsdGeomPrimvar        m_primVar;
    TfToken               m_name;
    TfToken               m_interpolation;
    TfToken               m_mayaInterpolation;
    SdfValueTypeName      m_typeName;
    MColorArray           m_colours;
    MIntArray             m_mayaUvCounts;
    MIntArray             m_mayaUvIndices;
    std::vector<uint32_t> m_indicesToExtract;
    int                   m_elementSize;
    bool                  isRGB;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  a utility to construct the diff reports on a UV set.
//----------------------------------------------------------------------------------------------------------------------
class ColourSetBuilder
{
public:
    std::vector<UsdColourSetDefinition> m_existingSetDefinitions;
    MStringArray                        m_existingSetNames;

    std::vector<TfToken> m_interpolations;
    std::vector<TfToken> m_mayaInterpolation;

    /// \brief  search through the list of Maya UV sets, and determine if a matching set exists in
    /// the prim vars \param  setNames the list of Maya UV sets \param  primvars the list of prim
    /// vars to search.
    void
    constructNewlyAddedSets(MStringArray& setNames, const std::vector<UsdGeomPrimvar>& primvars);

    /// \brief  reads the UV set data from the specified mesh
    /// \param  mesh the mesh to extract the data from
    void extractMayaData(const MFnMesh& mesh);

    /// \brief  performs the diff between
    void performDiffTest(PrimVarDiffReport& report);
};

//----------------------------------------------------------------------------------------------------------------------
void ColourSetBuilder::constructNewlyAddedSets(
    MStringArray&                      setNames,
    const std::vector<UsdGeomPrimvar>& primvars)
{
    for (int i = 0; unsigned(i) < setNames.length(); ++i) {
        for (auto it = primvars.begin(), end = primvars.end(); it != end; ++it) {
            UsdColourSetDefinition definition(*it);

            if (definition.m_name.GetString() == setNames[i].asChar()) {
                m_existingSetNames.append(setNames[i]);
                m_existingSetDefinitions.push_back(definition);
                setNames.remove(i);
                --i;
                break;
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ColourSetBuilder::extractMayaData(const MFnMesh& mesh)
{
    for (uint32_t i = 0, n = m_existingSetDefinitions.size(); i < n; ++i) {
        m_existingSetDefinitions[i].extractColourDataFromMaya(mesh, &m_existingSetNames[i]);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ColourSetBuilder::performDiffTest(PrimVarDiffReport& report)
{
    for (uint32_t i = 0, n = m_existingSetDefinitions.size(); i < n; ++i) {
        auto& definition = m_existingSetDefinitions[i];
        if (definition.m_interpolation != definition.m_mayaInterpolation) {
            // if the interpolation value has changed from the original data, the entire set will
            // need to be exported.
            if (definition.m_indicesToExtract.empty()) {
                report.emplace_back(
                    definition.m_primVar,
                    m_existingSetNames[i],
                    true,
                    false,
                    true,
                    definition.m_mayaInterpolation);
            } else {
                report.emplace_back(
                    definition.m_primVar,
                    m_existingSetNames[i],
                    true,
                    false,
                    true,
                    definition.m_mayaInterpolation,
                    std::move(definition.m_indicesToExtract));
            }
        } else {
            VtValue vtValue;
            if (definition.m_primVar.Get(&vtValue, UsdTimeCode::Default())) {
                if (vtValue.IsHolding<VtArray<GfVec3f>>()) {
                    const VtArray<GfVec3f> rawVal = vtValue.Get<VtArray<GfVec3f>>();
                    if (!MayaUsdUtils::compareArray(
                            (const double*)rawVal.cdata(),
                            &definition.m_colours[0].r,
                            rawVal.size(),
                            definition.m_colours.length())) {
                        report.emplace_back(
                            definition.m_primVar,
                            m_existingSetNames[i],
                            false,
                            false,
                            true,
                            definition.m_mayaInterpolation,
                            std::move(definition.m_indicesToExtract));
                    }
                } else if (vtValue.IsHolding<VtArray<GfVec4f>>()) {
                    const VtArray<GfVec4f> rawVal = vtValue.Get<VtArray<GfVec4f>>();
                    if (!MayaUsdUtils::compareArray(
                            &definition.m_colours[0].r,
                            (const float*)rawVal.cdata(),
                            definition.m_colours.length() * 4,
                            rawVal.size() * 4)) {
                        report.emplace_back(
                            definition.m_primVar,
                            m_existingSetNames[i],
                            false,
                            false,
                            true,
                            definition.m_mayaInterpolation,
                            std::move(definition.m_indicesToExtract));
                    }
                } else if (vtValue.IsHolding<VtArray<GfVec3d>>()) {
                    const VtArray<GfVec3d> rawVal = vtValue.Get<VtArray<GfVec3d>>();
                    if (!MayaUsdUtils::compareArray(
                            (const double*)rawVal.cdata(),
                            &definition.m_colours[0].r,
                            rawVal.size(),
                            definition.m_colours.length())) {
                        report.emplace_back(
                            definition.m_primVar,
                            m_existingSetNames[i],
                            false,
                            false,
                            true,
                            definition.m_mayaInterpolation,
                            std::move(definition.m_indicesToExtract));
                    }
                } else if (vtValue.IsHolding<VtArray<GfVec4d>>()) {
                    const VtArray<GfVec4d> rawVal = vtValue.Get<VtArray<GfVec4d>>();
                    if (!MayaUsdUtils::compareArray(
                            &definition.m_colours[0].r,
                            (const float*)rawVal.cdata(),
                            definition.m_colours.length() * 4,
                            rawVal.size() * 4)) {
                        report.emplace_back(
                            definition.m_primVar,
                            m_existingSetNames[i],
                            false,
                            false,
                            true,
                            definition.m_mayaInterpolation,
                            std::move(definition.m_indicesToExtract));
                    }
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
MStringArray hasNewColourSet(UsdGeomMesh& geom, MFnMesh& mesh, PrimVarDiffReport& report)
{
    const std::vector<UsdGeomPrimvar> primvars = geom.GetPrimvars();
    MStringArray                      setNames;
    mesh.getColorSetNames(setNames);

    ColourSetBuilder builder;
    builder.constructNewlyAddedSets(setNames, primvars);
    builder.extractMayaData(mesh);
    builder.performDiffTest(report);
    return setNames;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
struct UsdUvSetDefinition
{
    UsdUvSetDefinition(const UsdGeomPrimvar& primvar)
        : m_primVar(primvar)
    {
        primvar.GetDeclarationInfo(&m_name, &m_typeName, &m_interpolation, &m_elementSize);
        m_mayaInterpolation = m_interpolation;
    }

    void extractUvDataFromMaya(const MFnMesh& mesh, MString* mayaSetNamePtr)
    {
        MIntArray pointIndices, faceCounts;
        mesh.getVertices(faceCounts, pointIndices);
        mesh.getUVs(m_u, m_v, mayaSetNamePtr);
        mesh.getAssignedUVs(m_mayaUvCounts, m_mayaUvIndices, mayaSetNamePtr);
        m_mayaInterpolation = guessUVInterpolationTypeExtensive(
            m_u, m_v, m_mayaUvIndices, pointIndices, m_mayaUvCounts, m_indicesToExtract);
    }

    UsdGeomPrimvar        m_primVar;
    TfToken               m_name;
    TfToken               m_interpolation;
    TfToken               m_mayaInterpolation;
    SdfValueTypeName      m_typeName;
    MFloatArray           m_u;
    MFloatArray           m_v;
    MIntArray             m_mayaUvCounts;
    MIntArray             m_mayaUvIndices;
    std::vector<uint32_t> m_indicesToExtract;
    int                   m_elementSize;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  a utility to construct the diff reports on a UV set.
//----------------------------------------------------------------------------------------------------------------------
class UvSetBuilder
{
public:
    std::vector<UsdUvSetDefinition> m_existingSetDefinitions;
    MStringArray                    m_existingSetNames;

    std::vector<TfToken> m_interpolations;
    std::vector<TfToken> m_mayaInterpolation;

    /// \brief  search through the list of Maya UV sets, and determine if a matching set exists in
    /// the prim vars \param  setNames the list of Maya UV sets \param  primvars the list of prim
    /// vars to search.
    void
    constructNewlyAddedSets(MStringArray& setNames, const std::vector<UsdGeomPrimvar>& primvars);

    /// \brief  reads the UV set data from the specified mesh
    /// \param  mesh the mesh to extract the data from
    void extractMayaUvData(const MFnMesh& mesh);

    /// \brief  performs the diff between
    void performDiffTest(PrimVarDiffReport& report);
};

//----------------------------------------------------------------------------------------------------------------------
void UvSetBuilder::constructNewlyAddedSets(
    MStringArray&                      setNames,
    const std::vector<UsdGeomPrimvar>& primvars)
{
    for (uint32_t i = 0; i < setNames.length(); ++i) {
        for (auto it = primvars.begin(), end = primvars.end(); it != end; ++it) {
            UsdUvSetDefinition definition(*it);

            MString uvSetName = setNames[i];
            if (uvSetName == "map1") {
                uvSetName = "st";
            }

            if (definition.m_name.GetString() == uvSetName.asChar()) {
                m_existingSetNames.append(setNames[i]);
                m_existingSetDefinitions.push_back(definition);
                setNames.remove(i);
                --i;
                break;
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void UvSetBuilder::extractMayaUvData(const MFnMesh& mesh)
{
    for (uint32_t i = 0, n = m_existingSetDefinitions.size(); i < n; ++i) {
        m_existingSetDefinitions[i].extractUvDataFromMaya(mesh, &m_existingSetNames[i]);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void UvSetBuilder::performDiffTest(PrimVarDiffReport& report)
{
    for (uint32_t i = 0, n = m_existingSetDefinitions.size(); i < n; ++i) {
        auto& definition = m_existingSetDefinitions[i];
        if (definition.m_interpolation != definition.m_mayaInterpolation) {
            // if the interpolation value has changed from the original data, the entire set will
            // need to be exported.
            if (definition.m_indicesToExtract.empty()) {
                report.emplace_back(
                    definition.m_primVar,
                    m_existingSetNames[i],
                    false,
                    true,
                    true,
                    definition.m_mayaInterpolation);
            } else {
                report.emplace_back(
                    definition.m_primVar,
                    m_existingSetNames[i],
                    false,
                    true,
                    true,
                    definition.m_mayaInterpolation,
                    std::move(definition.m_indicesToExtract));
            }
        } else {
            VtValue vtValue;
            if (definition.m_primVar.Get(&vtValue, UsdTimeCode::Default())) {
                if (vtValue.IsHolding<VtArray<GfVec2f>>()) {
                    const VtArray<GfVec2f> rawVal = vtValue.Get<VtArray<GfVec2f>>();
                    if (definition.m_interpolation == UsdGeomTokens->constant) {
                        if (rawVal[0][0] != definition.m_u[0]
                            || rawVal[0][1] != definition.m_v[0]) {
                            report.emplace_back(
                                definition.m_primVar,
                                m_existingSetNames[i],
                                false,
                                false,
                                true,
                                definition.m_mayaInterpolation);
                        }
                    } else if (definition.m_interpolation == UsdGeomTokens->faceVarying) {
                        VtIntArray usdindices;
                        definition.m_primVar.GetIndices(&usdindices);
                        bool uv_indices_have_changed = false;
                        bool data_has_changed = false;
                        if (!MayaUsdUtils::compareArray(
                                (const int32_t*)&definition.m_mayaUvIndices[0],
                                usdindices.cdata(),
                                definition.m_mayaUvIndices.length(),
                                usdindices.size())) {
                            uv_indices_have_changed = true;
                        }

                        if (!MayaUsdUtils::compareUvArray(
                                (const float*)&definition.m_u[0],
                                (const float*)&definition.m_v[0],
                                (const float*)rawVal.cdata(),
                                rawVal.size(),
                                definition.m_u.length())) {
                            data_has_changed = true;
                        }
                        if (data_has_changed || uv_indices_have_changed) {
                            report.emplace_back(
                                definition.m_primVar,
                                m_existingSetNames[i],
                                false,
                                uv_indices_have_changed,
                                data_has_changed,
                                definition.m_mayaInterpolation);
                        }
                    } else {
                        if (!MayaUsdUtils::compareUvArray(
                                (const float*)&definition.m_u[0],
                                (const float*)&definition.m_v[0],
                                (const float*)rawVal.cdata(),
                                rawVal.size(),
                                definition.m_u.length())) {
                            if (definition.m_indicesToExtract.empty()) {
                                report.emplace_back(
                                    definition.m_primVar,
                                    m_existingSetNames[i],
                                    false,
                                    false,
                                    true,
                                    definition.m_mayaInterpolation);
                            } else {
                                report.emplace_back(
                                    definition.m_primVar,
                                    m_existingSetNames[i],
                                    false,
                                    false,
                                    true,
                                    definition.m_mayaInterpolation,
                                    std::move(definition.m_indicesToExtract));
                            }
                        }
                    }
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
MStringArray hasNewUvSet(UsdGeomMesh& geom, const MFnMesh& mesh, PrimVarDiffReport& report)
{
    const std::vector<UsdGeomPrimvar> primvars = geom.GetPrimvars();
    MStringArray                      setNames;
    mesh.getUVSetNames(setNames);

    UvSetBuilder builder;
    builder.constructNewlyAddedSets(setNames, primvars);
    builder.extractMayaUvData(mesh);
    builder.performDiffTest(report);
    return setNames;
}

//----------------------------------------------------------------------------------------------------------------------
TfToken guessUVInterpolationType(
    MFloatArray& u,
    MFloatArray& v,
    MIntArray&   indices,
    MIntArray&   pointIndices)
{
    // if UV coords are all identical, we have a constant value
    if (MayaUsdUtils::vec2AreAllTheSame(&u[0], &v[0], u.length())) {
        return UsdGeomTokens->constant;
    }

    // if the UV indices match the vertex indices, we have per-vertex assignment
    if (MayaUsdUtils::compareArray(
            &indices[0], &pointIndices[0], indices.length(), pointIndices.length())) {
        return UsdGeomTokens->vertex;
    }

    return UsdGeomTokens->faceVarying;
}

//----------------------------------------------------------------------------------------------------------------------
TfToken guessUVInterpolationTypeExtended(
    MFloatArray& u,
    MFloatArray& v,
    MIntArray&   indices,
    MIntArray&   pointIndices,
    MIntArray&   faceCounts)
{
    TfToken type = guessUVInterpolationType(u, v, indices, pointIndices);
    if (type != UsdGeomTokens->faceVarying) {
        return type;
    }

    // let's see whether we have a uniform UV set (based on the assumption that each face will have
    // unique UV indices)
    {
        uint32_t offset = 0;
        for (uint32_t i = 0, n = faceCounts.length(); i < n; ++i) {
            const uint32_t numVerts = faceCounts[i];
            int32_t        index = indices[offset];
            for (uint32_t j = 1; j < numVerts; ++j) {
                if (index != indices[offset + j]) {
                    goto face_varying;
                }
            }
            offset += numVerts;
        }
        return UsdGeomTokens->uniform;
    }

face_varying:
    return UsdGeomTokens->faceVarying;
}

//----------------------------------------------------------------------------------------------------------------------
TfToken guessUVInterpolationTypeExtensive(
    MFloatArray&           u,
    MFloatArray&           v,
    MIntArray&             indices,
    MIntArray&             pointIndices,
    MIntArray&             faceCounts,
    std::vector<uint32_t>& indicesToExtract)
{
    // sanity check on input arrays
    if (indices.length() == 0 || pointIndices.length() == 0 || u.length() == 0 || v.length() == 0
        || faceCounts.length() == 0) {
        TF_RUNTIME_ERROR("Unable to process mesh UV's - Invalid array lengths provided");
        return UsdGeomTokens->faceVarying;
    }

    // if UV coords are all identical, we have a constant value
    if (MayaUsdUtils::vec2AreAllTheSame(&u[0], &v[0], u.length())) {
        return UsdGeomTokens->constant;
    }

    std::map<int32_t, int32_t> indicesMap;

    // do an exhaustive test to see if the UV assignments are per-vertex
    {
        indicesMap.emplace(pointIndices[0], indices[0]);
        for (uint32_t i = 1, n = pointIndices.length(); i < n; ++i) {
            auto index = pointIndices[i];
            auto uvindex = indices[i];
            auto it = indicesMap.find(index);

            // if not found, insert new entry in map
            if (it == indicesMap.end()) {
                indicesMap.emplace(index, uvindex);
            } else {
                // if the UV index matches, we have the same assignment
                if (uvindex != it->second) {
                    // if not, check to see if the indices differ, but the values are the same
                    const float u0 = u[it->second];
                    const float v0 = v[it->second];
                    const float u1 = u[uvindex];
                    const float v1 = v[uvindex];
                    if (u0 != u1 || v0 != v1) {
                        goto uniform_test;
                    }
                }
            }
        }

        std::vector<uint32_t> tempIndicesToExtract(indicesMap.size());
        auto                  iter = indicesMap.begin();
        for (size_t i = 0; i < tempIndicesToExtract.size(); ++i, ++iter) {
            tempIndicesToExtract[i] = iter->second;
        }
        std::swap(indicesToExtract, tempIndicesToExtract);

        return UsdGeomTokens->vertex;
    }

uniform_test:

    // An exhaustive test to see if we have per-face assignment of UVs
    {
        uint32_t offset = 0;
        for (uint32_t i = 0, n = faceCounts.length(); i < n; ++i) {
            const uint32_t numVerts = faceCounts[i];
            const int32_t  index = indices[offset];

            // extract UV for first vertex in face
            const float u0 = u[index];
            const float v0 = v[index];

            // process each face
            for (uint32_t j = 1; j < numVerts; ++j) {
                const int32_t next_index = indices[offset + j];

                // if the indices don't match
                if (index != next_index) {
                    // check the UV values directly
                    const float u1 = u[next_index];
                    const float v1 = v[next_index];
                    if (u0 != u1 || v0 != v1) {
                        goto face_varying;
                    }
                }
            }
            offset += numVerts;
        }
        return UsdGeomTokens->uniform;
    }

face_varying:
    return UsdGeomTokens->faceVarying;
}

//----------------------------------------------------------------------------------------------------------------------
TfToken guessVec4InterpolationType(
    const float* xyzw,
    size_t       numElements,
    MIntArray&   indices,
    MIntArray&   pointIndices)
{
    // if UV coords are all identical, we have a constant value
    if (MayaUsdUtils::vec4AreAllTheSame(xyzw, numElements)) {
        return UsdGeomTokens->constant;
    }

    // if the UV indices match the vertex indices, we have per-vertex assignment
    if (MayaUsdUtils::compareArray(
            &indices[0], &pointIndices[0], indices.length(), pointIndices.length())) {
        return UsdGeomTokens->vertex;
    }
    return UsdGeomTokens->faceVarying;
}

//----------------------------------------------------------------------------------------------------------------------
TfToken guessVec4InterpolationTypeExtended(
    const float* xyzw,
    size_t       numElements,
    MIntArray&   indices,
    MIntArray&   pointIndices,
    MIntArray&   faceCounts)
{
    TfToken type = guessVec4InterpolationType(xyzw, numElements, indices, pointIndices);
    if (type != UsdGeomTokens->faceVarying) {
        return type;
    }

    // let's see whether we have a uniform UV set (based on the assumption that each face will have
    // unique indices)
    {
        uint32_t offset = 0;
        for (uint32_t i = 0, n = faceCounts.length(); i < n; ++i) {
            const uint32_t numVerts = faceCounts[i];
            int32_t        index = indices[offset];
            for (uint32_t j = 1; j < numVerts; ++j) {
                if (index != indices[offset + j]) {
                    goto face_varying;
                }
            }
            offset += numVerts;
        }
        return UsdGeomTokens->uniform;
    }

face_varying:
    return UsdGeomTokens->faceVarying;
}
//----------------------------------------------------------------------------------------------------------------------
TfToken guessVec4InterpolationTypeExtensive(
    const float* xyzw,
    size_t       numElements,
    MIntArray&   indices,
    MIntArray&   pointIndices,
    MIntArray&   faceCounts)
{
    // if prim vars are all identical, we have a constant value
    if (MayaUsdUtils::vec4AreAllTheSame(xyzw, numElements)) {
        return UsdGeomTokens->constant;
    }

    std::unordered_map<int32_t, int32_t> indicesMap;

    // do an exhaustive test to see if the UV assignments are per-vertex
    {
        indicesMap.emplace(pointIndices[0], indices[0]);
        for (uint32_t i = 1, n = pointIndices.length(); i < n; ++i) {
            auto index = pointIndices[i];
            auto xyzwindex = indices[i];
            auto it = indicesMap.find(index);

            // if not found, insert new entry in map
            if (it == indicesMap.end()) {
                indicesMap.emplace(index, xyzwindex);
            } else {
                // if the index matches, we have the same assignment
                if (xyzwindex != it->second) {
#if defined(__SSE__)

                    const f128 xyzw0 = loadu4f(xyzw + 4 * it->second);
                    const f128 xyzw1 = loadu4f(xyzw + 4 * xyzwindex);
                    const f128 cmp = cmpne4f(xyzw0, xyzw1);
                    if (movemask4f(cmp))
                        goto uniform_test;

#else

                    // if not, check to see if the indices differ, but the values are the same
                    const float x0 = xyzw[4 * it->second];
                    const float y0 = xyzw[4 * it->second + 1];
                    const float z0 = xyzw[4 * it->second + 2];
                    const float w0 = xyzw[4 * it->second + 3];
                    const float x1 = xyzw[4 * xyzwindex];
                    const float y1 = xyzw[4 * xyzwindex + 1];
                    const float z1 = xyzw[4 * xyzwindex + 2];
                    const float w1 = xyzw[4 * xyzwindex + 3];
                    if (x0 != x1 || y0 != y1 || z0 != z1 || w0 != w1)
                        goto uniform_test;

#endif
                }
            }
        }
        return UsdGeomTokens->vertex;
    }

uniform_test:

    // An exhaustive test to see if we have per-face assignment of UVs
    {
        uint32_t offset = 0;
        for (uint32_t i = 0, n = faceCounts.length(); i < n; ++i) {
            const uint32_t numVerts = faceCounts[i];
            const int32_t  index = indices[offset];

// extract UV for first vertex in face
#if defined(__SSE__)

            const f128 xyzw0 = loadu4f(xyzw + 4 * index);

#else

            const float x0 = xyzw[4 * index];
            const float y0 = xyzw[4 * index + 1];
            const float z0 = xyzw[4 * index + 2];
            const float w0 = xyzw[4 * index + 3];

#endif

            // process each face
            for (uint32_t j = 1; j < numVerts; ++j) {
                const int32_t next_index = indices[offset + j];

                // if the indices don't match
                if (index != next_index) {
#if defined(__SSE__)

                    const f128 xyzw1 = loadu4f(xyzw + 4 * next_index);
                    const f128 cmp = cmpne4f(xyzw0, xyzw1);
                    if (movemask4f(cmp))
                        goto face_varying;

#else
                    // check the UV values directly
                    const float x1 = xyzw[4 * next_index];
                    const float y1 = xyzw[4 * next_index + 1];
                    const float z1 = xyzw[4 * next_index + 2];
                    const float w1 = xyzw[4 * next_index + 3];
                    if (x0 != x1 || y0 != y1 || z0 != z1 || w0 != w1)
                        goto face_varying;

#endif
                }
            }
            offset += numVerts;
        }
        return UsdGeomTokens->uniform;
    }

face_varying:
    return UsdGeomTokens->faceVarying;
}

//----------------------------------------------------------------------------------------------------------------------
TfToken guessVec4InterpolationType(
    const double* xyzw,
    size_t        numElements,
    MIntArray&    indices,
    MIntArray&    pointIndices)
{
    // if UV coords are all identical, we have a constant value
    if (MayaUsdUtils::vec4AreAllTheSame(xyzw, numElements)) {
        return UsdGeomTokens->constant;
    }

    // if the UV indices match the vertex indices, we have per-vertex assignment
    if (MayaUsdUtils::compareArray(
            &indices[0], &pointIndices[0], indices.length(), pointIndices.length())) {
        return UsdGeomTokens->vertex;
    }
    return UsdGeomTokens->faceVarying;
}

//----------------------------------------------------------------------------------------------------------------------
TfToken guessVec4InterpolationTypeExtended(
    const double* xyzw,
    size_t        numElements,
    MIntArray&    indices,
    MIntArray&    pointIndices,
    MIntArray&    faceCounts)
{
    TfToken type = guessVec4InterpolationType(xyzw, numElements, indices, pointIndices);
    if (type != UsdGeomTokens->faceVarying) {
        return type;
    }

    // let's see whether we have a uniform UV set (based on the assumption that each face will have
    // unique UV indices)
    {
        uint32_t offset = 0;
        for (uint32_t i = 0, n = faceCounts.length(); i < n; ++i) {
            const uint32_t numVerts = faceCounts[i];
            int32_t        index = indices[offset];
            for (uint32_t j = 1; j < numVerts; ++j) {
                if (index != indices[offset + j]) {
                    goto face_varying;
                }
            }
            offset += numVerts;
        }
        return UsdGeomTokens->uniform;
    }

face_varying:
    return UsdGeomTokens->faceVarying;
}

//----------------------------------------------------------------------------------------------------------------------
TfToken guessVec4InterpolationTypeExtensive(
    const double* xyzw,
    size_t        numElements,
    MIntArray&    indices,
    MIntArray&    pointIndices,
    MIntArray&    faceCounts)
{
    // if prim vars are all identical, we have a constant value
    if (MayaUsdUtils::vec4AreAllTheSame(xyzw, numElements)) {
        return UsdGeomTokens->constant;
    }

    std::unordered_map<int32_t, int32_t> indicesMap;

    // do an exhaustive test to see if the assignments are per-vertex
    {
        indicesMap.emplace(pointIndices[0], indices[0]);
        for (uint32_t i = 1, n = pointIndices.length(); i < n; ++i) {
            auto index = pointIndices[i];
            auto xyzindex = indices[i];
            auto it = indicesMap.find(index);

            // if not found, insert new entry in map
            if (it == indicesMap.end()) {
                indicesMap.emplace(index, xyzindex);
            } else {
                // if the index matches, we have the same assignment
                if (xyzindex != it->second) {
#if defined(__AVX__)

                    const d256 xyzw0 = loadu4d(xyzw + 4 * it->second);
                    const d256 xyzw1 = loadu4d(xyzw + 4 * xyzindex);
                    const d256 cmp = cmpne4d(xyzw0, xyzw1);
                    if (movemask4d(cmp))
                        goto uniform_test;

#elif defined(__SSE__)

                    const d128 xy0 = loadu2d(xyzw + 4 * it->second);
                    const d128 zw0 = loadu2d(xyzw + 4 * it->second + 2);
                    const d128 xy1 = loadu2d(xyzw + 4 * xyzindex);
                    const d128 zw1 = loadu2d(xyzw + 4 * xyzindex + 2);
                    const d128 cmp = or2d(cmpne2d(xy0, xy1), cmpne2d(zw0, zw1));
                    if (movemask2d(cmp))
                        goto uniform_test;

#else

                    // if not, check to see if the indices differ, but the values are the same
                    const double x0 = xyzw[4 * it->second];
                    const double y0 = xyzw[4 * it->second + 1];
                    const double z0 = xyzw[4 * it->second + 2];
                    const double w0 = xyzw[4 * it->second + 3];
                    const double x1 = xyzw[4 * xyzindex];
                    const double y1 = xyzw[4 * xyzindex + 1];
                    const double z1 = xyzw[4 * xyzindex + 2];
                    const double w1 = xyzw[4 * xyzindex + 3];
                    if (x0 != x1 || y0 != y1 || z0 != z1 || w0 != w1)
                        goto uniform_test;

#endif
                }
            }
        }
        return UsdGeomTokens->vertex;
    }

uniform_test:

    // An exhaustive test to see if we have per-face assignment of UVs
    {
        uint32_t offset = 0;
        for (uint32_t i = 0, n = faceCounts.length(); i < n; ++i) {
            const uint32_t numVerts = faceCounts[i];
            const int32_t  index = indices[offset];

// extract for first vertex in face
#if defined(__AVX__)

            const d256 xyzw0 = loadu4d(xyzw + 4 * index);

#elif defined(__SSE__)

            const d128 xy0 = loadu2d(xyzw + 4 * index);
            const d128 zw0 = loadu2d(xyzw + 4 * index + 2);

#else

            const double x0 = xyzw[4 * index];
            const double y0 = xyzw[4 * index + 1];
            const double z0 = xyzw[4 * index + 2];
            const double w0 = xyzw[4 * index + 3];

#endif

            // process each face
            for (uint32_t j = 1; j < numVerts; ++j) {
                const int32_t next_index = indices[offset + j];

                // if the indices don't match
                if (index != next_index) {
#if defined(__AVX__)

                    const d256 xyzw1 = loadu4d(xyzw + 4 * next_index);
                    const d256 cmp = cmpne4d(xyzw0, xyzw1);
                    if (movemask4d(cmp))
                        goto face_varying;

#elif defined(__SSE__)

                    const d128 xy1 = loadu2d(xyzw + 4 * next_index);
                    const d128 zw1 = loadu2d(xyzw + 4 * next_index + 2);
                    const d128 cmp = or2d(cmpne2d(xy0, xy1), cmpne2d(zw0, zw1));
                    if (movemask2d(cmp))
                        goto face_varying;

#else

                    // check the values directly
                    const double x1 = xyzw[4 * next_index];
                    const double y1 = xyzw[4 * next_index + 1];
                    const double z1 = xyzw[4 * next_index + 2];
                    const double w1 = xyzw[4 * next_index + 3];
                    if (x0 != x1 || y0 != y1 || z0 != z1 || w0 != w1)
                        goto face_varying;

#endif
                }
            }
            offset += numVerts;
        }
        return UsdGeomTokens->uniform;
    }

face_varying:
    return UsdGeomTokens->faceVarying;
}

//----------------------------------------------------------------------------------------------------------------------
TfToken guessColourSetInterpolationType(const float* rgba, const size_t numElements)
{
    // if prim vars are all identical, we have a constant value
    if (MayaUsdUtils::vec4AreAllTheSame(rgba, numElements)) {
        return UsdGeomTokens->constant;
    }

    return UsdGeomTokens->faceVarying;
}

//----------------------------------------------------------------------------------------------------------------------
TfToken guessColourSetInterpolationTypeExtensive(
    const float*           rgba,
    const size_t           numElements,
    const size_t           numPoints,
    MIntArray&             pointIndices,
    MIntArray&             faceCounts,
    std::vector<uint32_t>& indicesToExtract)
{
    // if prim vars are all identical, we have a constant value
    if (MayaUsdUtils::vec4AreAllTheSame(rgba, numElements)) {
        return UsdGeomTokens->constant;
    }

    // check for per-vertex assignment
    std::vector<uint32_t> indicesMap;
    indicesMap.resize(numPoints, -1);

    {
        for (uint32_t i = 0, n = pointIndices.length(); i < n; ++i) {
            auto index = pointIndices[i];
            auto lastIndex = indicesMap[index];
            if (lastIndex == 0xFFFFFFFF) {
                indicesMap[index] = i;
            } else {
#if defined(__SSE__)

                const f128 rgba0 = loadu4f(rgba + 4 * lastIndex);
                const f128 rgba1 = loadu4f(rgba + 4 * i);
                const f128 cmp = cmpne4f(rgba0, rgba1);
                if (movemask4f(cmp))
                    goto uniform_test;

#else

                // if not, check to see if the indices differ, but the values are the same
                const float x0 = rgba[4 * lastIndex + 0];
                const float y0 = rgba[4 * lastIndex + 1];
                const float z0 = rgba[4 * lastIndex + 2];
                const float w0 = rgba[4 * lastIndex + 3];
                const float x1 = rgba[4 * i + 0];
                const float y1 = rgba[4 * i + 1];
                const float z1 = rgba[4 * i + 2];
                const float w1 = rgba[4 * i + 3];
                if (x0 != x1 || y0 != y1 || z0 != z1 || w0 != w1)
                    goto uniform_test;

#endif
            }
        }
        std::swap(indicesToExtract, indicesMap);
        return UsdGeomTokens->vertex;
    }

uniform_test:

    const uint32_t numFaces = faceCounts.length();
    indicesMap.resize(numFaces);
    uint32_t offset = 0;
    for (uint32_t i = 0; i < numFaces; ++i) {
        indicesMap[i] = offset;

        int numPointsInFace = faceCounts[i];
#if defined(__SSE__)

        const f128 rgba0 = loadu4f(rgba + 4 * offset);
        for (int32_t j = 1; j < numPointsInFace; ++j) {
            const f128 rgba1 = loadu4f(rgba + 4 * (offset + j));
            const f128 cmp = cmpne4f(rgba0, rgba1);
            if (movemask4f(cmp))
                goto faceVarying;
        }

#else

        const float* rgba0 = rgba + 4 * offset;
        for (int32_t j = 1; j < numPointsInFace; ++j) {
            const float* rgba1 = rgba + 4 * (offset + j);
            if (rgba0[0] != rgba1[0] || rgba0[1] != rgba1[1] || rgba0[2] != rgba1[2]
                || rgba0[3] != rgba1[3])
                goto faceVarying;
        }

#endif

        offset += numPointsInFace;
    }
    std::swap(indicesToExtract, indicesMap);
    return UsdGeomTokens->uniform;

faceVarying:
    return UsdGeomTokens->faceVarying;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace utils
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
