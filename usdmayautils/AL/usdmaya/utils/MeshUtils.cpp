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

#include "maya/MItMeshPolygon.h"
#include "maya/MGlobal.h"

namespace AL {
namespace usdmaya {
namespace utils {

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
void gatherFaceConnectsAndVertices(const UsdGeomMesh& mesh, MFloatPointArray& points,
  MVectorArray &normals, MIntArray& counts, MIntArray& connects, const bool leftHanded)
{
  static const UsdTimeCode timeCode = UsdTimeCode::Default();

  VtArray<GfVec3f> pointData;
  VtArray<GfVec3f> normalsData;
  VtArray<int> faceVertexCounts;
  VtArray<int> faceVertexIndices;

  UsdAttribute fvc = mesh.GetFaceVertexCountsAttr();
  UsdAttribute fvi = mesh.GetFaceVertexIndicesAttr();

  fvc.Get(&faceVertexCounts, timeCode);
  counts.setLength(faceVertexCounts.size());
  fvi.Get(&faceVertexIndices, timeCode);
  connects.setLength(faceVertexIndices.size());

  if(leftHanded)
  {
    int32_t* index = static_cast<int32_t*>(faceVertexIndices.data());
    int32_t* indexMax = index + faceVertexIndices.size();
    for(auto faceVertexCount: faceVertexCounts)
    {
      int32_t* start = index;
      int32_t* end = index + (faceVertexCount - 1);
      if(start < indexMax and end < indexMax)
      {
        std::reverse(start, end + 1);
        index += faceVertexCount;
      }
      else
      {
        faceVertexIndices.clear();
        fvi.Get(&faceVertexIndices, timeCode);
        break;
      }
    }
  }

  mesh.GetPointsAttr().Get(&pointData, UsdTimeCode::Default());
  mesh.GetPointsAttr().Get(&pointData, timeCode);
  mesh.GetNormalsAttr().Get(&normalsData, timeCode);

  points.setLength(pointData.size());
  convert3DArrayTo4DArray((const float*)pointData.cdata(), &points[0].x, pointData.size());

  memcpy(&counts[0], (const int32_t*)faceVertexCounts.cdata(), sizeof(int32_t) * faceVertexCounts.size());
  memcpy(&connects[0], (const int32_t*)faceVertexIndices.cdata(), sizeof(int32_t) * faceVertexIndices.size());

  normals.setLength(normalsData.size());
  if(leftHanded and normalsData.size())
  {
    int32_t index = 0;
    double* const optr = &normals[0].x;
    const double* const iptr = (const double*)normalsData.cdata();
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
    memcpy(&normals[0], &normalsData[0], sizeof(double) * 3 * normalsData.size());
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
void applyHoleFaces(const UsdGeomMesh& mesh, MFnMesh& fnMesh)
{
  // Set Holes
  VtArray<int> holeIndices;
  mesh.GetHoleIndicesAttr().Get(&holeIndices);
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
void applyAnimalColourSets(const UsdPrim& from, MFnMesh& fnMesh, const MIntArray& counts)
{
  std::vector<UsdAttribute> colour_sets;

  std::vector<UsdAttribute> attributes = from.GetAttributes();
  for(uint32_t i = 0, n = attributes.size(); i < n; ++i)
  {
    const UsdAttribute& attribute = attributes[i];
    if(attribute.IsCustom())
    {
      const std::string& attrName = attribute.GetName().GetString();
      if(!strncmp(_alusd_colour, attrName.c_str(), strlen(_alusd_colour)))
      {
        colour_sets.push_back(attribute);
      }
    }
  }

  for(auto it = colour_sets.begin(); it != colour_sets.end(); ++it)
  {
    UsdAttribute attribute = *it;
    if(attribute)
    {
      std::string setName = attribute.GetName().GetString().substr(strlen(_alusd_colour));
      MString colourSetName(setName.c_str(), setName.size());
      if(fnMesh.createColorSet(colourSetName))
      {
        if(fnMesh.setCurrentColorSetName(colourSetName))
        {
          VtArray<GfVec4f> colours;
          attribute.Get(&colours);

          MIntArray faceIds;
          generateIncrementingIndices(faceIds, fnMesh.numPolygons());

          MColorArray faceColours((const MColor*)colours.cdata(), colours.size());
          fnMesh.setFaceColors(faceColours, faceIds);
        }
      }
    }
  }

  if(!colour_sets.empty())
  {
    fnMesh.setDisplayColors(true);
  }
}

//----------------------------------------------------------------------------------------------------------------------
bool applyVertexCreases(const UsdGeomMesh& from, MFnMesh& fnMesh)
{
  UsdAttribute cornerIndices = from.GetCornerIndicesAttr();
  UsdAttribute cornerSharpness = from.GetCornerSharpnessesAttr();
  if(cornerIndices.IsAuthored() && cornerIndices.HasValue() &&
     cornerSharpness.IsAuthored() && cornerSharpness.HasValue())
  {
    VtArray<int32_t> vertexIdValues;
    VtArray<float> creaseValues;
    cornerIndices.Get(&vertexIdValues);
    cornerSharpness.Get(&creaseValues);

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
void applyAnimalVertexCreases(const UsdPrim& from, MFnMesh& fnMesh)
{
  static const TfToken alusd_crease_vertices_data("alusd_crease_vertices_data");
  static const TfToken alusd_crease_vertices_ids("alusd_crease_vertices_ids");

  UsdAttribute creases = from.GetAttribute(alusd_crease_vertices_data);
  UsdAttribute vertices = from.GetAttribute(alusd_crease_vertices_ids);
  if(creases && vertices)
  {
    VtArray<double> creaseValues;
    VtArray<int32_t> vertexIdValues;
    creases.Get(&creaseValues);
    vertices.Get(&vertexIdValues);
    MUintArray vertexIds((const uint32_t*)vertexIdValues.cdata(), vertexIdValues.size());
    MDoubleArray creaseData(creaseValues.cdata(), creaseValues.size());
    if(!fnMesh.setCreaseVertices(vertexIds, creaseData))
    {
      std::cerr << "Unable to set crease vertices on mesh " << fnMesh.name().asChar() << std::endl;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
bool applyEdgeCreases(const UsdGeomMesh& from, MFnMesh& fnMesh)
{
  UsdAttribute creaseIndices = from.GetCreaseIndicesAttr();
  UsdAttribute creaseLengths = from.GetCreaseLengthsAttr();
  UsdAttribute creaseSharpness = from.GetCreaseSharpnessesAttr();

  if(creaseIndices.IsAuthored() && creaseIndices.HasValue() &&
     creaseLengths.IsAuthored() && creaseLengths.HasValue() &&
     creaseSharpness.IsAuthored() && creaseSharpness.HasValue())
  {
    VtArray<int32_t> indices, lengths;
    VtArray<float> sharpness;

    creaseIndices.Get(&indices);
    creaseLengths.Get(&lengths);
    creaseSharpness.Get(&sharpness);

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
void applyAnimalEdgeCreases(const UsdPrim& from, MFnMesh& fnMesh)
{
  static const TfToken alusd_crease_edges_data("alusd_crease_edges_data");
  static const TfToken alusd_crease_edges_ids("alusd_crease_edges_ids");

  UsdAttribute creases = from.GetAttribute(alusd_crease_edges_data);
  UsdAttribute edges = from.GetAttribute(alusd_crease_edges_ids);
  if(creases && edges)
  {
    VtArray<double> creaseValues;
    VtArray<int32_t> edgesIdValues;
    creases.Get(&creaseValues);
    edges.Get(&edgesIdValues);

    MUintArray creaseEdgeIds;
    creaseEdgeIds.setLength(creaseValues.size());
    MIntArray edgeIds;
    MDoubleArray createValues(creaseValues.cdata(), creaseValues.size());
    MObject temp = fnMesh.object();
    MItMeshVertex iter(temp);
    for(size_t i = 0, k = 0; i < edgesIdValues.size(); i += 2, ++k)
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
            creaseEdgeIds[k] = edgeIds[j];
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

    if(!fnMesh.setCreaseEdges(creaseEdgeIds, createValues))
    {
      std::cerr << "Unable to set crease edges on mesh " << fnMesh.name().asChar() << std::endl;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void applyGlimpseSubdivParams(const UsdPrim& from, MFnMesh& fnMesh)
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

  const UsdGeomMesh mesh(from);

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
    MPlug plug = fnMesh.findPlug("gSubdiv", &status);
    if(status)
    {
      bool value;
      glimpse_gSubdiv_attr.Get(&value);
      plug.setBool(value);
    }
  }

  if(glimpse_gSubdivKeepUvBoundary_attr)
  {
    MPlug plug = fnMesh.findPlug("gSubdivKeepUvBoundary", &status);
    if(status)
    {
      bool value;
      glimpse_gSubdivKeepUvBoundary_attr.Get(&value);
      plug.setBool(value);
    }
  }

  if(glimpse_gSubdivLevel_attr)
  {
    MPlug plug = fnMesh.findPlug("gSubdivLevel", &status);
    if(status)
    {
      int32_t value;
      glimpse_gSubdivLevel_attr.Get(&value);
      plug.setInt(value);
    }
  }

  if(glimpse_gSubdivMode_attr)
  {
    MPlug plug = fnMesh.findPlug("gSubdivMode", &status);
    if(status)
    {
      int32_t value;
      glimpse_gSubdivMode_attr.Get(&value);
      plug.setInt(value);
    }
  }

  if(glimpse_gSubdivPrimSizeMult_attr)
  {
    MPlug plug = fnMesh.findPlug("gSubdivPrimSizeMult", &status);
    if(status)
    {
      float value;
      glimpse_gSubdivPrimSizeMult_attr.Get(&value);
      plug.setFloat(value);
    }
  }

  if(glimpse_gSubdivEdgeLengthMultiplier_attr)
  {
    MPlug plug = fnMesh.findPlug("gSubdivEdgeLengthMultiplier", &status);
    if(status)
    {
      float value;
      glimpse_gSubdivEdgeLengthMultiplier_attr.Get(&value);
      plug.setFloat(value);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void applyPrimVars(const UsdGeomMesh& mesh, MFnMesh& fnMesh, const MIntArray& counts, const MIntArray& connects)
{
  MFloatArray u, v;
  MIntArray indices;
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

    if (primvar.Get(&vtValue, UsdTimeCode::Default()))
    {
      if (vtValue.IsHolding<VtArray<GfVec2f> >())
      {
        const VtArray<GfVec2f> rawVal = vtValue.Get<VtArray<GfVec2f> >();
        u.setLength(rawVal.size());
        v.setLength(rawVal.size());
        unzipUVs((const float*)rawVal.cdata(), &u[0], &v[0], rawVal.size());

        MString uvSetName(name.GetText());
        MString* uv_set = &uvSetName;
        if (uvSetName == "st")
        {
          uvSetName = "map1";
          uv_set = 0;
        }

        if(primvar.IsIndexed())
        {
          if (interpolation == UsdGeomTokens->faceVarying)
          {
            if(!uv_set || fnMesh.createUVSet(uvSetName))
            {
              if(fnMesh.setUVs(u, v, uv_set))
              {
                VtIntArray usdindices;
                primvar.GetIndices(&usdindices);
                indices = MIntArray(usdindices.cdata(), usdindices.size());
                if(!fnMesh.assignUVs(counts, indices, uv_set))
                {
                  std::cout << "Failed to assign UVS for uvset: " << uvSetName.asChar() << ", on mesh " << fnMesh.name().asChar() << "\n";
                }
              }
              else
              {
                std::cout << "Failed to set UVS for uvset: " << uvSetName.asChar() << ", on mesh " << fnMesh.name().asChar() << "\n";
              }
            }
            else
            {
              std::cout << "Failed to create uvset: " << uvSetName.asChar() << ", on mesh " << fnMesh.name().asChar() << "\n";
            }
          }
        }
        else
        {
          if(fnMesh.createUVSet(uvSetName))
          {
            if(fnMesh.setUVs(u, v, uv_set))
            {
              if (interpolation == UsdGeomTokens->faceVarying)
              {
                generateIncrementingIndices(indices, rawVal.size());
                if(!fnMesh.assignUVs(counts, indices, uv_set))
                {
                  std::cout << "Failed to assign UVS for uvset: " << uvSetName.asChar() << ", on mesh " << fnMesh.name().asChar() << "\n";
                }
              }
              else
              if (interpolation == UsdGeomTokens->vertex)
              {
                if(!fnMesh.assignUVs(counts, connects, uv_set))
                {
                  std::cout << "Failed to assign UVS for uvset: " << uvSetName.asChar() << ", on mesh " << fnMesh.name().asChar() << "\n";
                }
              }
            }
          }
        }
      }
      else
      if (vtValue.IsHolding<VtArray<GfVec4f> >())
      {
        MString colourSetName(name.GetText());
        if(fnMesh.createColorSet(colourSetName))
        {
          const VtArray<GfVec4f> rawVal = vtValue.Get<VtArray<GfVec4f> >();
          colours.setLength(rawVal.size());
          memcpy(&colours[0], (const float*)rawVal.cdata(), sizeof(float) * 4 * rawVal.size());

          if(fnMesh.setColors(colours, &colourSetName))
          {
            if(!fnMesh.setCurrentColorSetName(colourSetName))
            {
              std::cerr << "Failed to set colours on mesh " << std::endl;
            }
          }
        }
        else
          std::cerr << "Failed to set colour set on mesh " << std::endl;
      }
    }
  }
}


//----------------------------------------------------------------------------------------------------------------------
void copyFaceConnectsAndPolyCounts(UsdGeomMesh& mesh, const MFnMesh& fnMesh)
{
  MIntArray faceConnects, polyCounts;
  fnMesh.getVertices(polyCounts, faceConnects);
  VtArray<int32_t> faceVertexCounts(polyCounts.length());
  VtArray<int32_t> faceVertexIndices(faceConnects.length());
  memcpy((int32_t*)faceVertexCounts.data(), &polyCounts[0], sizeof(uint32_t) * polyCounts.length());
  memcpy((int32_t*)faceVertexIndices.data(), &faceConnects[0], sizeof(uint32_t) * faceConnects.length());
  mesh.GetFaceVertexCountsAttr().Set(faceVertexCounts);
  mesh.GetFaceVertexIndicesAttr().Set(faceVertexIndices);
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
void copyUvSetData(UsdGeomMesh& mesh, const MFnMesh& fnMesh, const bool leftHanded)
{
  UsdPrim prim = mesh.GetPrim();
  MStringArray uvSetNames;
  MStatus status = fnMesh.getUVSetNames(uvSetNames);
  if(status == MS::kSuccess && uvSetNames.length() > 0)
  {
    VtArray<GfVec2f> uvValues;
    MFloatArray uValues, vValues;
    MIntArray uvCounts, uvIds;

    for (uint32_t i = 0; i < uvSetNames.length(); i++)
    {
      // Initialize the VtArray to the max possible size (facevarying)
      if(fnMesh.getAssignedUVs(uvCounts, uvIds, &uvSetNames[i]))
      {
       int32_t* ptr = &uvCounts[0];
       if(!isUvSetDataSparse(ptr, uvCounts.length()))
       {
         if(fnMesh.getUVs(uValues, vValues, &uvSetNames[i]))
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

           uvSet.SetIndices(uvIndices);
         }
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
void copyColourSetData(UsdGeomMesh& mesh, const MFnMesh& fnMesh)
{
  UsdPrim prim = mesh.GetPrim();
  MStringArray colourSetNames;
  MStatus status = fnMesh.getColorSetNames(colourSetNames);
  if(status == MS::kSuccess && colourSetNames.length())
  {
    VtArray<GfVec4f> colourValues;
    MColorArray colours;
    MIntArray uvCounts, uvIds;

    for (uint32_t i = 0; i < colourSetNames.length(); i++)
    {
      MColor defaultColour(1, 0, 0);

      //If we're writing displayColor which is part of the GPrim Schema, we need to force to Vec3.
      if(colourSetNames[i] ==  "displayColor")
      {
        if(fnMesh.getColors(colours, &colourSetNames[i]))
        {
          VtArray<GfVec3f> colourValues;
          colourValues.resize(colours.length());
          for (uint32_t j = 0; j < colours.length(); j++) {
            colourValues[j] = GfVec3f(colours[j].r, colours[j].g, colours[j].b);
          }
          UsdGeomPrimvar colourSet = mesh.CreatePrimvar(TfToken(colourSetNames[i].asChar()), SdfValueTypeNames->Float3Array, UsdGeomTokens->faceVarying);
          colourSet.Set(colourValues);
        }
      }
      else
      {
        //@todo: This code is duplicated - refactor to handle RGB/Vec3, RGBA/Vec4, RGB/Vec3 + A/Vec1 etc
        if(fnMesh.getColors(colours, &colourSetNames[i]))
        {
          colourValues.resize(colours.length());
          float* to = (float*)colourValues.data();
          const float* from = &colours[0].r;
          memcpy(to, from, sizeof(float) * 4 * colours.length());

          UsdGeomPrimvar colourSet = mesh.CreatePrimvar(TfToken(colourSetNames[i].asChar()), SdfValueTypeNames->Float4Array, UsdGeomTokens->faceVarying);
          colourSet.Set(colourValues);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void copyInvisibleHoles(UsdGeomMesh& mesh, const MFnMesh& fnMesh)
{
  // Holes - we treat InvisibleFaces as holes
  MUintArray mayaHoles = fnMesh.getInvisibleFaces();
  const uint32_t count = mayaHoles.length();
  if (count)
  {
    VtArray<int32_t> subdHoles(count);
    uint32_t* ptr = &mayaHoles[0];
    memcpy((int32_t*)subdHoles.data(), ptr, count);
    mesh.GetHoleIndicesAttr().Set(subdHoles);
  }
}

//----------------------------------------------------------------------------------------------------------------------
void copyGlimpseTesselationAttributes(UsdGeomMesh& mesh, const MFnMesh& fnMesh)
{
  MStatus status;
  MPlug plug;

  bool renderAsSubd = true;
  int32_t subdMode = 0;
  int32_t subdLevel = -1;
  float subdivPrimSizeMult = 1.0f;
  bool keepUvBoundary = false;
  float subdEdgeLengthMult = 1.0f;

  plug = fnMesh.findPlug("gSubdiv", &status); // render as subdivision surfaces
  if (status) plug.getValue(renderAsSubd);

  plug = fnMesh.findPlug("gSubdivMode", &status);
  if (status) plug.getValue(subdMode);

  plug = fnMesh.findPlug("gSubdivLevel", &status);
  if (status)
  {
    plug.getValue(subdLevel);
    subdLevel = std::max(subdLevel, -1);
  }

  plug = fnMesh.findPlug("gSubdivPrimSizeMult", &status);
  if (status) plug.getValue(subdivPrimSizeMult);

  plug = fnMesh.findPlug("gSubdivKeepUvBoundary", &status); // render as subdivision surfaces
  if (status) plug.getValue(keepUvBoundary);

  plug = fnMesh.findPlug("gSubdivEdgeLengthMultiplier", &status);
  if (status) plug.getValue(subdEdgeLengthMult);

  UsdPrim prim = mesh.GetPrim();

  // TODO: ideally this would be using the ALGlimpseSubdivAPI to create / set
  // these attributes. However, it seems from the docs that getting / setting
  // mesh attributes for custom data is a known issue
  static const TfToken token_gSubdiv("glimpse:subdiv:enabled");
  static const TfToken token_gSubdivMode("glimpse:subdiv:mode");
  static const TfToken token_gSubdivLevel("glimpse:subdiv:level");
  static const TfToken token_gSubdivPrimSizeMult("glimpse:subdiv:primSizeMult");
  static const TfToken token_gSubdivKeepUvBoundary("glimpse:subdiv:keepUvBoundary");
  static const TfToken token_gSubdivEdgeLengthMultiplier("glimpse:subdiv:edgeLengthMultiplier");

  prim.CreateAttribute(token_gSubdiv, SdfValueTypeNames->Bool, false).Set(renderAsSubd);
  prim.CreateAttribute(token_gSubdivMode, SdfValueTypeNames->Int, false).Set(subdMode);
  prim.CreateAttribute(token_gSubdivLevel, SdfValueTypeNames->Int, false).Set(subdLevel);
  prim.CreateAttribute(token_gSubdivPrimSizeMult, SdfValueTypeNames->Float, false).Set(subdivPrimSizeMult);
  prim.CreateAttribute(token_gSubdivKeepUvBoundary, SdfValueTypeNames->Bool, false).Set(keepUvBoundary);
  prim.CreateAttribute(token_gSubdivEdgeLengthMultiplier, SdfValueTypeNames->Float, false).Set(subdEdgeLengthMult);
}

//----------------------------------------------------------------------------------------------------------------------
void copyAnimalCreaseVertices(UsdGeomMesh& mesh, const MFnMesh& fnMesh)
{
  MUintArray vertIds;
  MDoubleArray creaseData;
  MStatus status = fnMesh.getCreaseVertices(vertIds, creaseData);
  if(status)
  {
    UsdPrim prim = mesh.GetPrim();
    if(creaseData.length())
    {
      UsdAttribute creases = prim.CreateAttribute(TfToken("alusd_crease_vertices_data"), SdfValueTypeNames->DoubleArray);
      VtArray<double> usdCreaseValues;
      double* ptr = &creaseData[0];
      usdCreaseValues.assign(ptr, ptr + creaseData.length());
      creases.Set(usdCreaseValues);
    }

    if(vertIds.length())
    {
      UsdAttribute creases = prim.CreateAttribute(TfToken("alusd_crease_vertices_ids"), SdfValueTypeNames->IntArray);
      VtArray<int32_t> usdCreaseIndices;
      int32_t* ptr = (int32_t*)&vertIds[0];
      usdCreaseIndices.assign(ptr, ptr + vertIds.length());
      creases.Set(usdCreaseIndices);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void copyCreaseVertices(UsdGeomMesh& mesh, const MFnMesh& fnMesh)
{
  MUintArray vertIds;
  MDoubleArray creaseData;
  MStatus status = fnMesh.getCreaseVertices(vertIds, creaseData);
  if(status && creaseData.length() && vertIds.length())
  {
    VtArray<int> subdCornerIndices(vertIds.length());
    VtArray<float> subdCornerSharpnesses(creaseData.length());
    AL::usdmaya::utils::doubleToFloat(subdCornerSharpnesses.data(), &creaseData[0], creaseData.length());
    memcpy(subdCornerIndices.data(), (uint32_t*)&vertIds[0], vertIds.length() * sizeof(int32_t));
    mesh.GetCornerIndicesAttr().Set(subdCornerIndices);
    mesh.GetCornerSharpnessesAttr().Set(subdCornerSharpnesses);
  }
}

//----------------------------------------------------------------------------------------------------------------------
void copyAnimalCreaseEdges(UsdGeomMesh& mesh, const MFnMesh& fnMesh)
{
  MUintArray edgeIds;
  MDoubleArray creaseData;
  MStatus status = fnMesh.getCreaseEdges(edgeIds, creaseData);
  if (status && edgeIds.length() && creaseData.length())
  {
    UsdPrim prim = mesh.GetPrim();
    {
      UsdAttribute creases = prim.CreateAttribute(TfToken("alusd_crease_edges_data"), SdfValueTypeNames->DoubleArray);
      VtArray<double> usdCreaseValues;
      usdCreaseValues.resize(creaseData.length());
      memcpy((double*)usdCreaseValues.data(), (double*)&creaseData[0], creaseData.length() * sizeof(double));
      creases.Set(usdCreaseValues);
    }

    {
      UsdAttribute creases = prim.CreateAttribute(TfToken("alusd_crease_edges_ids"), SdfValueTypeNames->IntArray);
      VtArray<int32_t> usdCreaseIndices;
      usdCreaseIndices.resize(edgeIds.length() * 2);

      for(uint32_t i = 0, j = 0; i < edgeIds.length(); ++i, j += 2)
      {
        int2 vertexIds;
        fnMesh.getEdgeVertices(edgeIds[i], vertexIds);
        usdCreaseIndices[j] = vertexIds[0];
        usdCreaseIndices[j + 1] = vertexIds[1];
      }

      creases.Set(usdCreaseIndices);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void copyCreaseEdges(UsdGeomMesh& mesh, const MFnMesh& fnMesh)
{
  MUintArray edgeIds;
  MDoubleArray creaseData;
  MStatus status = fnMesh.getCreaseEdges(edgeIds, creaseData);
  if (status && edgeIds.length() && creaseData.length())
  {
    UsdPrim prim = mesh.GetPrim();
    {
      VtArray<float> usdCreaseValues;
      usdCreaseValues.resize(creaseData.length());
      AL::usdmaya::utils::doubleToFloat(usdCreaseValues.data(), (double*)&creaseData[0], creaseData.length());
      mesh.GetCreaseSharpnessesAttr().Set(usdCreaseValues);
    }

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

      creases.Set(usdCreaseIndices);
    }

    // Note: In the original USD maya bridge, they actually attempt to merge creases.
    // I'm not doing that at all (to be honest their approach looks to be questionable as to whether it would actually
    // work all that well, if at all).
    {
      UsdAttribute creasesLengths = mesh.GetCreaseLengthsAttr();
      VtArray<int32_t> lengths;
      lengths.resize(creaseData.length());
      std::fill(lengths.begin(), lengths.end(), 2);
      creasesLengths.Set(lengths);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

// Loops through each Colour Set in the mesh writing out a set of non-indexed Colour Values in RGBA format,
// Renames Mayacolours sets, prefixing with "alusd_colour_"
// Writes out per-Face values only
// @todo: needs refactoring to handle face/vert/faceVarying correctly, allow separate RGB/A to be written etc.
void copyAnimalFaceColours(UsdGeomMesh& mesh, const MFnMesh& fnMesh)
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
      colour_set.Set(colourValues);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void copyVertexData(const MFnMesh& fnMesh, const UsdAttribute& pointsAttr, UsdTimeCode time)
{
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
//----------------------------------------------------------------------------------------------------------------------
} // utils
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
