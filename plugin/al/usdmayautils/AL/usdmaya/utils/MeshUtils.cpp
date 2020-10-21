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

#include "AL/usdmaya/utils/MeshUtils.h"
#include "AL/maya/utils/Utils.h"
#include "AL/usdmaya/utils/DiffPrimVar.h"
#include "AL/usdmaya/utils/Utils.h"

#include <mayaUsdUtils/DebugCodes.h>
#include <mayaUsdUtils/DiffCore.h>

#include <pxr/usd/usdUtils/pipeline.h>

#include <maya/MItMeshPolygon.h>
#include <maya/MGlobal.h>

#include <iostream>

namespace AL {
namespace usdmaya {
namespace utils {


const TfToken prefToken("pref");
const TfToken displayColorToken("displayColor");
const TfToken displayOpacityToken("displayOpacity");
const TfToken primvarDisplayOpacityToken("primvars:displayOpacity");

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

  mesh.GetPointsAttr().Get(&pointData, m_timeCode);

  // According to the docs for UsdGeomMesh: If 'normals' and 'primvars:normals' are both specified, the latter has precedence.
  const TfToken primvarNormalsToken("primvars:normals");
  TfToken interpolation = mesh.GetNormalsInterpolation();
  bool hasNormalsOpinion = false;
  if(mesh.HasPrimvar(primvarNormalsToken))
  {
    UsdGeomPrimvar primvar = mesh.GetPrimvar(primvarNormalsToken);
    interpolation = primvar.GetInterpolation();
    hasNormalsOpinion = true;
    primvar.Get(&normalsData, m_timeCode);
  }
  else    
  if(mesh.GetNormalsAttr().HasAuthoredValueOpinion())
  {
    mesh.GetNormalsAttr().Get(&normalsData, m_timeCode);
    hasNormalsOpinion = mesh.GetNormalsAttr().HasAuthoredValueOpinion();
  }

  points.setLength(pointData.size());
  convert3DArrayTo4DArray((const float*)pointData.cdata(), &points[0].x, pointData.size());

  memcpy(&counts[0], (const int32_t*)faceVertexCounts.cdata(), sizeof(int32_t) * faceVertexCounts.size());
  memcpy(&connects[0], (const int32_t*)faceVertexIndices.cdata(), sizeof(int32_t) * faceVertexIndices.size());

  if(hasNormalsOpinion)
  {
    if(interpolation == UsdGeomTokens->faceVarying ||
       interpolation == UsdGeomTokens->varying)
    {
      normals.setLength(normalsData.size());
      double* const optr = &normals[0].x;
      const float* const iptr = (const float*)normalsData.cdata();
      for(size_t i = 0, n = normalsData.size() * 3; i < n; i += 3)
      {
        optr[i + 0] = iptr[i + 0];
        optr[i + 1] = iptr[i + 1];
        optr[i + 2] = iptr[i + 2];
      }
    }
    else
    if(interpolation == UsdGeomTokens->uniform)
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
    if(interpolation == UsdGeomTokens->vertex)
    {
      const float* const iptr = (const float*)normalsData.cdata();
      normals.setLength(normalsData.size());
      for(uint32_t i = 0, nf = normalsData.size(); i < nf; ++i)
      {
        normals[i] = MVector(iptr[3 * i], iptr[3 * i + 1], iptr[3 * i + 2]);
      }
    }
  }
  else
  {
    // check for cases where data is left handed.
    // Maya fails
    TfToken orientation;
    bool leftHanded = (mesh.GetOrientationAttr().Get(&orientation, m_timeCode) && orientation == UsdGeomTokens->leftHanded);
    if(leftHanded)
    {
      size_t numPoints = pointData.size();
      size_t numFaces = faceVertexCounts.size();
      std::vector<GfVec3f> tempNormals(numPoints, GfVec3f(0, 0, 0));

      const GfVec3f* const ptemp = (const GfVec3f*)pointData.cdata();
      GfVec3f* const pnorm = (GfVec3f*)tempNormals.data();
      const int32_t* pcounts = (const int32_t*)faceVertexCounts.cdata();
      const int32_t* pconnects = (const int32_t*)faceVertexIndices.cdata();

      // compute each face normal, and add into the array of vertex normals.
      for(size_t i = 0, offset = 0; i < numFaces; ++i)
      {
        int32_t nverts = pcounts[i];
        const int32_t* pface =  pconnects + offset;

        offset += nverts;

        // grab first two points & normals, and compute edge.
        GfVec3f v0 = ptemp[pface[0]];
        GfVec3f v1 = ptemp[pface[1]];
        GfVec3f n0 = pnorm[pface[0]];
        GfVec3f n1 = pnorm[pface[1]];
        GfVec3f n2;
        GfVec3f e1 = v1 - v0;

        // loop through each triangle in face
        for(int32_t j = 2; j < nverts; ++j)
        {
          // compute last edge in tri (will have previous edge from last iteration)
          const GfVec3f v2 = ptemp[pface[j]];
          n2 = pnorm[pface[j]];
          GfVec3f e2 = v2 - v0;

          // compute triangle normal
          GfVec3f fn = GfCross(e2, e1);
          n0 += fn;
          n1 += fn;
          n2 += fn;

          // write summed normal (with original value) back into array
          pnorm[pface[j - 1]] = n1;

          // for next iteration
          n1 = n2;
          e1 = e2;
        }

        // write back first and last normal
        pnorm[pface[0]] = n0;
        pnorm[pface[nverts-1]] = n2;
      }

      // normalise each normal in the array
      for(size_t j = 0; j < numPoints; ++j)
      {
        pnorm[j] = GfGetNormalized(pnorm[j]);
      }

      // now expand array into a set of vertex-face normals
      {
        normals.setLength(connects.length());
        for(uint32_t i = 0, nf = connects.length(); i < nf; ++i)
        {
          int index = connects[i];
          normals[i] = MVector(pnorm[index][0], pnorm[index][1], pnorm[index][2]);
        }
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
  // Lambda to set vertex normals in unlocked state
  auto setUnlockedVertexNormals = [&](MVectorArray& normals) -> bool
  {
    MIntArray vertexList(fnMesh.numVertices());
    for(uint32_t i = 0, n = fnMesh.numVertices(); i < n; ++i)
    {
      vertexList[i] = i;
    }
    if (fnMesh.setVertexNormals(normals, vertexList, MSpace::kObject))
    {
      return fnMesh.unlockVertexNormals(vertexList);
    }
    return false;
  };

  // Lambda to set face vertex normals in unlocked state
  auto setUnlockedFaceVertexNormals = [&](MVectorArray& normals, MIntArray& faceList, MIntArray& vertexList) -> bool
  {
    if (fnMesh.setFaceVertexNormals(normals, faceList, vertexList, MSpace::kObject))
    {
      return fnMesh.unlockFaceVertexNormals(faceList, vertexList);
    }
    return false;
  };
  
  if(normals.length())
  {
    // According to the docs for UsdGeomMesh: If 'normals' and 'primvars:normals' are both specified, the latter has precedence.
    TfToken primvarNormalsToken("primvars:normals");
    if(mesh.HasPrimvar(primvarNormalsToken))
    {
      UsdGeomPrimvar primvar = mesh.GetPrimvar(primvarNormalsToken);
      const TfToken interpolation = primvar.GetInterpolation();
      const bool isIndexed = primvar.IsIndexed();
      if(interpolation == UsdGeomTokens->vertex)
      {
        if(isIndexed)
        {
          VtIntArray indices;
          primvar.GetIndices(&indices, m_timeCode);

          MVectorArray ns(indices.size());
          for(uint32_t i = 0, n = indices.size(); i < n; ++i)
          {
            ns[i] = normals[indices[i]];
          }
          return setUnlockedVertexNormals(ns);
        }
        else
        {
          return setUnlockedVertexNormals(normals);
        }
      }
      else
      if(interpolation == UsdGeomTokens->faceVarying)
      {
        MIntArray normalsFaceIds;
        normalsFaceIds.setLength(connects.length());
        
        int32_t* normalsFaceIdsPtr = &normalsFaceIds[0];
        for (uint32_t i = 0, k = 0, n = counts.length(); i < n; i++)
        {
          for (uint32_t j = 0, m = counts[i]; j < m; j++, ++k)
          {
            normalsFaceIdsPtr[k] = i;
          }
        }

        if(isIndexed)
        {
          VtIntArray indices;
          primvar.GetIndices(&indices, m_timeCode);

          MVectorArray ns(indices.size());
          for(uint32_t i = 0, n = indices.size(); i < n; ++i)
          {
            ns[i] = normals[indices[i]];
          }

          return setUnlockedFaceVertexNormals(ns, normalsFaceIds, connects);
        }
        else
        {
          return setUnlockedFaceVertexNormals(normals, normalsFaceIds, connects);
        }

      }
    }
    else
    {
      if(mesh.GetNormalsInterpolation() == UsdGeomTokens->vertex)
      {
        return setUnlockedVertexNormals(normals);
      }
      else
      {
        MIntArray normalsFaceIds;
        normalsFaceIds.setLength(connects.length());
        int32_t* normalsFaceIdsPtr = &normalsFaceIds[0];
        if (normals.length() == uint32_t(fnMesh.numFaceVertices()))
        {
          for (uint32_t i = 0, k = 0, n = counts.length(); i < n; i++)
          {
            for (uint32_t j = 0, m = counts[i]; j < m; j++, ++k)
            {
              normalsFaceIdsPtr[k] = i;
            }
          }
        }
        return setUnlockedFaceVertexNormals(normals, normalsFaceIds, connects);
      }
    }
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


void MeshImportContext::applyColourSetData()
{
  const std::vector<UsdGeomPrimvar> primvars = mesh.GetPrimvars();
  for (auto it = primvars.begin(), end = primvars.end(); it != end; ++it)
  {

    const UsdGeomPrimvar& primvar = *it;
    TfToken name, interpolation;
    SdfValueTypeName typeName;
    int elementSize;
    
    primvar.GetDeclarationInfo(&name, &typeName, &interpolation, &elementSize);

    TfToken role = typeName.GetRole();
    if(role != SdfValueRoleNames->Color)
      continue;

    // early out for channels that are definitely not colourSets
    if (name == prefToken || name == displayOpacityToken)
      continue;
    VtValue vtValue;
    MColorArray colours;
    MString colourSetName(name.GetText());
    fnMesh.setDisplayColors(true);

    if (primvar.Get(&vtValue, m_timeCode))
    {

      //early out for primvar channels that are not Vec3/Vec4 (so exclude UVs for example)
      if (!(vtValue.IsHolding<VtArray<GfVec3f>>() || vtValue.IsHolding<VtArray<GfVec4f>>()))
        continue;

      MStatus status;
      colourSetName = fnMesh.createColorSetWithName(colourSetName, nullptr, nullptr, &status);
      if (status)
      {
        status = fnMesh.setCurrentColorSetName(colourSetName);
        if (status)
        {
          // Prepare maya colours array
          MFnMesh::MColorRepresentation representation{};
          if (vtValue.IsHolding<VtArray<GfVec3f>>())
          {
            //If we can find the special displayColorToken used by USD, let's check for the optional matching displayOpacityToken too
            bool setCombinedDisplayAndOpacityColourSet = false;
            if (name == displayColorToken)
            {
              UsdPrim prim = mesh.GetPrim();
              if (prim.HasAttribute(primvarDisplayOpacityToken))
              {
                UsdAttribute usdAttr = prim.GetAttribute(primvarDisplayOpacityToken);
                UsdGeomPrimvar primvar = UsdGeomPrimvar(usdAttr);
                VtValue opacityValues;
                if (primvar.Get(&opacityValues, m_timeCode))
                {
                  if (opacityValues.IsHolding<VtArray<float>>())
                  {
                    const VtArray<float> rawValOpacity = opacityValues.UncheckedGet<VtArray<float>>();
                    colours.setLength(rawValOpacity.size());
                    const VtArray<GfVec3f> rawValColour = vtValue.UncheckedGet<VtArray<GfVec3f>>();
                    assert(rawValOpacity.size() == rawValColour.size());
        
                    for (uint32_t i = 0, n = rawValColour.size(); i < n; ++i)
                      colours[i] = MColor(rawValColour[i][0], rawValColour[i][1], rawValColour[i][2], rawValOpacity[i]);
                    representation = MFnMesh::kRGBA;
                    setCombinedDisplayAndOpacityColourSet = true;
                  }
                }
              }
            }
            if (!setCombinedDisplayAndOpacityColourSet) 
            {
              const VtArray<GfVec3f> rawVal = vtValue.UncheckedGet<VtArray<GfVec3f>>();
              colours.setLength(rawVal.size());
              for (uint32_t i = 0, n = rawVal.size(); i < n; ++i)
                colours[i] = MColor(rawVal[i][0], rawVal[i][1], rawVal[i][2]);
              representation = MFnMesh::kRGB;
            }
          }
          else if (vtValue.IsHolding<VtArray<GfVec4f> >())
          {
            const VtArray<GfVec4f> rawVal = vtValue.UncheckedGet<VtArray<GfVec4f> >();
            colours.setLength(rawVal.size());
            memcpy(&colours[0][0], (const float*) rawVal.cdata(), sizeof(float) * 4 * rawVal.size());
            representation = MFnMesh::kRGBA;
          }
        
          // Set colors
          if (!fnMesh.setColors(colours, &colourSetName, representation))
          {
            TF_DEBUG(MAYAUSDUTILS_INFO).Msg("Failed to set colours for colour set \"%s\" on mesh \"%s\", error: %s\n",
                colourSetName.asChar(), fnMesh.name().asChar(), status.errorString().asChar());
            continue;
          }

          // When primvar is indexed assume these indices
          MIntArray mayaIndices;
          VtIntArray usdindices;
          if (primvar.GetIndices(&usdindices, m_timeCode))
          {
            mayaIndices.setLength(usdindices.size());
            std::memcpy(&mayaIndices[0], usdindices.cdata(), sizeof(int) * usdindices.size());
            if (mayaIndices.length() != connects.length())
            {
              TF_DEBUG(MAYAUSDUTILS_INFO).Msg(
                  "Retrieved indexed values are not compatible with topology for colour set \"%s\" on mesh \"%s\"\n",
                  colourSetName.asChar(), fnMesh.name().asChar());
              continue;
            }
          }

          // Otherwise generate indices based on interpolation
          if (mayaIndices.length() == 0)
          {
            if (interpolation == UsdGeomTokens->faceVarying)
            {
              generateIncrementingIndices(mayaIndices, colours.length());
            }
            else if (interpolation == UsdGeomTokens->uniform)
            {
              if (colours.length() == counts.length())
              {
                mayaIndices.setLength(connects.length());
                for (uint32_t i = 0, idx = 0, n = counts.length(); i < n; ++i)
                {
                  for (uint32_t j = 0; j < size_t(counts[i]); ++j)
                  {
                    mayaIndices[idx++] = i;
                  }
                }
              }
            }
            else if (interpolation == UsdGeomTokens->vertex)
            {
              mayaIndices = connects;
            }
            else if (interpolation == UsdGeomTokens->constant)
            {
              mayaIndices = MIntArray(connects.length(), 0);
            }
          }

          if (mayaIndices.length() != uint32_t(fnMesh.numFaceVertices()))
          {
            TF_DEBUG(MAYAUSDUTILS_INFO).Msg("Incompatible colour indices for colour set \"%s\" on mesh \"%s\"\n",
                colourSetName.asChar(), fnMesh.name().asChar());
            continue;
          }
          // Assign colors to indices
          if (!fnMesh.assignColors(mayaIndices, &colourSetName))
          {
            TF_DEBUG(MAYAUSDUTILS_INFO).Msg(
                "Failed to assign colour indices for colour set \"%s\" on mesh \"%s\", error: %s\n",
                colourSetName.asChar(), fnMesh.name().asChar(), status.errorString().asChar());
          }
        }
      }
    }
  }
}


//----------------------------------------------------------------------------------------------------------------------
void MeshImportContext::applyUVs()
{
  const TfToken prefToken("pref");
  const std::vector<UsdGeomPrimvar> primvars = mesh.GetPrimvars();
  for(auto it = primvars.begin(), end = primvars.end(); it != end; ++it)
  {
    const UsdGeomPrimvar& primvar = *it;
    TfToken name, interpolation;
    SdfValueTypeName typeName;
    int elementSize;
    
    primvar.GetDeclarationInfo(&name, &typeName, &interpolation, &elementSize);

    // early out for channels that are definitely not UVs
    if (name == prefToken || name == displayOpacityToken || name == displayColorToken)
      continue;
    VtValue vtValue;

    if (primvar.Get(&vtValue, m_timeCode))
    {
      if (vtValue.IsHolding<VtArray<GfVec2f> >())
      {
        MIntArray mayaIndices;
        MFloatArray u, v;
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
              primvar.GetIndices(&usdindices, UsdTimeCode::EarliestTime());
              mayaIndices.setLength(usdindices.size());
              std::memcpy(&mayaIndices[0], usdindices.cdata(), sizeof(int) * usdindices.size());
              s = fnMesh.assignUVs(counts, mayaIndices, uv_set);
              if(!s)
              {
                TF_DEBUG(MAYAUSDUTILS_INFO).Msg("Failed to assign UVS for uvset \"%s\" on mesh \"%s\", error: %s\n",
                    uvSetName.asChar(), fnMesh.name().asChar(), s.errorString().asChar());
              }
            }
            else
            {
              TF_DEBUG(MAYAUSDUTILS_INFO).Msg("Failed to set UVS for uvset \"%s\" on mesh \"%s\", error: %s\n",
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
                TF_DEBUG(MAYAUSDUTILS_INFO).Msg("Failed to assign UVS for uvset \"%s\" on mesh \"%s\", error: %s\n",
                    uvSetName.asChar(), fnMesh.name().asChar(), s.errorString().asChar());
              }
            }
            else
            if (interpolation == UsdGeomTokens->vertex)
            {
              MStatus s = fnMesh.assignUVs(counts, connects, uv_set);
              if(!s)
              {
                TF_DEBUG(MAYAUSDUTILS_INFO).Msg("Failed to assign UVS for uvset \"%s\" on mesh \"%s\", error: %s\n",
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
                TF_DEBUG(MAYAUSDUTILS_INFO).Msg("Failed to assign UVS for uvset \"%s\" on mesh \"%s\", error: %s\n",
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
                TF_DEBUG(MAYAUSDUTILS_INFO).Msg("Failed to assign UVS for uvset \"%s\" on mesh \"%s\", error: %s\n",
                    uvSetName.asChar(), fnMesh.name().asChar(), s.errorString().asChar());
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
    CompactionLevel compactionLevel,
    bool reverseNormals)
  : fnMesh(), faceCounts(), faceConnects(), m_timeCode(timeCode), mesh(mesh), compaction(compactionLevel), 
    performDiff(performDiff), reverseNormals(reverseNormals)
{
  MStatus status = fnMesh.setObject(path);
  valid = (status == MS::kSuccess);
  AL_MAYA_CHECK_ERROR2(status, MString("unable to attach function set to mesh") + path.fullPathName());
  if(status)
  {
    fnMesh.getVertices(faceCounts, faceConnects);
  }

  if(!reverseNormals && fnMesh.findPlug("opposite", true).asBool())
  {
    mesh.CreateOrientationAttr().Set(UsdGeomTokens->leftHanded);
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
void MeshExportContext::copyUvSetData()
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
  size_t coloursLength=0;
  VtArray<GfVec4f> colourValues;
  std::vector<uint32_t> indicesToExtract;

  for (uint32_t i = 0; i < colourSetNames.length(); i++)
  {
    MFnMesh::MColorRepresentation representation = fnMesh.getColorRepresentation(colourSetNames[i]);
    MItMeshPolygon it(fnMesh.object());
    while(!it.isDone())
    {
      MColorArray faceColours;
      it.getColors(faceColours, &colourSetNames[i]);
      it.next();
      // Append face colours
      uint32_t offset = colours.length();
      colours.setLength(offset+faceColours.length());
      for (uint32_t j = 0, n = faceColours.length(); j < n; ++j)
        colours[offset+j] = faceColours[j];
    }
    TfToken interpolation= UsdGeomTokens->faceVarying;
    coloursLength = colours.length();

    switch(compaction)
    {
    case kNone:
      break;
    case kBasic:
      interpolation = guessColourSetInterpolationType(&colours[0].r, coloursLength);
      break;

    case kMedium:
    case kFull:
      interpolation = guessColourSetInterpolationTypeExtensive(
          &colours[0].r,
          coloursLength,
          fnMesh.numVertices(),
          faceConnects,
          faceCounts,
          indicesToExtract);
      break;
    }

    // if outputting as a vec3 (or we're writing to the displayColor GPrim schema attribute)
    if (colourSetNames[i] ==  displayColorToken.GetText())
    {

      if (representation >= MFnMesh::kRGB)
      {
        VtArray<GfVec3f> colourValues;
        if(interpolation == UsdGeomTokens->constant)
        {
          colourValues.resize(1);
          if(coloursLength)
          {
            colourValues[0] = GfVec3f(colours[0].r, colours[0].g, colours[0].b);
          }
        }
        else
        {
          if(indicesToExtract.empty())
          {
            colourValues.resize(coloursLength);
            for (uint32_t j = 0; j < coloursLength; j++)
            {
              colourValues[j] = GfVec3f(colours[j].r, colours[j].g, colours[j].b);
            }
          }
          else
          {
            colourValues.resize(indicesToExtract.size());
            for (uint32_t j = 0; j < indicesToExtract.size(); j++)
            {
              assert(indicesToExtract[j] < coloursLength);
              auto& colour = colours[indicesToExtract[j]];
              colourValues[j] = GfVec3f(colour.r, colour.g, colour.b);
            }
          }
        }
        UsdGeomPrimvar colourSet = mesh.CreatePrimvar(TfToken(colourSetNames[i].asChar()), SdfValueTypeNames->Color3fArray, interpolation);
        colourSet.Set(colourValues, m_timeCode);
      }
      if (MFnMesh::kRGBA == representation )
      {
        VtArray<float> alphaValues;
        if(interpolation == UsdGeomTokens->constant)
        {
          alphaValues.resize(1);
          if(coloursLength)
          {
              alphaValues[0] = colours[0].a;
          }
        }
        else
        {
          if(indicesToExtract.empty())
          {
            alphaValues.resize(coloursLength);
            for (uint32_t j = 0; j < coloursLength; j++)
            {
              alphaValues[j] = colours[0].a;
            }
          }
          else
          {
            alphaValues.resize(indicesToExtract.size());
            for (uint32_t j = 0; j < indicesToExtract.size(); j++)
            {
              assert(indicesToExtract[j] < coloursLength);
              auto& colour = colours[indicesToExtract[j]];
              alphaValues[j] = colour.a;
            }
          }
        }
        UsdGeomPrimvar opacitySet = mesh.CreatePrimvar(displayOpacityToken, SdfValueTypeNames->FloatArray, interpolation);
        opacitySet.Set(alphaValues, m_timeCode);
      }

    }
    else
    {
      VtArray<GfVec4f> colourValues;
      if(interpolation == UsdGeomTokens->constant)
      {
        colourValues.resize(1);
        if(coloursLength)
        {
          colourValues[0] = GfVec4f(colours[0].r, colours[0].g, colours[0].b, colours[0].a);
        }
      }
      else
      {
        if(indicesToExtract.empty())
        {
          colourValues.resize(coloursLength);
          float* to = (float*)colourValues.data();
          const float* from = &colours[0].r;
          memcpy(to, from, sizeof(float) * 4 * coloursLength);
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
      UsdGeomPrimvar colourSet = mesh.CreatePrimvar(TfToken(colourSetNames[i].asChar()), SdfValueTypeNames->Color4fArray, interpolation);
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
    if(MFnMesh::kRGB == representation || diff_report[i].setName() ==  displayColorToken.GetText())
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
          colourValues.resize(coloursLength);
          for (uint32_t j = 0; j < coloursLength; j++)
          {
            colourValues[j] = GfVec3f(colours[j].r, colours[j].g, colours[j].b);
          }
        }
        else
        {
          colourValues.resize(indicesToExtract.size());
          for (uint32_t j = 0; j < indicesToExtract.size(); j++)
          {
            assert(indicesToExtract[j] < coloursLength);
            auto& colour = colours[indicesToExtract[j]];
            colourValues[j] = GfVec3f(colour.r, colour.g, colour.b);
          }
        }
      }
      UsdGeomPrimvar colourSet = mesh.CreatePrimvar(TfToken(diff_report[i].setName().asChar()), SdfValueTypeNames->Color3fArray, interp);
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
          colourValues.resize(coloursLength);
          float* to = (float*)colourValues.data();
          const float* from = &colours[0].r;
          memcpy(to, from, sizeof(float) * 4 * coloursLength);
        }
        else
        {
          colourValues.resize(indicesToExtract.size());
          for (uint32_t j = 0; j < indicesToExtract.size(); j++)
          {
            assert(indicesToExtract[j] < coloursLength);
            auto& colour = colours[indicesToExtract[j]];
            colourValues[j] = GfVec4f(colour.r, colour.g, colour.b, colour.a);
          }
        }
      }
      UsdGeomPrimvar colourSet = mesh.CreatePrimvar(TfToken(diff_report[i].setName().asChar()), SdfValueTypeNames->Color4fArray, interp);
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
    if(UsdAttribute pointsAttr = mesh.GetPointsAttr())
    {
      MStatus status;
      const uint32_t numVertices = fnMesh.numVertices();
      const float* pointsData = fnMesh.getRawPoints(&status);
      if(status)
      {
        const GfVec3f* vecData = reinterpret_cast<const GfVec3f*>(pointsData);
        VtArray<GfVec3f> points(vecData, vecData + numVertices);
        pointsAttr.Set(points, time);
      }
      else
      {
        MGlobal::displayError(MString("Unable to access mesh vertices on mesh: ") + fnMesh.fullPathName());
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MeshExportContext::copyExtentData(UsdTimeCode time)
{
  if(diffGeom & kExtent)
  {
    if(UsdAttribute extentAttr = mesh.GetExtentAttr())
    {
      MStatus status;
      const float* pointsData = fnMesh.getRawPoints(&status);
      if(status)
      {
        const GfVec3f* vecData = reinterpret_cast<const GfVec3f*>(pointsData);
        const uint32_t numVertices = fnMesh.numVertices();
        VtArray<GfVec3f> points(vecData, vecData + numVertices);

        VtArray<GfVec3f> extent(2);
        UsdGeomPointBased::ComputeExtent(points, &extent);
        extentAttr.Set(extent, time);
      }
      else
      {
        MGlobal::displayError(MString("Unable to access mesh vertices on mesh: ") + fnMesh.fullPathName());
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MeshExportContext::copyBindPoseData(UsdTimeCode time)
{
  if(diffGeom & kPoints)
  {

    UsdGeomPrimvar pRefPrimVarAttr = mesh.CreatePrimvar(
            UsdUtilsGetPrefName(),
            SdfValueTypeNames->Point3fArray,
            UsdGeomTokens->vertex);

    if(pRefPrimVarAttr)
    {
      MStatus status;
      const uint32_t numVertices = fnMesh.numVertices();
      const float* pointsData = fnMesh.getRawPoints(&status);
      if(status)
      {
        const GfVec3f* vecData = reinterpret_cast<const GfVec3f*>(pointsData);
        VtArray<GfVec3f> points(vecData, vecData + numVertices);

        pRefPrimVarAttr.Set(points, time);
      }
      else
      {
        MGlobal::displayError(MString("Unable to access mesh vertices on mesh: ") + fnMesh.fullPathName());
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void MeshExportContext::copyNormalData(UsdTimeCode time, bool copyAsPrimvar)
{
  const TfToken normalPrimvarName("primvars:normals");
  if(diffGeom & kNormals)
  {
    UsdAttribute normalsAttr = mesh.GetNormalsAttr();
    UsdGeomPrimvar primvar;
    if(copyAsPrimvar)
    {
      primvar = mesh.CreatePrimvar(normalPrimvarName, SdfValueTypeNames->Float3Array);
      normalsAttr = primvar.GetAttr();
    }

    bool invertNormals = false;
    if(fnMesh.findPlug("opposite", true).asBool())
    {
      invertNormals = reverseNormals;
    }

    {
      MStatus status;
      const uint32_t numNormals = fnMesh.numNormals();
      const float* normalsData = fnMesh.getRawNormals(&status);
      const GfVec3f* vecData = reinterpret_cast<const GfVec3f*>(normalsData);
      if(status && numNormals)
      {
        MIntArray normalCounts, normalIndices;
        fnMesh.getNormalIds(normalCounts, normalIndices);

        // if prim vars are all identical, we have a constant value
        if(MayaUsdUtils::vec3AreAllTheSame(normalsData, numNormals))
        {
          VtArray<GfVec3f> normals(1);
          if(copyAsPrimvar)
          {
            primvar.SetInterpolation(UsdGeomTokens->constant);
          }
          else
          {
            mesh.SetNormalsInterpolation(UsdGeomTokens->constant);
          }
          normals[0][0] = normalsData[0];
          normals[0][1] = normalsData[1];
          normals[0][2] = normalsData[2];
          normalsAttr.Set(normals, time);
        }
        else
        if(numNormals != normalIndices.length())
        {
          if(MayaUsdUtils::compareArray(&normalIndices[0], &faceConnects[0], normalIndices.length(), faceConnects.length()))
          {
            if(copyAsPrimvar)
            {
              primvar.SetInterpolation(UsdGeomTokens->vertex);
            }
            else
            {
              mesh.SetNormalsInterpolation(UsdGeomTokens->vertex);
            }

            VtArray<GfVec3f> normals(vecData, vecData + numNormals);
            normalsAttr.Set(normals, time);
          }
          else
          {
            std::unordered_map<uint32_t, uint32_t> missing;
            bool isPerVertex = true;
            for(uint32_t i = 0, n = normalIndices.length(); isPerVertex && i < n; ++i)
            {
              if(normalIndices[i] != faceConnects[i])
              {
                auto it = missing.find(normalIndices[i]);
                if(it == missing.end())
                {
                  missing[normalIndices[i]] = faceConnects[i];
                }
                else
                if(it->second != uint32_t(faceConnects[i]))
                {
                  isPerVertex = false;
                }
              }
            }

            if(isPerVertex)
            {
              VtArray<GfVec3f> normals(vecData, vecData + numNormals);
              for(auto& c : missing)
              {
                const uint32_t orig = c.first;
                const uint32_t remapped = c.second;
                const uint32_t index = 3 * orig;
                const GfVec3f normal(normalsData[index], normalsData[index + 1], normalsData[index + 2]);
                normals[remapped] = normal;
              }
              if(copyAsPrimvar)
              {
                primvar.SetInterpolation(UsdGeomTokens->vertex);
              }
              else
              {
                mesh.SetNormalsInterpolation(UsdGeomTokens->vertex);
              }
              normalsAttr.Set(normals, time);
            }
            else
            {
              if(copyAsPrimvar)
              {
                primvar.SetInterpolation(UsdGeomTokens->faceVarying);
                VtArray<GfVec3f> normals(numNormals);
                for(uint32_t i = 0, n = numNormals; i < n; ++i)
                {
                  const uint32_t index = 3 * i;
                  normals[i] = GfVec3f(normalsData[index], normalsData[index + 1], normalsData[index + 2]);
                }
                normalsAttr.Set(normals, time);  
                VtArray<int> normalIds(normalIndices.length());
                memcpy(normalIds.data(), &normalIndices[0], sizeof(int) * normalIndices.length());
                primvar.SetIndices(normalIds, time);  
              }
              else
              {
                VtArray<GfVec3f> normals(normalIndices.length());
                for(uint32_t i = 0, n = normalIndices.length(); i < n; ++i)
                {
                  const uint32_t index = 3 * normalIndices[i];
                  normals[i] = GfVec3f(normalsData[index], normalsData[index + 1], normalsData[index + 2]);
                }
                mesh.SetNormalsInterpolation(UsdGeomTokens->faceVarying);
                normalsAttr.Set(normals, time);  
              }
            }
          }
        }
        else
        {
          // run a check to see if the normalIds is relevant in this case.
          bool isOrdered = true;
          for(uint32_t i = 0, n = uint32_t(normalIndices.length()); i < n; ++i)
          {
            if(uint32_t(normalIndices[i]) != i)
            {
              isOrdered = false;
              break;
            }
          }
          if(isOrdered)
          {
            if(copyAsPrimvar)
            {
              primvar.SetInterpolation(UsdGeomTokens->faceVarying);
            }
            else
            {
              mesh.SetNormalsInterpolation(UsdGeomTokens->faceVarying);
            }
            VtArray<GfVec3f> normals(vecData, vecData + numNormals);
            normalsAttr.Set(normals, time);
          }
          else
          {
            if(copyAsPrimvar)
            {
              primvar.SetInterpolation(UsdGeomTokens->faceVarying);
            }
            else
            {
              mesh.SetNormalsInterpolation(UsdGeomTokens->faceVarying);
            }
            VtArray<GfVec3f> normals(numNormals);
            for(uint32_t i = 0, n = normalIndices.length(); i < n; ++i)
            {
              const uint32_t index = 3 * normalIndices[i];
              normals[i] = GfVec3f(normalsData[index], normalsData[index + 1], normalsData[index + 2]);
            }
            normalsAttr.Set(normals, time);
          }
        }

        if(invertNormals)
        {
          VtArray<GfVec3f> normals;
          normalsAttr.Get(&normals, time);
          for(size_t i = 0; i < normals.size(); ++i)
          {
            normals[i] = -normals[i]; 
          }
          normalsAttr.Set(normals, time);
        }
      }
      else
      {
        MGlobal::displayError(MString("Unable to access mesh normals on mesh: ") + fnMesh.fullPathName());
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
} // utils
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
