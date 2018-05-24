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
#include "AL/maya/utils/Utils.h"
#include "AL/usdmaya/utils/MeshUtils.h"
#include "AL/usdmaya/utils/DiffPrimVar.h"
#include "AL/usdmaya/utils/Utils.h"
#include "AL/usd/utils/DebugCodes.h"

#include "maya/MItMeshPolygon.h"
#include "maya/MGlobal.h"

namespace AL {
namespace usdmaya {
namespace utils {

enum GlimpseUserDataTypes
{
  kInt = 2,
  kFloat = 4,
  kInt2 = 7,
  kInt3 = 8,
  kVector = 13,
  kColor = 15,
  kString = 16,
  kMatrix = 17
};

//----------------------------------------------------------------------------------------------------------------------
void floatToDouble(double* output, const float* const input, size_t count)
{
  for(size_t i = 0; i < count; ++i)
  {
    output[i] = double(input[i]);
  }
}

//----------------------------------------------------------------------------------------------------------------------
void doubleToFloat(float* output, const double* const input, size_t count)
{
  for(size_t i = 0; i < count; ++i)
  {
    output[i] = float(input[i]);
  }
}

//----------------------------------------------------------------------------------------------------------------------
#if AL_UTILS_ENABLE_SIMD
#if defined(__AVX2__)
static void convert3Dto4d_avx(f256 a, f256 b, f256 c, float* const output)
{
  static const f256 wvalues = set8f(0, 0, 0, 1.0f, 0, 0, 0, 1.0f);
  static const f256 wmask = set8f(0, 0, 0, -0.0f, 0, 0, 0, -0.0f);
  const f256 v23 = permute2f128(a, b, 0x12);
  const f256 v45 = permute2f128(b, c, 0x12);
  static const i256 mask01 = set8i(0, 1, 2, 0, 3, 4, 5, 0);
  static const i256 mask23 = set8i(2, 3, 4, 0, 5, 6, 7, 0);
  f256 o01 = permutevar8x32f(a, mask01);
  f256 o23 = permutevar8x32f(v23, mask23);
  f256 o45 = permutevar8x32f(v45, mask01);
  f256 o67 = permutevar8x32f(c, mask23);
  o01 = select8f(wvalues, o01, wmask);
  o23 = select8f(wvalues, o01, wmask);
  o45 = select8f(wvalues, o01, wmask);
  o67 = select8f(wvalues, o01, wmask);
  storeu8f(output, o01);
  storeu8f(output + 8, o23);
  storeu8f(output + 16, o45);
  storeu8f(output + 24, o67);
}
#endif


//----------------------------------------------------------------------------------------------------------------------
/// \brief  assuming a, b, & c are 8 packed 3D vectors of the form:
///
///         { v0x, v0y, v0z, v1x, v1y, v1z,   *snip*, v7x, v7y, v7z }
///
///         This method will convert that to 4D vectors with a 'w' value of 1.
///
///         { v0x, v0y, v0z, 1.0, v1x, v1y, v1z, 1.0, *snip*, v7x, v7y, v7z, 1.0 }
///
///         The output array must contain 32 floating poing values
//----------------------------------------------------------------------------------------------------------------------
static void convert3Dto4d_sse(const f128 a, const f128 b, const f128 c, float* output)
{
  static const f128 wvalues = set4f(0, 0, 0, 1.0f);
  static const f128 wmask = cast4f(set4i(0, 0, 0, 0xFFFFFFFF));

  const f128 o0 = select4f(a, wvalues, wmask);
  const f128 o3 = or4f(cast4f(shiftBytesRight(cast4i(c), 4)), wvalues);
  f128 o1 = shuffle4f(a, b, 1, 0, 3, 3);
  o1 = select4f(shuffle4f(o1, o1, 1, 3, 2, 0), wvalues, wmask);
  f128 o2 = select4f(shuffle4f(b, c, 1, 0, 3, 2), wvalues, wmask);

  storeu4f(output, o0);
  storeu4f(output + 4, o1);
  storeu4f(output + 8, o2);
  storeu4f(output + 12, o3);
}

//----------------------------------------------------------------------------------------------------------------------
static void convert3Dto4d(const float* const c, float* const output, uint32_t count)
{
  switch(count)
  {
  case 3:
    output[8] = c[6];
    output[9] = c[7];
    output[10] = c[8];
    output[11] = 1.0f;
  case 2:
    output[4] = c[3];
    output[5] = c[4];
    output[6] = c[5];
    output[7] = 1.0f;
  case 1:
    output[0] = c[0];
    output[1] = c[1];
    output[2] = c[2];
    output[3] = 1.0f;
    default: break;
  }
}
#endif

//----------------------------------------------------------------------------------------------------------------------
void convert3DArrayTo4DArray(const float* const input, float* const output, size_t count)
{
#if AL_UTILS_ENABLE_SIMD
# if defined(__AVX2__) && ENABLE_SOME_AVX_ROUTINES
  size_t count8 = count >> 3;
  bool count4 = (count & 0x4) != 0;
  uint32_t remainder = count & 0x3;
  size_t i = 0, j = 0;
  for(size_t n = 24 * count8; i != n; i += 24, j += 32)
  {
    const float* const ptr = input + i;
    const f256 a = loadu8f(ptr);
    const f256 b = loadu8f(ptr + 8);
    const f256 c = loadu8f(ptr + 16);
    convert3Dto4d_avx(a, b, c, output + j);
  }
  if(count4)
  {
    const float* const ptr = input + i;
    const f128 a = loadu4f(ptr);
    const f128 b = loadu4f(ptr + 4);
    const f128 c = loadu4f(ptr + 8);
    convert3Dto4d_sse(a, b, c, output + j);
    i += 12;
    j += 16;
  }
  convert3Dto4d(input + i, output + j, remainder);
# elif defined(__SSE3__)
  const size_t count4 = count >> 2;
  const uint32_t remainder = count & 0x3;
  size_t i = 0, j = 0;
  for(size_t n = 12 * count4; i != n; i += 12, j += 16)
  {
    const float* const ptr = input + i;
    const f128 a = loadu4f(ptr);
    const f128 b = loadu4f(ptr + 4);
    const f128 c = loadu4f(ptr + 8);
    convert3Dto4d_sse(a, b, c, output + j);
  }
  convert3Dto4d(input + i, output + j, remainder);
# endif
#else
  for(size_t i = 0, j = 0, n = count * 3; i != n; i += 3, j += 4)
  {
    output[j ] = input[i ];
    output[j + 1] = input[i + 1];
    output[j + 2] = input[i + 2];
    output[j + 3] = 1.0f;
  }
#endif
}

//----------------------------------------------------------------------------------------------------------------------
void MeshImportContext::gatherFaceConnectsAndVertices()
{
  VtArray<GfVec3f> pointData;
  VtArray<GfVec3f> normalsData;
  VtArray<int> faceVertexCounts;
  VtArray<int> faceVertexIndices;

  UsdAttribute fvc = mesh.GetFaceVertexCountsAttr();
  UsdAttribute fvi = mesh.GetFaceVertexIndicesAttr();

  fvc.Get(&faceVertexCounts, m_timeCode);
  counts.setLength(faceVertexCounts.size());
  fvi.Get(&faceVertexIndices, m_timeCode);
  connects.setLength(faceVertexIndices.size());

  if(leftHanded)
  {
    int32_t* index = static_cast<int32_t*>(faceVertexIndices.data());
    int32_t* indexMax = index + faceVertexIndices.size();
    for(auto faceVertexCount: faceVertexCounts)
    {
      int32_t* start = index;
      int32_t* end = index + (faceVertexCount - 1);
      if(start < indexMax && end < indexMax)
      {
        std::reverse(start, end + 1);
        index += faceVertexCount;
      }
      else
      {
        faceVertexIndices.clear();
        fvi.Get(&faceVertexIndices, m_timeCode);
        break;
      }
    }
  }

  mesh.GetPointsAttr().Get(&pointData, m_timeCode);
  if(mesh.GetNormalsAttr().HasAuthoredValueOpinion())
  {
    mesh.GetNormalsAttr().Get(&normalsData, m_timeCode);
  }

  points.setLength(pointData.size());
  convert3DArrayTo4DArray((const float*)pointData.cdata(), &points[0].x, pointData.size());

  memcpy(&counts[0], (const int32_t*)faceVertexCounts.cdata(), sizeof(int32_t) * faceVertexCounts.size());
  memcpy(&connects[0], (const int32_t*)faceVertexIndices.cdata(), sizeof(int32_t) * faceVertexIndices.size());

  if(mesh.GetNormalsAttr().HasAuthoredValueOpinion())
  {
    if(mesh.GetNormalsInterpolation() == UsdGeomTokens->faceVarying ||
       mesh.GetNormalsInterpolation() == UsdGeomTokens->varying)
    {
      normals.setLength(normalsData.size());
      if(leftHanded && normalsData.size())
      {
        int32_t index = 0;
        double* const optr = &normals[0].x;
        const float* const iptr = (const float*)normalsData.cdata();
        for(auto faceVertexCount: faceVertexCounts)
        {
          for(int32_t i = index, j = index + faceVertexCount - 1; i < index + faceVertexCount; i++, j--)
          {
            optr[3 * j] = iptr[3 * i];
            optr[3 * j + 1] = iptr[3 * i + 1];
            optr[3 * j + 2] = iptr[3 * i + 2];
          }
          index += faceVertexCount;
        }
      }
      else
      {
        double* const optr = &normals[0].x;
        const float* const iptr = (const float*)normalsData.cdata();
        for(size_t i = 0, n = normalsData.size() * 3; i < n; i += 3)
        {
          optr[i + 0] = iptr[i + 0];
          optr[i + 1] = iptr[i + 1];
          optr[i + 2] = iptr[i + 2];
        }
      }
    }
    else
    if(mesh.GetNormalsInterpolation() == UsdGeomTokens->uniform)
    {
      const float* const iptr = (const float*)normalsData.cdata();
      normals.setLength(connects.length());
      for(uint32_t i = 0, k = 0, nf = counts.length(); i < nf; ++i)
      {
        uint32_t nv = counts[i];
        for(uint32_t j = 0; j < nv; ++j)
        {
          normals[k + j] = MVector(iptr[3 * i], iptr[3 * i + 1], iptr[3 * i + 2]);
        }
        k += nv;
      }
    }
    else
    if(mesh.GetNormalsInterpolation() == UsdGeomTokens->vertex)
    {
      const float* const iptr = (const float*)normalsData.cdata();
      normals.setLength(connects.length());
      for(uint32_t i = 0, nf = connects.length(); i < nf; ++i)
      {
        int index = connects[i];
        normals[i] = MVector(iptr[3 * index], iptr[3 * index + 1], iptr[3 * index + 2]);
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void convertFloatVec3ArrayToDoubleVec3Array(const float* const input, double* const output, size_t count)
{
#if AL_UTILS_ENABLE_SIMD
  const f128* const input128 = (const f128*)input;
  d128* const output128 = (d128*)output;
  const size_t count4 = count >> 2;
  for(size_t i = 0; i < count4; ++i)
  {
    const f128* const input = input128 + i * 3;
    d128* const output = output128 + i * 6;
    const f128 r0 = input[0];
    const f128 r1 = input[1];
    const f128 r2 = input[2];
    const d128 r1a = cvt2f_to_2d(cast4f(shiftBytesRight(cast4i(r0), 8)));
    const d128 r1b = cvt2f_to_2d(cast4f(shiftBytesRight(cast4i(r1), 8)));
    const d128 r1c = cvt2f_to_2d(cast4f(shiftBytesRight(cast4i(r2), 8)));
    const d128 r0a = cvt2f_to_2d(r0);
    const d128 r0b = cvt2f_to_2d(r1);
    const d128 r0c = cvt2f_to_2d(r2);
    output[0] = r0a;
    output[1] = r1a;
    output[2] = r0b;
    output[3] = r1b;
    output[4] = r0c;
    output[5] = r1c;
  }
  for(size_t i = count4 << 2; i < count; ++i)
  {
    output[i * 3] = float(input[i * 3]);
    output[i * 3 + 1] = float(input[i * 3 + 1]);
    output[i * 3 + 2] = float(input[i * 3 + 2]);
  }
#else
  for(size_t i = 0, n = 3 * count; i < n; i += 3)
  {
    output[i] = float(input[i]);
    output[i + 1] = float(input[i + 1]);
    output[i + 2] = float(input[i + 2]);
  }
#endif
}

//----------------------------------------------------------------------------------------------------------------------
void generateIncrementingIndices(MIntArray& indices, const size_t count)
{
#if AL_UTILS_ENABLE_SIMD
# if defined(__AVX2__) && ENABLE_SOME_AVX_ROUTINES
  const size_t padding = count & 0x7ULL;
  const size_t total = count + (padding ? (8 - padding) : 0);
  indices.setLength(total);

  int32_t* ptr = (int32_t*)&indices[0];
  i256 inc8 = set8i(0, 1, 2, 3, 4, 5, 6, 7);
  const i256 eight = set8i(8, 8, 8, 8,  8, 8, 8, 8);
  for(size_t i = 0; i < total; i += 8)
  {
    storeu8i(ptr, inc8);
    inc8 = add8i(inc8, eight);
  }
# else
  const size_t padding = count & 0x3ULL;
  const size_t total = count + (padding ? (4 - padding) : 0);
  indices.setLength(total);

  int32_t* ptr = (int32_t*)&indices[0];
  i128 inc4 = set4i(0, 1, 2, 3);
  const i128 four = set4i(4, 4, 4, 4);
  for(size_t i = 0; i < total; i += 4)
  {
    storeu4i(ptr + i, inc4);
    inc4 = add4i(inc4, four);
  }
# endif

  // trim a couple from the end of the array (I'm assuming maya isn't stupid enough to reallocate upon shrinking by a small amount)
  indices.setLength(count);

#else

  indices.setLength(count);

  for(size_t i = 0; i < count; ++i)
  {
    indices[i] = i;
  }

#endif
}

//----------------------------------------------------------------------------------------------------------------------
void MeshImportContext::applyHoleFaces()
{
  // Set Holes
  VtArray<int> holeIndices;
  mesh.GetHoleIndicesAttr().Get(&holeIndices, m_timeCode);
  if(holeIndices.size())
  {
    MUintArray mayaHoleIndices((const uint32_t*)holeIndices.cdata(), holeIndices.size());
    AL_MAYA_CHECK_ERROR2(fnMesh.setInvisibleFaces(mayaHoleIndices), "Unable to set invisible faces");
  }
}

//----------------------------------------------------------------------------------------------------------------------
void unzipUVs(const float* const uv, float* const u, float* const v, const size_t count)
{
#if AL_UTILS_ENABLE_SIMD

#ifdef __AVX2__
  const size_t count8 = count & ~7ULL;
  size_t i = 0, j = 0;
  for(; i < count8; i += 8, j += 16)
  {
    const f256 uva = loadu8f(uv + j);
    const f256 uvb = loadu8f(uv + j + 8);
    const f256 uva1 = permute2f128(uva, uvb, 0x20);
    const f256 uvb1 = permute2f128(uva, uvb, 0x31);
    const f256 uvals = shuffle8f(uva1, uvb1, 2, 0, 2, 0);
    const f256 vvals = shuffle8f(uva1, uvb1, 3, 1, 3, 1);
    storeu8f(u + i, uvals);
    storeu8f(v + i, vvals);
  }

  if(count & 0x4)
  {
    const f128 uva = loadu4f(uv + j);
    const f128 uvb = loadu4f(uv + j + 4);
    const f128 uvals = shuffle4f(uva, uvb, 2, 0, 2, 0);
    const f128 vvals = shuffle4f(uva, uvb, 3, 1, 3, 1);
    storeu4f(u + i, uvals);
    storeu4f(v + i, vvals);
    i += 4;
    j += 8;
  }
#else

  const size_t count4 = count & ~3ULL;
  size_t i = 0, j = 0;
  for(; i < count4; i += 4, j += 8)
  {
    const f128 uva = loadu4f(uv + j);
    const f128 uvb = loadu4f(uv + j + 4);
    const f128 uvals = shuffle4f(uva, uvb, 2, 0, 2, 0);
    const f128 vvals = shuffle4f(uva, uvb, 3, 1, 3, 1);
    storeu4f(u + i, uvals);
    storeu4f(v + i, vvals);
  }

#endif

  switch(count & 3)
  {
  case 3:
    u[i + 2] = uv[j + 4];
    v[i + 2] = uv[j + 5];
  case 2:
    u[i + 1] = uv[j + 2];
    v[i + 1] = uv[j + 3];
  case 1:
    u[i] = uv[j];
    v[i] = uv[j + 1];
  default:
    break;
  }

#else
  for(size_t i = 0, j = 0; i < count; ++i, j += 2)
  {
    u[i] = uv[j];
    v[i] = uv[j + 1];
  }
#endif
}

//----------------------------------------------------------------------------------------------------------------------
bool MeshImportContext::applyVertexNormals()
{
  if(normals.length())
  {
    MIntArray normalsFaceIds;
    normalsFaceIds.setLength(connects.length());
    int32_t* normalsFaceIdsPtr = &normalsFaceIds[0];
    if (normals.length() == fnMesh.numFaceVertices())
    {
      for (uint32_t i = 0, k = 0, n = counts.length(); i < n; i++)
      {
        for (uint32_t j = 0, m = counts[i]; j < m; j++, ++k)
        {
          normalsFaceIdsPtr[k] = i;
        }
      }
    }

    return fnMesh.setFaceVertexNormals(normals, normalsFaceIds, connects, MSpace::kObject) == MS::kSuccess;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool MeshImportContext::applyVertexCreases()
{
  UsdAttribute cornerIndices = mesh.GetCornerIndicesAttr();
  UsdAttribute cornerSharpness = mesh.GetCornerSharpnessesAttr();
  if(cornerIndices.IsAuthored() && cornerIndices.HasValue() &&
     cornerSharpness.IsAuthored() && cornerSharpness.HasValue())
  {
    VtArray<int32_t> vertexIdValues;
    VtArray<float> creaseValues;
    cornerIndices.Get(&vertexIdValues, m_timeCode);
    cornerSharpness.Get(&creaseValues, m_timeCode);

    MUintArray vertexIds((const uint32_t*)vertexIdValues.cdata(), vertexIdValues.size());
    MDoubleArray creaseData;
    creaseData.setLength(creaseValues.size());
    floatToDouble(&creaseData[0], (const float*)creaseValues.cdata(), creaseValues.size());
    if(!fnMesh.setCreaseVertices(vertexIds, creaseData))
    {
      std::cerr << "Unable to set crease vertices on mesh " << fnMesh.name().asChar() << std::endl;
    }
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool MeshImportContext::applyEdgeCreases()
{
  UsdAttribute creaseIndices = mesh.GetCreaseIndicesAttr();
  UsdAttribute creaseLengths = mesh.GetCreaseLengthsAttr();
  UsdAttribute creaseSharpness = mesh.GetCreaseSharpnessesAttr();

  if(creaseIndices.IsAuthored() && creaseIndices.HasValue() &&
     creaseLengths.IsAuthored() && creaseLengths.HasValue() &&
     creaseSharpness.IsAuthored() && creaseSharpness.HasValue())
  {
    VtArray<int32_t> indices, lengths;
    VtArray<float> sharpness;

    creaseIndices.Get(&indices, m_timeCode);
    creaseLengths.Get(&lengths, m_timeCode);
    creaseSharpness.Get(&sharpness, m_timeCode);

    // expand data into vertex pair + single sharpness value
    MUintArray edgesIdValues;
    MDoubleArray creaseValues;
    for(uint32_t i = 0, k = 0; i < lengths.size(); ++i)
    {
      const int32_t len = lengths[i];
      if(!len)
        continue;

      int32_t firstVertex = indices[k++];
      for(int32_t j = 1; j < len; ++j)
      {
        int32_t nextVertex = indices[k++];
        edgesIdValues.append(firstVertex);
        edgesIdValues.append(nextVertex);
        firstVertex = nextVertex;
        creaseValues.append(sharpness[i]);
      }
    }

    //
    MObject temp = fnMesh.object();
    MItMeshVertex iter(temp);
    MIntArray edgeIds;
    MUintArray creaseEdgeIds;
    for(size_t i = 0, k = 0; i < edgesIdValues.length(); i += 2, ++k)
    {
      const int32_t vertexIndex0 = edgesIdValues[i];
      const int32_t vertexIndex1 = edgesIdValues[i + 1];
      int prev;
      if(!iter.setIndex(vertexIndex0, prev))
      {
        std::cout << "could not set index on vertex iterator" << std::endl;
      }

      if(iter.getConnectedEdges(edgeIds))
      {
        bool found = false;
        for(uint32_t j = 0; j < edgeIds.length(); ++j)
        {
          int2 edgeVerts;
          fnMesh.getEdgeVertices(edgeIds[j], edgeVerts);

          if((vertexIndex0 == edgeVerts[0] && vertexIndex1 == edgeVerts[1]) ||
             (vertexIndex1 == edgeVerts[0] && vertexIndex0 == edgeVerts[1]))
          {
            found = true;
            creaseEdgeIds.append(edgeIds[j]);
            break;
          }
        }
        if(!found)
        {
          std::cout << "could not find matching edge" << std::endl;
        }
      }
      else
      {
        std::cout << "could not access connected edges" << std::endl;
      }
    }

    if(!fnMesh.setCreaseEdges(creaseEdgeIds, creaseValues))
    {
      std::cerr << "Unable to set crease edges on mesh " << fnMesh.name().asChar() << std::endl;
    }
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
void MeshImportContext::applyGlimpseSubdivParams()
{
  // TODO: ideally, this should be coming from the ALGlimpseMeshAPI
  // and not be setting the attribute names directly
  static const TfToken glimpse_gSubdiv("glimpse:subdiv:enabled");
  static const TfToken glimpse_gSubdivKeepUvBoundary("glimpse:subdiv:keepUvBoundary");
  static const TfToken glimpse_gSubdivLevel("glimpse:subdiv:level");
  static const TfToken glimpse_gSubdivMode("glimpse:subdiv:mode");
  static const TfToken glimpse_gSubdivPrimSizeMult("glimpse:subdiv:primSizeMult");
  static const TfToken glimpse_gSubdivEdgeLengthMultiplier("glimpse:subdiv:edgeLengthMultiplier");

  static const TfToken primvar_gSubdiv("isSubdiv");
  static const TfToken primvar_gSubdivLevel("subdivLevel");

  UsdPrim from = mesh.GetPrim();
  UsdAttribute glimpse_gSubdiv_attr = from.GetAttribute(glimpse_gSubdiv);
  UsdAttribute glimpse_gSubdivKeepUvBoundary_attr = from.GetAttribute(glimpse_gSubdivKeepUvBoundary);
  UsdAttribute glimpse_gSubdivLevel_attr = from.GetAttribute(glimpse_gSubdivLevel);
  UsdAttribute glimpse_gSubdivMode_attr = from.GetAttribute(glimpse_gSubdivMode);
  UsdAttribute glimpse_gSubdivPrimSizeMult_attr = from.GetAttribute(glimpse_gSubdivPrimSizeMult);
  UsdAttribute glimpse_gSubdivEdgeLengthMultiplier_attr = from.GetAttribute(glimpse_gSubdivEdgeLengthMultiplier);

  // if the mesh is coming from alembic the glimpse subdivision
  // attributes are stored as primvars
  if (!glimpse_gSubdiv_attr && mesh.HasPrimvar(primvar_gSubdiv))
  {
    glimpse_gSubdiv_attr = mesh.GetPrimvar(primvar_gSubdiv);
  }

  if (!glimpse_gSubdivLevel_attr && mesh.HasPrimvar(primvar_gSubdivLevel))
  {
    glimpse_gSubdivLevel_attr = mesh.GetPrimvar(primvar_gSubdivLevel);
  }

  MStatus status;
  if(glimpse_gSubdiv_attr)
  {
    MPlug plug = fnMesh.findPlug("gSubdiv", true, &status);
    if(status)
    {
      bool value;
      glimpse_gSubdiv_attr.Get(&value, m_timeCode);
      plug.setBool(value);
    }
  }

  if(glimpse_gSubdivKeepUvBoundary_attr)
  {
    MPlug plug = fnMesh.findPlug("gSubdivKeepUvBoundary", true, &status);
    if(status)
    {
      bool value;
      glimpse_gSubdivKeepUvBoundary_attr.Get(&value, m_timeCode);
      plug.setBool(value);
    }
  }

  if(glimpse_gSubdivLevel_attr)
  {
    MPlug plug = fnMesh.findPlug("gSubdivLevel", true, &status);
    if(status)
    {
      int32_t value;
      glimpse_gSubdivLevel_attr.Get(&value, m_timeCode);
      plug.setInt(value);
    }
  }

  if(glimpse_gSubdivMode_attr)
  {
    MPlug plug = fnMesh.findPlug("gSubdivMode", true, &status);
    if(status)
    {
      int32_t value;
      glimpse_gSubdivMode_attr.Get(&value, m_timeCode);
      plug.setInt(value);
    }
  }

  if(glimpse_gSubdivPrimSizeMult_attr)
  {
    MPlug plug = fnMesh.findPlug("gSubdivPrimSizeMult", true, &status);
    if(status)
    {
      float value;
      glimpse_gSubdivPrimSizeMult_attr.Get(&value, m_timeCode);
      plug.setFloat(value);
    }
  }

  if(glimpse_gSubdivEdgeLengthMultiplier_attr)
  {
    MPlug plug = fnMesh.findPlug("gSubdivEdgeLengthMultiplier", true, &status);
    if(status)
    {
      float value;
      glimpse_gSubdivEdgeLengthMultiplier_attr.Get(&value, m_timeCode);
      plug.setFloat(value);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MeshImportContext::applyGlimpseUserDataParams()
{
  // TODO: glimpse user data can be set on any DAG node, push up to DagNodeTranslator?
  // TODO: a schema, similar to that of primvars, should be used
  static const std::string glimpse_namespace("glimpse:userData");

  MStatus status;
  MPlug plug = fnMesh.findPlug("gUserData", true, &status);
  if(!status)
  {
    return;
  }

  auto applyUserData = [](MPlug& elemPlug, const std::string& name, int type, const std::string& value)
  {
    MPlug namePlug = elemPlug.child(0);
    MPlug typePlug = elemPlug.child(1);
    MPlug valuePlug = elemPlug.child(2);

    namePlug.setValue(AL::maya::utils::convert(name));
    typePlug.setValue(type);
    valuePlug.setValue(AL::maya::utils::convert(value));
  };

  unsigned int logicalIndex = 0;
  std::vector<UsdProperty> userDataProperties = mesh.GetPrim().GetPropertiesInNamespace(glimpse_namespace);
  for(auto &userDataProperty : userDataProperties)
  {
    if (UsdAttribute attr = userDataProperty.As<UsdAttribute>())
    {
      SdfValueTypeName typeName = attr.GetTypeName();
      if(typeName == SdfValueTypeNames->Int)
      {
        int value;
        attr.Get(&value, m_timeCode);

        MPlug elementPlug = plug.elementByLogicalIndex(logicalIndex++);
        applyUserData(elementPlug, attr.GetBaseName().GetString(), GlimpseUserDataTypes::kInt, std::to_string(value));
      }
      else if(typeName == SdfValueTypeNames->Int2)
      {
        GfVec2i vec;
        attr.Get(&vec, m_timeCode);

        std::stringstream ss;
        ss << vec[0] << ' ' << vec[1];

        MPlug elementPlug = plug.elementByLogicalIndex(logicalIndex++);
        applyUserData(elementPlug, attr.GetBaseName().GetString(), GlimpseUserDataTypes::kInt2, ss.str());
      }
      else if(typeName == SdfValueTypeNames->Int3)
      {
        GfVec3i vec;
        attr.Get(&vec, m_timeCode);

        std::stringstream ss;
        ss << vec[0] << ' ' << vec[1] << ' ' << vec[2];

        MPlug elementPlug = plug.elementByLogicalIndex(logicalIndex++);
        applyUserData(elementPlug, attr.GetBaseName().GetString(), GlimpseUserDataTypes::kInt3, ss.str());
      }
      else if(typeName == SdfValueTypeNames->Float)
      {
        float value;
        attr.Get(&value, m_timeCode);

        MPlug elementPlug = plug.elementByLogicalIndex(logicalIndex++);
        applyUserData(elementPlug, attr.GetBaseName().GetString(), GlimpseUserDataTypes::kFloat,
                      std::to_string(value));
      }
      else if(typeName == SdfValueTypeNames->String)
      {
        std::string value;
        attr.Get(&value, m_timeCode);

        MPlug elementPlug = plug.elementByLogicalIndex(logicalIndex++);
        applyUserData(elementPlug, attr.GetBaseName().GetString(), GlimpseUserDataTypes::kString, value);
      }
      else if(typeName == SdfValueTypeNames->Vector3f)
      {
        GfVec3f vec;
        attr.Get(&vec, m_timeCode);

        std::stringstream ss;
        ss << vec[0] << ' ' << vec[1] << ' ' << vec[2];

        MPlug elementPlug = plug.elementByLogicalIndex(logicalIndex++);
        applyUserData(elementPlug, attr.GetBaseName().GetString(), GlimpseUserDataTypes::kVector, ss.str());
      }
      else if(typeName == SdfValueTypeNames->Color3f)
      {
        GfVec3f vec;
        attr.Get(&vec, m_timeCode);

        std::stringstream ss;
        ss << vec[0] << ' ' << vec[1] << ' ' << vec[2];

        MPlug elementPlug = plug.elementByLogicalIndex(logicalIndex++);
        applyUserData(elementPlug, attr.GetBaseName().GetString(), GlimpseUserDataTypes::kColor, ss.str());
      }
      else if(typeName == SdfValueTypeNames->Matrix4d)
      {
        GfMatrix4d matrix;
        attr.Get(&matrix, m_timeCode);

        std::stringstream ss;
        ss << matrix[0][0] << ' ' << matrix[0][1] << ' ' << matrix[0][2] << ' ';
        ss << matrix[1][0] << ' ' << matrix[1][1] << ' ' << matrix[1][2] << ' ';
        ss << matrix[2][0] << ' ' << matrix[2][1] << ' ' << matrix[2][2] << ' ';
        ss << matrix[3][0] << ' ' << matrix[3][1] << ' ' << matrix[3][2];

        MPlug elementPlug = plug.elementByLogicalIndex(logicalIndex++);
        applyUserData(elementPlug, attr.GetBaseName().GetString(), GlimpseUserDataTypes::kMatrix, ss.str());
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MeshImportContext::applyPrimVars(bool createUvs, bool createColours)
{
  MIntArray mayaIndices;
  MFloatArray u, v;
  MColorArray colours;
  const std::vector<UsdGeomPrimvar> primvars = mesh.GetPrimvars();
  for(auto it = primvars.begin(), end = primvars.end(); it != end; ++it)
  {
    const UsdGeomPrimvar& primvar = *it;
    TfToken name, interpolation;
    SdfValueTypeName typeName;
    int elementSize;
    primvar.GetDeclarationInfo(&name, &typeName, &interpolation, &elementSize);
    VtValue vtValue;

    if (primvar.Get(&vtValue, m_timeCode))
    {
      if (vtValue.IsHolding<VtArray<GfVec2f> >())
      {
        if(!createUvs)
          continue;
        const VtArray<GfVec2f> rawVal = vtValue.Get<VtArray<GfVec2f> >();
        u.setLength(rawVal.size());
        v.setLength(rawVal.size());
        unzipUVs((const float*)rawVal.cdata(), &u[0], &v[0], rawVal.size());

        MString uvSetName = AL::usdmaya::utils::convert(name);
        MString* uv_set = &uvSetName;
        if (uvSetName == "st")
        {
          uvSetName = "map1";
          uv_set = 0;
        }

        if(uv_set)
        {
          uvSetName = fnMesh.createUVSetWithName(uvSetName);
        }

        if(primvar.IsIndexed())
        {
          if (interpolation == UsdGeomTokens->faceVarying)
          {
            MStatus s = fnMesh.setUVs(u, v, uv_set);
            if(s)
            {
              VtIntArray usdindices;
              primvar.GetIndices(&usdindices);
              mayaIndices.setLength(usdindices.size());
              std::memcpy(&mayaIndices[0], usdindices.cdata(), sizeof(int) * usdindices.size());
              s = fnMesh.assignUVs(counts, mayaIndices, uv_set);
              if(!s)
              {
                TF_DEBUG(ALUTILS_INFO).Msg("Failed to assign UVS for uvset \"%s\" on mesh \"%s\", error: %s\n",
                    uvSetName.asChar(), fnMesh.name().asChar(), s.errorString().asChar());
              }
            }
            else
            {
              TF_DEBUG(ALUTILS_INFO).Msg("Failed to set UVS for uvset \"%s\" on mesh \"%s\", error: %s\n",
                  uvSetName.asChar(), fnMesh.name().asChar(), s.errorString().asChar());
            }
          }
        }
        else
        {
          if(fnMesh.setUVs(u, v, uv_set))
          {
            if (interpolation == UsdGeomTokens->faceVarying)
            {
              generateIncrementingIndices(mayaIndices, rawVal.size());
              MStatus s = fnMesh.assignUVs(counts, mayaIndices, uv_set);
              if(!s)
              {
                TF_DEBUG(ALUTILS_INFO).Msg("Failed to assign UVS for uvset \"%s\" on mesh \"%s\", error: %s\n",
                    uvSetName.asChar(), fnMesh.name().asChar(), s.errorString().asChar());
              }
            }
            else
            if (interpolation == UsdGeomTokens->vertex)
            {
              MStatus s = fnMesh.assignUVs(counts, connects, uv_set);
              if(!s)
              {
                TF_DEBUG(ALUTILS_INFO).Msg("Failed to assign UVS for uvset \"%s\" on mesh \"%s\", error: %s\n",
                    uvSetName.asChar(), fnMesh.name().asChar(), s.errorString().asChar());
              }
            }
            else
            if (interpolation == UsdGeomTokens->uniform)
            {
              mayaIndices.setLength(connects.length());
              for(uint32_t i = 0, j = 0; i < counts.length(); ++i)
              {
                for(int k = 0; k < counts[i]; ++k)
                {
                  mayaIndices[j++] = i;
                }
              }
              MStatus s = fnMesh.assignUVs(counts, mayaIndices, uv_set);
              if(!s)
              {
                TF_DEBUG(ALUTILS_INFO).Msg("Failed to assign UVS for uvset \"%s\" on mesh \"%s\", error: %s\n",
                    uvSetName.asChar(), fnMesh.name().asChar(), s.errorString().asChar());
              }
            }
            else
            if (interpolation == UsdGeomTokens->constant)
            {
              // should all be zero, since there is only 1 UV in the set
              mayaIndices.setLength(connects.length());
              std::memset(&mayaIndices[0], 0, sizeof(int) * mayaIndices.length());
              MStatus s = fnMesh.assignUVs(counts, mayaIndices, uv_set);
              if(!s)
              {
                TF_DEBUG(ALUTILS_INFO).Msg("Failed to assign UVS for uvset \"%s\" on mesh \"%s\", error: %s\n",
                    uvSetName.asChar(), fnMesh.name().asChar(), s.errorString().asChar());
              }
            }
          }
        }
      }
      else
      if (vtValue.IsHolding<VtArray<GfVec4f> >())
      {
        if(!createColours)
          continue;

        MString colourSetName(name.GetText());
        fnMesh.setDisplayColors(true);

        MStatus s;
        #if MAYA_API_VERSION >= 201800
        colourSetName = fnMesh.createColorSetWithName(colourSetName, nullptr, nullptr, &s);
        #else
        colourSetName = fnMesh.createColorSetWithName(colourSetName, nullptr, &s);
        #endif
        if(s)
        {
          s = fnMesh.setCurrentColorSetName(colourSetName);
          if(s)
          {
            const VtArray<GfVec4f> rawVal = vtValue.Get<VtArray<GfVec4f> >();
            colours.setLength(rawVal.size());
            memcpy(&colours[0], (const float*)rawVal.cdata(), sizeof(float) * 4 * rawVal.size());

            if (interpolation == UsdGeomTokens->faceVarying)
            {
              s = fnMesh.setColors(colours, &colourSetName);
              if(s)
              {
                if(primvar.IsIndexed())
                {
                  VtIntArray usdindices;
                  primvar.GetIndices(&usdindices);
                  mayaIndices.setLength(usdindices.size());
                  std::memcpy(&mayaIndices[0], usdindices.cdata(), sizeof(int) * usdindices.size());

                  s = fnMesh.assignColors(mayaIndices, &colourSetName);
                  if(!s)
                  {
                    TF_DEBUG(ALUTILS_INFO).Msg("Failed to set colour indices for colour set \"%s\" on mesh \"%s\", error: %s\n",
                        colourSetName.asChar(), fnMesh.name().asChar(), s.errorString().asChar());
                  }
                }
              }
              else
              {
                TF_DEBUG(ALUTILS_INFO).Msg("Failed to set colours for colour set \"%s\" on mesh \"%s\", error: %s\n",
                    colourSetName.asChar(), fnMesh.name().asChar(), s.errorString().asChar());
              }
            }
            else
            if (interpolation == UsdGeomTokens->uniform)
            {
              if(primvar.IsIndexed())
              {
                VtIntArray usdindices;
                primvar.GetIndices(&usdindices);
                mayaIndices.setLength(usdindices.size());
                std::memcpy(&mayaIndices[0], usdindices.cdata(), sizeof(int) * usdindices.size());
              }
              else
              {
                generateIncrementingIndices(mayaIndices, rawVal.size());
              }

              s = fnMesh.setFaceColors(colours, mayaIndices, MFnMesh::kRGBA);
              if(!s)
              {
                TF_DEBUG(ALUTILS_INFO).Msg("Failed to set colours for colour set \"%s\" on mesh \"%s\", error: %s\n",
                    colourSetName.asChar(), fnMesh.name().asChar(), s.errorString().asChar());
              }
            }
            else
            if (interpolation == UsdGeomTokens->vertex)
            {
              MColorArray temp;
              temp.setLength(fnMesh.numFaceVertices());
              if(primvar.IsIndexed())
              {
                const MColor* pcolours = (const MColor*)rawVal.cdata();
                VtIntArray usdindices;
                primvar.GetIndices(&usdindices);
                for(uint32_t i = 0, n = connects.length(); i < n; ++i)
                {
                  temp[i] = pcolours[usdindices[connects[i]]];
                }
              }
              else
              {
                const MColor* pcolours = (const MColor*)rawVal.cdata();
                for(uint32_t i = 0, n = connects.length(); i < n; ++i)
                {
                  temp[i] = pcolours[connects[i]];
                }
              }
              s = fnMesh.setColors(temp, &colourSetName);
              if(!s)
              {
                TF_DEBUG(ALUTILS_INFO).Msg("Failed to set colours for colour set \"%s\" on mesh \"%s\", error: %s\n",
                    colourSetName.asChar(), fnMesh.name().asChar(), s.errorString().asChar());
              }
            }
            else
            if (interpolation == UsdGeomTokens->constant)
            {
              colours.setLength(fnMesh.numFaceVertices());
              for(uint32_t i = 1; i < colours.length(); ++i)
              {
                colours[i] = colours[0];
              }
              s = fnMesh.setColors(colours, &colourSetName);
              if(!s)
              {
                TF_DEBUG(ALUTILS_INFO).Msg("Failed to set colours for colour set \"%s\" on mesh \"%s\", error: %s\n",
                    colourSetName.asChar(), fnMesh.name().asChar(), s.errorString().asChar());
              }
            }
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
MeshExportContext::MeshExportContext(
    MDagPath path,
    UsdGeomMesh& mesh,
    UsdTimeCode timeCode,
    bool performDiff,
    CompactionLevel compactionLevel)
  : fnMesh(), faceCounts(), faceConnects(), m_timeCode(timeCode), mesh(mesh), compaction(compactionLevel), performDiff(performDiff)
{
  MStatus status = fnMesh.setObject(path);
  valid = (status == MS::kSuccess);
  AL_MAYA_CHECK_ERROR2(status, MString("unable to attach function set to mesh") + path.fullPathName());
  if(status)
  {
    fnMesh.getVertices(faceCounts, faceConnects);
  }

  if(performDiff)
  {
    diffGeom = utils::diffGeom(mesh, fnMesh, m_timeCode, kAllComponents);
    diffMesh = diffFaceVertices(mesh, fnMesh, m_timeCode, kAllComponents);
  }
  else
  {
    diffGeom = kAllComponents;
    diffMesh = kAllComponents;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MeshExportContext::copyFaceConnectsAndPolyCounts()
{
  if((diffMesh & kFaceVertexCounts) && faceCounts.length())
  {
    VtArray<int32_t> faceVertexCounts(faceCounts.length());
    memcpy((int32_t*)faceVertexCounts.data(), &faceCounts[0], sizeof(uint32_t) * faceCounts.length());
    if(UsdAttribute vertextCounts = mesh.GetFaceVertexCountsAttr())
    {
      vertextCounts.Set(faceVertexCounts);
    }
  }

  if((diffMesh & kFaceVertexIndices) && faceConnects.length())
  {
    VtArray<int32_t> faceVertexIndices(faceConnects.length());
    memcpy((int32_t*)faceVertexIndices.data(), &faceConnects[0], sizeof(uint32_t) * faceConnects.length());
    if(UsdAttribute faceVertexIndicies = mesh.GetFaceVertexIndicesAttr())
    {
      faceVertexIndicies.Set(faceVertexIndices);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Checks to see if any elements within the UV counts array happen to be zero.
//----------------------------------------------------------------------------------------------------------------------
bool isUvSetDataSparse(const int32_t* uvCounts, const uint32_t count)
{
#if AL_UTILS_ENABLE_SIMD
# if defined(__AVX2__) && ENABLE_SOME_AVX_ROUTINES
  const i256* counts = (const __m256i*)&uvCounts[0];
  const i256 zero = zero8i();
  const uint32_t count8 = count >> 3;

  for(uint32_t i = 0; i < count8; ++i)
  {
    if(movemask8i(cmpeq8i(zero, loadu8i(counts + i))))
      return true;
  }

  for(uint32_t i = count8 << 3; i < count; ++i)
  {
    if(!uvCounts[i]) return true;
  }
# else
  const i128* counts = (const i128*)(&uvCounts[0]);
  const i128 zero = zero4i();
  const uint32_t count4 = count >> 2;

  for(uint32_t i = 0; i < count4; ++i)
  {
    if(movemask4i(cmpeq4i(zero, loadu4i(counts + i))))
      return true;
  }

  for(uint32_t i = count4 << 2; i < count; ++i)
  {
    if(!uvCounts[i]) return true;
  }
# endif
#else
  for(uint32_t i = 0; i < count; ++i)
  {
    if(!uvCounts[i])
      return true;
  }
#endif
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
void zipUVs(const float* u, const float* v, float* uv, const size_t count)
{
#if AL_UTILS_ENABLE_SIMD
# ifdef __AVX2__

  uint32_t uvCount8 = count & ~7U;

  for(uint32_t i = 0; i < uvCount8; i += 8, uv += 16)
  {
    const f256 U = loadu8f(u + i);
    const f256 V = loadu8f(v + i);
    const f256 uv0 = unpacklo8f(U, V);
    const f256 uv1 = unpackhi8f(U, V);
    storeu8f(uv, permute2f128(uv0, uv1, 0x20));
    storeu8f(uv + 8, permute2f128(uv0, uv1, 0x31));
  }

  if(count & 0x4)
  {
    const f128 U = loadu4f(u + uvCount8);
    const f128 V = loadu4f(v + uvCount8);
    storeu4f(uv, unpacklo4f(U, V));
    storeu4f(uv + 4, unpackhi4f(U, V));
    uv += 8;
    uvCount8 += 4;
  }

  switch(count & 3)
  {
  case 3:
    uv[4] = u[uvCount8 + 2];
    uv[5] = v[uvCount8 + 2];
  case 2:
    uv[2] = u[uvCount8 + 1];
    uv[3] = v[uvCount8 + 1];
  case 1:
    uv[0] = u[uvCount8 + 0];
    uv[1] = v[uvCount8 + 0];
  default:
    break;
  }

# else

  const uint32_t uvCount4 = count & ~3U;

  for(uint32_t i = 0; i < uvCount4; i += 4, uv += 8)
  {
    const f128 U = loadu4f(u + i);
    const f128 V = loadu4f(v + i);
    storeu4f(uv, unpacklo4f(U, V));
    storeu4f(uv + 4, unpackhi4f(U, V));
  }

  switch(count & 3)
  {
  case 3:
    uv[4] = u[uvCount4 + 2];
    uv[5] = v[uvCount4 + 2];
  case 2:
    uv[2] = u[uvCount4 + 1];
    uv[3] = v[uvCount4 + 1];
  case 1:
    uv[0] = u[uvCount4 + 0];
    uv[1] = v[uvCount4 + 0];
  default:
    break;
  }

# endif
#else
  for(uint32_t i = 0, j = 0; i < count; i++, j += 2)
  {
    uv[j] = u[i];
    uv[j + 1] = v[i];
  }
#endif
}

//----------------------------------------------------------------------------------------------------------------------
static void reverseIndices(VtArray<int32_t>& indices, const MIntArray& counts)
{
  auto iter = indices.begin();
  for (uint32_t i = 0, len = counts.length(); i < len; ++i)
  {
    int32_t cnt = counts[i];
    std::reverse(iter, iter + cnt);
    iter += cnt;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MeshExportContext::copyUvSetData(bool leftHanded)
{
  UsdPrim prim = mesh.GetPrim();
  MStringArray uvSetNames;
  usdmaya::utils::PrimVarDiffReport diff_report;
  if(performDiff)
  {
    uvSetNames = usdmaya::utils::hasNewUvSet(mesh, fnMesh, diff_report);
    if(diff_report.empty() && uvSetNames.length() == 0)
      return;
  }
  else
  {
    MStatus status = fnMesh.getUVSetNames(uvSetNames);
    if(!status || uvSetNames.length() == 0)
      return;
  }

  VtArray<GfVec2f> uvValues;
  MFloatArray uValues, vValues;
  MIntArray uvCounts, uvIds;
  std::vector<uint32_t> indicesToExtract;

  for (uint32_t i = 0; i < uvSetNames.length(); i++)
  {
    TfToken interpolation = UsdGeomTokens->faceVarying;

    // Initialize the VtArray to the max possible size (facevarying)
    if(fnMesh.getAssignedUVs(uvCounts, uvIds, &uvSetNames[i]))
    {
      int32_t* ptr = &uvCounts[0];
      if(!isUvSetDataSparse(ptr, uvCounts.length()))
      {
        if(fnMesh.getUVs(uValues, vValues, &uvSetNames[i]))
        {
          indicesToExtract.clear();
          switch(compaction)
          {
          case kNone:
            break;
          case kBasic:
            interpolation = guessUVInterpolationType(uValues, vValues, uvIds, faceConnects);
            break;
          case kMedium:
            interpolation = guessUVInterpolationTypeExtended(uValues, vValues, uvIds, faceConnects, uvCounts);
            break;
          case kFull:
            interpolation = guessUVInterpolationTypeExtensive(uValues, vValues, uvIds, faceConnects, uvCounts, indicesToExtract);
            break;
          }

          if(interpolation == UsdGeomTokens->constant)
          {
            uvValues.resize(1);
            fnMesh.getUV(0, uvValues[0][0], uvValues[0][1], &uvSetNames[i]);
            if (uvSetNames[i] == "map1")
            {
              uvSetNames[i] = "st";
            }
            UsdGeomPrimvar uvSet = mesh.CreatePrimvar(TfToken(uvSetNames[i].asChar()), SdfValueTypeNames->Float2Array, UsdGeomTokens->constant);
            uvSet.Set(uvValues, m_timeCode);
          }
          else
          if(interpolation == UsdGeomTokens->vertex)
          {
            if(uValues.length())
            {
              const uint32_t npoints = fnMesh.numVertices();
              uvValues.resize(npoints);

              float* uptr = &uValues[0];
              float* vptr = &vValues[0];
              float* uvptr = (float*)uvValues.data();
              if(indicesToExtract.empty())
              {
                zipUVs(uptr, vptr, uvptr, uValues.length());
              }
              else
              {
                for(uint32_t j = 0; j < indicesToExtract.size(); ++j)
                {
                  uint32_t index = indicesToExtract[j];
                  uvptr[j * 2 + 0] = uptr[index];
                  uvptr[j * 2 + 1] = vptr[index];
                }
              }
              if (uvSetNames[i] == "map1")
              {
                uvSetNames[i] = "st";
              }
              UsdGeomPrimvar uvSet = mesh.CreatePrimvar(TfToken(uvSetNames[i].asChar()), SdfValueTypeNames->Float2Array, UsdGeomTokens->vertex);
              uvSet.Set(uvValues, m_timeCode);
            }
          }
          else
          if(interpolation == UsdGeomTokens->uniform)
          {
            const uint32_t nfaces = fnMesh.numPolygons();
            uvValues.resize(nfaces);
            for(uint32_t j = 0; j < nfaces; ++j)
            {
              fnMesh.getPolygonUV(j, 0, uvValues[j][0], uvValues[j][1], &uvSetNames[i]);
            }
            if (uvSetNames[i] == "map1")
            {
              uvSetNames[i] = "st";
            }
            UsdGeomPrimvar uvSet = mesh.CreatePrimvar(TfToken(uvSetNames[i].asChar()), SdfValueTypeNames->Float2Array, UsdGeomTokens->uniform);
            uvSet.Set(uvValues, m_timeCode);
          }
          else
          {
            uvValues.resize(uValues.length());
            if (uvSetNames[i] == "map1")
            {
              uvSetNames[i] = "st";
            }

            float* uptr = &uValues[0];
            float* vptr = &vValues[0];
            float* uvptr = (float*)uvValues.data();
            zipUVs(uptr, vptr, uvptr, vValues.length());

            /// \todo   Ideally I'd want some form of interpolation scheme such as UsdGeomTokens->faceVaryingIndexed
            UsdGeomPrimvar uvSet = mesh.CreatePrimvar(TfToken(uvSetNames[i].asChar()), SdfValueTypeNames->Float2Array, UsdGeomTokens->faceVarying);
            uvSet.Set(uvValues);

            VtArray<int32_t> uvIndices;
            int32_t* ptr = &uvIds[0];
            uvIndices.assign(ptr, ptr + uvIds.length());
            if (leftHanded)
            {
              reverseIndices(uvIndices, uvCounts);
            }

            uvSet.SetIndices(uvIndices, m_timeCode);
          }
        }
      }
      else
      {
        // What to do here then....
      }
    }
  }

  for (uint32_t i = 0; i < diff_report.size(); i++)
  {
    UsdGeomPrimvar& uvSet = diff_report[i].primVar();
    if(diff_report[i].constantInterpolation())
    {
      uvValues.resize(1);
      fnMesh.getUV(0, uvValues[0][0], uvValues[0][1], &diff_report[i].setName());
      uvSet.Set(uvValues, m_timeCode);
      uvSet.SetInterpolation(UsdGeomTokens->constant);
    }
    else
    if(diff_report[i].vertexInterpolation())
    {
      const uint32_t npoints = fnMesh.numVertices();
      uvValues.resize(npoints);
      fnMesh.getUVs(uValues, vValues, &diff_report[i].setName());

      float* uptr = &uValues[0];
      float* vptr = &vValues[0];
      float* uvptr = (float*)uvValues.data();
      if(diff_report[i].indicesToExtract().empty())
      {
        zipUVs(uptr, vptr, uvptr, uValues.length());
      }
      else
      {
        auto& indices = diff_report[i].indicesToExtract();
        for(uint32_t j = 0; j < diff_report[i].indicesToExtract().size(); ++j)
        {
          uint32_t index = indices[j];
          uvptr[j * 2 + 0] = uptr[index];
          uvptr[j * 2 + 1] = vptr[index];
        }
      }
      uvSet.Set(uvValues, m_timeCode);
      uvSet.SetInterpolation(UsdGeomTokens->vertex);
    }
    else
    if(diff_report[i].uniformInterpolation())
    {
      const uint32_t nfaces = fnMesh.numPolygons();
      uvValues.resize(nfaces);
      for(uint32_t j = 0; j < nfaces; ++j)
      {
        fnMesh.getPolygonUV(j, 0, uvValues[j][0], uvValues[j][1], &diff_report[i].setName());
      }
      uvSet.Set(uvValues, m_timeCode);
      uvSet.SetInterpolation(UsdGeomTokens->uniform);
    }
    else
    if(diff_report[i].faceVaryingInterpolation())
    {
      // Initialize the VtArray to the max possible size (facevarying)
      if(fnMesh.getAssignedUVs(uvCounts, uvIds, &diff_report[i].setName()))
      {
        int32_t* ptr = &uvCounts[0];
        if(!isUvSetDataSparse(ptr, uvCounts.length()))
        {
          if(fnMesh.getUVs(uValues, vValues, &diff_report[i].setName()))
          {
            uvValues.resize(uValues.length());

            /// \todo   Ideally I'd want some form of interpolation scheme such as UsdGeomTokens->faceVaryingIndexed
            const UsdGeomPrimvar& uvSet = diff_report[i].primVar();
            if(diff_report[i].dataHasChanged())
            {
              float* uptr = &uValues[0];
              float* vptr = &vValues[0];
              float* uvptr = (float*)uvValues.data();
              zipUVs(uptr, vptr, uvptr, vValues.length());
              uvSet.Set(uvValues, m_timeCode);
            }

            if(diff_report[i].indicesHaveChanged())
            {
              VtArray<int32_t> uvIndices;
              int32_t* ptr = &uvIds[0];
              uvIndices.assign(ptr, ptr + uvIds.length());
              if (leftHanded)
              {
                reverseIndices(uvIndices, uvCounts);
              }
              uvSet.SetIndices(uvIndices, m_timeCode);
            }
          }
          uvSet.SetInterpolation(UsdGeomTokens->faceVarying);
        }
        else
        {
          // What to do here then....
        }
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void interleaveIndexedUvData(float* output, const float* u, const float* v, const int32_t* indices, const uint32_t numIndices)
{
#if AL_UTILS_ENABLE_SIMD

#if defined(__AVX2__) && ENABLE_SOME_AVX_ROUTINES

  const uint32_t numIndices8 = numIndices & ~7;
  uint32_t i = 0;
  for(; i < numIndices8; i += 8, output += 16)
  {
    const i256 I = loadu8i(indices + i);
    const f256 U = i32gather8f(u, I);
    const f256 V = i32gather8f(v, I);
    const f256 uv0 = unpacklo8f(U, V);
    const f256 uv1 = unpackhi8f(U, V);
    storeu8f(output, permute2f128(uv0, uv1, 0x20));
    storeu8f(output + 8, permute2f128(uv0, uv1, 0x31));
  }

  if(numIndices & 0x4)
  {
    const i128 I = loadu4i(indices + i);
    const f128 U = i32gather4f(u, I);
    const f128 V = i32gather4f(v, I);
    const f128 uv0 = unpacklo4f(U, V);
    const f128 uv1 = unpackhi4f(U, V);
    storeu4f(output, uv0);
    storeu4f(output + 4, uv1);
    output += 8;
    i += 4;
  }

#else

  const i128 uptr = splat2i64(intptr_t(u));
  const i128 vptr = splat2i64(intptr_t(v));
  const i128 mask = set4i(0xFFFFFFFF, 0, 0xFFFFFFFF, 0);

  const uint32_t numIndices4 = numIndices & ~3;
  uint32_t i = 0;
  for(; i < numIndices4; i += 4, output += 8)
  {
    // load 4 indices
    const i128 I = loadu4i(indices + i);

    // mask out into 2 pairs of 64 bit indices, and scale values by 4 (using shift)
    const i128 I02 = lshift64(and4i(mask, I), 2);
    const i128 I13 = lshift64(and4i(mask, shiftBytesRight(I, 4)), 2);

    // get addresses by adding the base offset
    const i128 U02 = add2i64(I02, uptr);
    const i128 U13 = add2i64(I13, uptr);
    const i128 V02 = add2i64(I02, vptr);
    const i128 V13 = add2i64(I13, vptr);

    #ifndef __SSE4_1__
    ALIGN16(float* ptrs[8]);
    store4i(ptrs    , U02);
    store4i(ptrs + 2, U13);
    store4i(ptrs + 4, V02);
    store4i(ptrs + 6, V13);

    const f128 u0 = load1f(ptrs[0]);
    const f128 u2 = load1f(ptrs[1]);
    const f128 u1 = load1f(ptrs[2]);
    const f128 u3 = load1f(ptrs[3]);
    const f128 v0 = load1f(ptrs[4]);
    const f128 v2 = load1f(ptrs[5]);
    const f128 v1 = load1f(ptrs[6]);
    const f128 v3 = load1f(ptrs[7]);
    #else
    #define extract_float_ptr(reg, index) reinterpret_cast<const float*>(_mm_extract_epi64(reg, index))
    const f128 u0 = load1f(extract_float_ptr(U02, 0));
    const f128 u2 = load1f(extract_float_ptr(U02, 1));
    const f128 u1 = load1f(extract_float_ptr(U13, 0));
    const f128 u3 = load1f(extract_float_ptr(U13, 1));
    const f128 v0 = load1f(extract_float_ptr(V02, 0));
    const f128 v2 = load1f(extract_float_ptr(V02, 1));
    const f128 v1 = load1f(extract_float_ptr(V13, 0));
    const f128 v3 = load1f(extract_float_ptr(V13, 1));
    #undef extract_ptr
    #endif

    const f128 uv0 = unpacklo4f(u0, v0);
    const f128 uv1 = unpacklo4f(u1, v1);
    storeu4f(output, movelh4f(uv0, uv1));

    const f128 uv2 = unpacklo4f(u2, v2);
    const f128 uv3 = unpacklo4f(u3, v3);
    storeu4f(output + 4, movelh4f(uv2, uv3));
  }

#endif

  switch(numIndices & 0x3)
  {
  case 3: output[4] = u[indices[i + 2]];
          output[5] = v[indices[i + 2]];
  case 2: output[2] = u[indices[i + 1]];
          output[3] = v[indices[i + 1]];
  case 1: output[0] = u[indices[i]];
          output[1] = v[indices[i]];
  default: break;
  }

#else

  for(uint32_t i = 0, j = 0; i < numIndices; ++i, j += 2)
  {
    output[j] = u[indices[i]];
    output[j + 1] = v[indices[i]];
  }

#endif
}

//----------------------------------------------------------------------------------------------------------------------
// Loops through each Colour Set in the mesh writing out a set of non-indexed Colour Values in RGBA format,
// Writes out faceVarying values only
// Have a special case for "displayColor" which write as RGB
// @todo: needs refactoring to handle face/vert/faceVarying correctly, allow separate RGB/A to be written etc.
void MeshExportContext::copyColourSetData()
{
  UsdPrim prim = mesh.GetPrim();
  MStringArray colourSetNames;
  usdmaya::utils::PrimVarDiffReport diff_report;
  if(performDiff)
  {
    colourSetNames = usdmaya::utils::hasNewColourSet(mesh, fnMesh, diff_report);
    if(diff_report.empty() && colourSetNames.length() == 0)
      return;
  }
  else
  {
    MStatus status = fnMesh.getColorSetNames(colourSetNames);
    if(!status || colourSetNames.length() == 0)
      return;
  }

  MColorArray colours;
  VtArray<GfVec4f> colourValues;
  std::vector<uint32_t> indicesToExtract;

  for (uint32_t i = 0; i < colourSetNames.length(); i++)
  {
    MFnMesh::MColorRepresentation representation = fnMesh.getColorRepresentation(colourSetNames[i]);
    fnMesh.getColors(colours, &colourSetNames[i]);
    TfToken interpolation= UsdGeomTokens->faceVarying;

    switch(compaction)
    {
    case kNone:
      break;
    case kBasic:
      interpolation = guessColourSetInterpolationType(&colours[0].r, colours.length());
      break;

    case kMedium:
    case kFull:
      interpolation = guessColourSetInterpolationTypeExtensive(
          &colours[0].r,
          colours.length(),
          fnMesh.numVertices(),
          faceConnects,
          faceCounts,
          indicesToExtract);
      break;
    }

    // if outputting as a vec3 (or we're writing to the displayColor GPrim schema attribute)
    if(MFnMesh::kRGB == representation || colourSetNames[i] ==  "displayColor")
    {
      VtArray<GfVec3f> colourValues;
      if(interpolation == UsdGeomTokens->constant)
      {
        colourValues.resize(1);
        if(colours.length())
        {
          colourValues[0] = GfVec3f(colours[0].r, colours[0].g, colours[0].b);
        }
      }
      else
      {
        if(indicesToExtract.empty())
        {
          colourValues.resize(colours.length());
          for (uint32_t j = 0; j < colours.length(); j++)
          {
            colourValues[j] = GfVec3f(colours[j].r, colours[j].g, colours[j].b);
          }
        }
        else
        {
          colourValues.resize(indicesToExtract.size());
          for (uint32_t j = 0; j < indicesToExtract.size(); j++)
          {
            auto& colour = colours[indicesToExtract[j]];
            colourValues[j] = GfVec3f(colour.r, colour.g, colour.b);
          }
        }
      }
      UsdGeomPrimvar colourSet = mesh.CreatePrimvar(TfToken(colourSetNames[i].asChar()), SdfValueTypeNames->Float3Array, interpolation);
      colourSet.Set(colourValues);
    }
    else
    {
      VtArray<GfVec4f> colourValues;
      if(interpolation == UsdGeomTokens->constant)
      {
        colourValues.resize(1);
        if(colours.length())
        {
          colourValues[0] = GfVec4f(colours[0].r, colours[0].g, colours[0].b, colours[0].a);
        }
      }
      else
      {
        if(indicesToExtract.empty())
        {
          colourValues.resize(colours.length());
          float* to = (float*)colourValues.data();
          const float* from = &colours[0].r;
          memcpy(to, from, sizeof(float) * 4 * colours.length());
        }
        else
        {
          colourValues.resize(indicesToExtract.size());
          for (uint32_t j = 0; j < indicesToExtract.size(); j++)
          {
            auto& colour = colours[indicesToExtract[j]];
            colourValues[j] = GfVec4f(colour.r, colour.g, colour.b, colour.a);
          }
        }
      }
      UsdGeomPrimvar colourSet = mesh.CreatePrimvar(TfToken(colourSetNames[i].asChar()), SdfValueTypeNames->Float4Array, interpolation);
      colourSet.Set(colourValues, m_timeCode);
    }
  }

  for (uint32_t i = 0; i < diff_report.size(); i++)
  {
    MColor defaultColour(1, 0, 0);
    MFnMesh::MColorRepresentation representation = fnMesh.getColorRepresentation(diff_report[i].setName());
    fnMesh.getColors(colours, &diff_report[i].setName(), &defaultColour);

    std::vector<uint32_t>& indicesToExtract = diff_report[i].indicesToExtract();

    TfToken interp = UsdGeomTokens->faceVarying;

    if(diff_report[i].constantInterpolation()) interp = UsdGeomTokens->constant;
    else if(diff_report[i].uniformInterpolation()) interp = UsdGeomTokens->uniform;
    else if(diff_report[i].vertexInterpolation()) interp = UsdGeomTokens->vertex;

    // if outputting as a vec3 (or we're writing to the displayColor GPrim schema attribute)
    if(MFnMesh::kRGB == representation || diff_report[i].setName() ==  "displayColor")
    {
      VtArray<GfVec3f> colourValues;
      if(interp == UsdGeomTokens->constant)
      {
        colourValues.resize(1);
        colourValues[0] = GfVec3f(colours[0].r, colours[0].g, colours[0].b);
      }
      else
      {
        if(indicesToExtract.empty())
        {
          colourValues.resize(colours.length());
          for (uint32_t j = 0; j < colours.length(); j++)
          {
            colourValues[j] = GfVec3f(colours[j].r, colours[j].g, colours[j].b);
          }
        }
        else
        {
          colourValues.resize(indicesToExtract.size());
          for (uint32_t j = 0; j < indicesToExtract.size(); j++)
          {
            auto& colour = colours[indicesToExtract[i]];
            colourValues[j] = GfVec3f(colour.r, colour.g, colour.b);
          }
        }
      }
      UsdGeomPrimvar colourSet = mesh.CreatePrimvar(TfToken(diff_report[i].setName().asChar()), SdfValueTypeNames->Float3Array, interp);
      colourSet.Set(colourValues, m_timeCode);
    }
    else
    {
      VtArray<GfVec4f> colourValues;
      if(interp == UsdGeomTokens->constant)
      {
        colourValues.resize(1);
        colourValues[0] = GfVec4f(colours[0].r, colours[0].g, colours[0].b, colours[0].a);
      }
      else
      {
        if(indicesToExtract.empty())
        {
          colourValues.resize(colours.length());
          float* to = (float*)colourValues.data();
          const float* from = &colours[0].r;
          memcpy(to, from, sizeof(float) * 4 * colours.length());
        }
        else
        {
          colourValues.resize(indicesToExtract.size());
          for (uint32_t j = 0; j < indicesToExtract.size(); j++)
          {
            auto& colour = colours[indicesToExtract[i]];
            colourValues[j] = GfVec4f(colour.r, colour.g, colour.b, colour.a);
          }
        }
      }
      UsdGeomPrimvar colourSet = mesh.CreatePrimvar(TfToken(diff_report[i].setName().asChar()), SdfValueTypeNames->Float4Array, interp);
      colourSet.Set(colourValues, m_timeCode);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MeshExportContext::copyInvisibleHoles()
{
  if(diffMesh & kHoleIndices)
  {
    // Holes - we treat InvisibleFaces as holes
    MUintArray mayaHoles = fnMesh.getInvisibleFaces();
    const uint32_t count = mayaHoles.length();
    if (count)
    {
      VtArray<int32_t> subdHoles(count);
      uint32_t* ptr = &mayaHoles[0];
      memcpy((int32_t*)subdHoles.data(), ptr, count * sizeof(uint32_t));
      mesh.GetHoleIndicesAttr().Set(subdHoles, m_timeCode);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MeshExportContext::copyGlimpseTesselationAttributes()
{
  // TODO: ideally this would be using the ALGlimpseSubdivAPI to create / set
  // these attributes. However, it seems from the docs that getting / setting
  // mesh attributes for custom data is a known issue
  static const TfToken token_gSubdiv("glimpse:subdiv:enabled");
  static const TfToken token_gSubdivMode("glimpse:subdiv:mode");
  static const TfToken token_gSubdivLevel("glimpse:subdiv:level");
  static const TfToken token_gSubdivPrimSizeMult("glimpse:subdiv:primSizeMult");
  static const TfToken token_gSubdivKeepUvBoundary("glimpse:subdiv:keepUvBoundary");
  static const TfToken token_gSubdivEdgeLengthMultiplier("glimpse:subdiv:edgeLengthMultiplier");

  MStatus status;

  UsdPrim prim = mesh.GetPrim();
  MPlug plug = fnMesh.findPlug("gSubdiv", true, &status); // render as subdivision surfaces
  if (status)
  {
    bool renderAsSubd = true;
    plug.getValue(renderAsSubd);
    prim.CreateAttribute(token_gSubdiv, SdfValueTypeNames->Bool).Set(renderAsSubd);
  }

  plug = fnMesh.findPlug("gSubdivMode", true, &status);
  if (status)
  {
    int32_t subdMode = 0;
    plug.getValue(subdMode);
    prim.CreateAttribute(token_gSubdivMode, SdfValueTypeNames->Int).Set(subdMode);
  }

  plug = fnMesh.findPlug("gSubdivLevel", true, &status);
  if (status)
  {
    int32_t subdLevel = -1;
    plug.getValue(subdLevel);
    subdLevel = std::max(subdLevel, -1);
    prim.CreateAttribute(token_gSubdivLevel, SdfValueTypeNames->Int).Set(subdLevel);
  }

  plug = fnMesh.findPlug("gSubdivPrimSizeMult", true, &status);
  if (status)
  {
    float subdivPrimSizeMult = 1.0f;
    plug.getValue(subdivPrimSizeMult);
    prim.CreateAttribute(token_gSubdivPrimSizeMult, SdfValueTypeNames->Float).Set(subdivPrimSizeMult);
  }

  plug = fnMesh.findPlug("gSubdivKeepUvBoundary", true, &status); // render as subdivision surfaces
  if (status)
  {
    bool keepUvBoundary = true;
    plug.getValue(keepUvBoundary);
    prim.CreateAttribute(token_gSubdivKeepUvBoundary, SdfValueTypeNames->Bool, false).Set(keepUvBoundary);
  }

  plug = fnMesh.findPlug("gSubdivEdgeLengthMultiplier", true, &status);
  if (status)
  {
    float subdEdgeLengthMult = 1.0f;
    plug.getValue(subdEdgeLengthMult);
    prim.CreateAttribute(token_gSubdivEdgeLengthMultiplier, SdfValueTypeNames->Float, false).Set(subdEdgeLengthMult);
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MeshExportContext::copyCreaseVertices()
{
  if(diffMesh & (kCornerSharpness | kCornerIndices))
  {
    MUintArray vertIds;
    MDoubleArray creaseData;
    MStatus status = fnMesh.getCreaseVertices(vertIds, creaseData);
    if(status && creaseData.length() && vertIds.length())
    {
      if(diffMesh & kCornerSharpness)
      {
        VtArray<float> subdCornerSharpnesses(creaseData.length());
        AL::usdmaya::utils::doubleToFloat(subdCornerSharpnesses.data(), &creaseData[0], creaseData.length());
        mesh.GetCornerSharpnessesAttr().Set(subdCornerSharpnesses, m_timeCode);
      }

      if(diffMesh & kCornerIndices)
      {
        VtArray<int> subdCornerIndices(vertIds.length());
        memcpy(subdCornerIndices.data(), &vertIds[0], vertIds.length() * sizeof(int32_t));
        mesh.GetCornerIndicesAttr().Set(subdCornerIndices, m_timeCode);
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MeshExportContext::copyCreaseEdges()
{
  if(diffMesh & (kCreaseWeights | kCreaseIndices | kCreaseLengths))
  {
    MUintArray edgeIds;
    MDoubleArray creaseData;
    MStatus status = fnMesh.getCreaseEdges(edgeIds, creaseData);
    if (status && edgeIds.length() && creaseData.length())
    {
      UsdPrim prim = mesh.GetPrim();

      if(diffMesh & kCreaseWeights)
      {
        VtArray<float> usdCreaseValues;
        usdCreaseValues.resize(creaseData.length());
        AL::usdmaya::utils::doubleToFloat(usdCreaseValues.data(), (double*)&creaseData[0], creaseData.length());
        mesh.GetCreaseSharpnessesAttr().Set(usdCreaseValues, m_timeCode);
      }

      if(diffMesh & kCreaseIndices)
      {
        UsdAttribute creases = mesh.GetCreaseIndicesAttr();
        VtArray<int32_t> usdCreaseIndices;
        usdCreaseIndices.resize(edgeIds.length() * 2);

        for(uint32_t i = 0, j = 0; i < edgeIds.length(); ++i, j += 2)
        {
          int2 vertexIds;
          fnMesh.getEdgeVertices(edgeIds[i], vertexIds);
          usdCreaseIndices[j] = vertexIds[0];
          usdCreaseIndices[j + 1] = vertexIds[1];
        }

        creases.Set(usdCreaseIndices, m_timeCode);
      }

      // Note: In the original USD maya bridge, they actually attempt to merge creases.
      // I'm not doing that at all (to be honest their approach looks to be questionable as to whether it would actually
      // work all that well, if at all).
      if(diffMesh & kCreaseLengths)
      {
        UsdAttribute creasesLengths = mesh.GetCreaseLengthsAttr();
        VtArray<int32_t> lengths;
        lengths.resize(creaseData.length());
        std::fill(lengths.begin(), lengths.end(), 2);
        creasesLengths.Set(lengths, m_timeCode);
      }
    }
  }
}
//----------------------------------------------------------------------------------------------------------------------
// Loops through each Colour Set in the mesh writing out a set of non-indexed Colour Values in RGBA format,
// Renames Mayacolours sets, prefixing with "alusd_colour_"
// Writes out per-Face values only
// @todo: needs refactoring to handle face/vert/faceVarying correctly, allow separate RGB/A to be written etc.
void MeshExportContext::copyAnimalFaceColours()
{
  MStringArray colourSetNames;
  MStatus status = fnMesh.getColorSetNames(colourSetNames);
  if(status == MS::kSuccess && colourSetNames.length())
  {
    VtArray<GfVec4f> colourValues;
    colourValues.resize(fnMesh.numPolygons());

    for(uint32_t i = 0; i < colourSetNames.length(); ++i)
    {
      MItMeshPolygon it(fnMesh.object());
      for(uint32_t j = 0; !it.isDone(); it.next(), ++j)
      {
        MColor colour;
        it.getColor(colour, &colourSetNames[i]);
        colourValues[j] = GfVec4f(colour.r, colour.g, colour.b, colour.a);
      }

      std::string name = _alusd_colour;
      name += colourSetNames[i].asChar();
      UsdAttribute colour_set = mesh.GetPrim().CreateAttribute(TfToken(name), SdfValueTypeNames->Float4Array);
      colour_set.Set(colourValues, m_timeCode);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MeshExportContext::copyVertexData(UsdTimeCode time)
{
  if(diffGeom & kPoints)
  {
    UsdAttribute pointsAttr = mesh.GetPointsAttr();
    MStatus status;
    const uint32_t numVertices = fnMesh.numVertices();
    VtArray<GfVec3f> points(numVertices);
    const float* pointsData = fnMesh.getRawPoints(&status);
    if(status)
    {
      memcpy((GfVec3f*)points.data(), pointsData, sizeof(float) * 3 * numVertices);

      pointsAttr.Set(points, time);
    }
    else
    {
      MGlobal::displayError(MString("Unable to access mesh vertices on mesh: ") + fnMesh.fullPathName());
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MeshExportContext::copyNormalData(UsdTimeCode time)
{
  if(diffGeom & kNormals)
  {
    UsdAttribute normalsAttr = mesh.GetNormalsAttr();
    MStatus status;
    const uint32_t numNormals = fnMesh.numNormals();
    const float* normalsData = fnMesh.getRawNormals(&status);
    if(status && numNormals)
    {
      // if prim vars are all identical, we have a constant value
      if(usd::utils::vec3AreAllTheSame(normalsData, numNormals))
      {
        VtArray<GfVec3f> normals(1);
        mesh.SetNormalsInterpolation(UsdGeomTokens->constant);
        normals[0][0] = normalsData[0];
        normals[0][1] = normalsData[1];
        normals[0][2] = normalsData[2];
      }
      else
      {
        VtArray<GfVec3f> normals(numNormals);
        mesh.SetNormalsInterpolation(UsdGeomTokens->faceVarying);
        memcpy((GfVec3f*)normals.data(), normalsData, sizeof(float) * 3 * numNormals);
        normalsAttr.Set(normals, time);
      }
    }
    else
    {
      MGlobal::displayError(MString("Unable to access mesh normals on mesh: ") + fnMesh.fullPathName());
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MeshExportContext::copyGlimpseUserDataAttributes()
{
  // TODO: glimpse user data can be set on any DAG node, push up to DagNodeTranslator?
  static const std::string glimpse_namespace("glimpse:userData:");

  MStatus status;
  MPlug plug;

  MString name;
  int type = 0;
  MString value;

  UsdPrim prim = mesh.GetPrim();

  auto allInts = [](const MStringArray& tokens) -> bool
  {
    auto length = tokens.length();
    for(uint32_t i = 0; i < length; i++)
    {
      if (!tokens[i].isInt())
      {
        return false;
      }
    }
    return true;
  };

  auto allFloats = [](const MStringArray& tokens) -> bool
  {
    auto length = tokens.length();
    for(uint32_t i = 0; i < length; i++)
    {
      if (!tokens[i].isFloat())
      {
        return false;
      }
    }

    return true;
  };

  auto copyUserData = [&](const MString& name, int type, const MString& value)
  {
    std::stringstream ss;
    ss << glimpse_namespace << name.asChar();

    TfToken nameToken(ss.str());

    MStringArray tokens;
    value.split(' ', tokens);

    switch(type)
    {
    case GlimpseUserDataTypes::kInt:
      {
        // int
        prim.CreateAttribute(nameToken, SdfValueTypeNames->Int, false).Set(value.asInt(), m_timeCode);
      }
      break;
      
    case GlimpseUserDataTypes::kInt2:
      {
        // int2
        if(tokens.length() == 2 && allInts(tokens))
        {
          GfVec2i vec(tokens[0].asInt(), tokens[1].asInt());
          prim.CreateAttribute(nameToken, SdfValueTypeNames->Int2, false).Set(vec, m_timeCode);
        }
      }
      break;
      
    case GlimpseUserDataTypes::kInt3:
      {
        // int3
        if(tokens.length() == 3 && allInts(tokens))
        {
          GfVec3i vec(tokens[0].asInt(), tokens[1].asInt(), tokens[2].asInt());
          prim.CreateAttribute(nameToken, SdfValueTypeNames->Int3, false).Set(vec, m_timeCode);
        }
      }
      break;
      
    case GlimpseUserDataTypes::kFloat:
      {
        // float
        prim.CreateAttribute(nameToken, SdfValueTypeNames->Float, false).Set(value.asFloat(), m_timeCode);
      }
      break;
      
    case GlimpseUserDataTypes::kVector:
      {
        // vector
        if(tokens.length() == 3 && allFloats(tokens))
        {
          GfVec3f vec(tokens[0].asFloat(), tokens[1].asFloat(), tokens[2].asFloat());
          prim.CreateAttribute(nameToken, SdfValueTypeNames->Vector3f, false).Set(vec, m_timeCode);
        }
      }
      break;
      
    case GlimpseUserDataTypes::kColor:
      {
        // color
        if(tokens.length() == 3 && allFloats(tokens))
        {
          GfVec3f vec(tokens[0].asFloat(), tokens[1].asFloat(), tokens[2].asFloat());
          prim.CreateAttribute(nameToken, SdfValueTypeNames->Color3f, false).Set(vec, m_timeCode);
        }
      }
      break;
      
    case GlimpseUserDataTypes::kString:
      {
        // string
        prim.CreateAttribute(nameToken, SdfValueTypeNames->String, false).Set(AL::maya::utils::convert(value), m_timeCode);
      }
      break;
      
    case GlimpseUserDataTypes::kMatrix:
      {
        // matrix
        // the value stored for this entry is a 4x3
        if(tokens.length() == 12 && allFloats(tokens))
        {
          double components[4][4] =
            {
              {tokens[0].asDouble(), tokens[1].asDouble(), tokens[2].asDouble(),   0.0},
              {tokens[3].asDouble(), tokens[4].asDouble(), tokens[5].asDouble(),   0.0},
              {tokens[6].asDouble(), tokens[7].asDouble(), tokens[8].asDouble(),   0.0},
              {tokens[9].asDouble(), tokens[10].asDouble(), tokens[11].asDouble(), 1.0}
            };

          // TODO: not sure why but SdfValueTypeNames does not have a defined type for Matrix4f only Matrix4d
          GfMatrix4d matrix(components);
          prim.CreateAttribute(nameToken, SdfValueTypeNames->Matrix4d, false).Set(matrix);
        }
      }
      break;
      
    default:
      // unsupported user data type
      break;
    }
  };

  plug = fnMesh.findPlug("gUserData", true);
  if(status && plug.isCompound() && plug.isArray())
  {
    for(uint32_t i = 0; i < plug.numElements(); i++)
    {
      MPlug compoundPlug = plug[i];

      MPlug namePlug = compoundPlug.child(0);
      MPlug typePlug = compoundPlug.child(1);
      MPlug valuePlug = compoundPlug.child(2);

      namePlug.getValue(name);
      typePlug.getValue(type);
      valuePlug.getValue(value);

      copyUserData(name, type, value);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
} // utils
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
