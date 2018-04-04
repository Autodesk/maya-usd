//
// Copyright 2018 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.//
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
#include "maya/MFnMesh.h"
#include "maya/MDoubleArray.h"
#include "maya/MFloatArray.h"
#include "maya/MIntArray.h"
#include "maya/MUintArray.h"
#include "maya/MItMeshFaceVertex.h"
#include "maya/MGlobal.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usd/attribute.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace utils {

//----------------------------------------------------------------------------------------------------------------------
uint32_t diffGeom(UsdGeomPointBased& geom, MFnMesh& mesh, UsdTimeCode timeCode, uint32_t exportMask)
{
  uint32_t result = 0;

  if(exportMask & kPoints)
  {
    VtArray<GfVec3f> pointData;
    geom.GetPointsAttr().Get(&pointData, timeCode);

    MStatus status;
    const float* const usdPoints = (const float* const)pointData.cdata();
    const float* const mayaPoints = (const float* const)mesh.getRawPoints(&status);
    const size_t usdPointsCount = pointData.size();
    const size_t mayaPointsCount = mesh.numVertices();
    if(!usd::utils::compareArray(usdPoints, mayaPoints, usdPointsCount * 3, mayaPointsCount * 3))
    {
      result |= kPoints;
    }
  }

  if(exportMask & kNormals)
  {
    VtArray<GfVec3f> normalData;
    geom.GetNormalsAttr().Get(&normalData, timeCode);

    MStatus status;
    const float* const usdNormals = (const float* const)normalData.cdata();
    const float* const mayaNormals = (const float* const)mesh.getRawNormals(&status);
    const size_t usdNormalsCount = normalData.size();
    const size_t mayaNormalsCount = mesh.numVertices();
    if(!usd::utils::compareArray(usdNormals, mayaNormals, usdNormalsCount * 3, mayaNormalsCount * 3))
    {
      result |= kNormals;
    }
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------
uint32_t diffFaceVertices(UsdGeomMesh& geom, MFnMesh& mesh, UsdTimeCode timeCode, uint32_t exportMask)
{
  uint32_t result = 0;

  if(exportMask & (kFaceVertexCounts | kFaceVertexIndices))
  {
    const int numPolygons = mesh.numPolygons();
    const int numFaceVerts = mesh.numFaceVertices();

    VtArray<int> faceVertexCounts;
    VtArray<int> faceVertexIndices;
    UsdAttribute fvc = geom.GetFaceVertexCountsAttr();
    UsdAttribute fvi = geom.GetFaceVertexIndicesAttr();

    fvc.Get(&faceVertexCounts, timeCode);
    fvi.Get(&faceVertexIndices, timeCode);

    if(numPolygons == faceVertexCounts.size() && numFaceVerts == faceVertexIndices.size())
    {
      const int* const pFaceVertexCounts = faceVertexCounts.cdata();
      MIntArray vertexCount;
      MIntArray vertexList;
      if(mesh.getVertices(vertexCount, vertexList))
      {

      }

      if(numPolygons && !usd::utils::compareArray(&vertexCount[0], pFaceVertexCounts, numPolygons, numPolygons))
      {
        result |= kFaceVertexCounts;
      }

      const int* const pFaceVertexIndices = faceVertexIndices.cdata();
      if(numFaceVerts && !usd::utils::compareArray(&vertexList[0], pFaceVertexIndices, numFaceVerts, numFaceVerts))
      {
        result |= kFaceVertexIndices;
      }
    }
    else
    if(numPolygons != faceVertexCounts.size() && numFaceVerts == faceVertexIndices.size())
    {
      // I'm going to test this, but I suspect it's impossible
      result |= (kFaceVertexIndices | kFaceVertexCounts);
    }
    else
    if(numPolygons == faceVertexCounts.size() && numFaceVerts != faceVertexIndices.size())
    {
      // If the number of face verts have changed, but the number of polygons remains the same,
      // then since numFaceVerts = sum(faceVertexCounts), we can assume that one of the faceVertexCounts
      // elements must have changed.
      result |= (kFaceVertexIndices | kFaceVertexCounts);
    }
    else
    {
      // counts differ, no point in checking actual values, we'll just update the new values
      result |= (kFaceVertexIndices | kFaceVertexCounts);
    }
  }

  //
  if(exportMask & kHoleIndices)
  {
    VtArray<int> holeIndices;
    UsdAttribute holesAttr = geom.GetHoleIndicesAttr();
    holesAttr.Get(&holeIndices, timeCode);

    MUintArray mayaHoleIndices = mesh.getInvisibleFaces();

    const uint32_t numHoleIndices = holeIndices.size();
    const uint32_t numMayaHoleIndices = mayaHoleIndices.length();
    if(numMayaHoleIndices != numHoleIndices)
    {
      result |= kHoleIndices;
    }
    else
    if(numMayaHoleIndices && !usd::utils::compareArray((int32_t*)&mayaHoleIndices[0], holeIndices.cdata(), numMayaHoleIndices, numHoleIndices))
    {
      result |= kHoleIndices;
    }
  }

  if(exportMask & (kCreaseWeights | kCreaseIndices))
  {
    MUintArray mayaEdgeCreaseIndices;
    MDoubleArray mayaCreaseWeights;
    mesh.getCreaseEdges(mayaEdgeCreaseIndices, mayaCreaseWeights);

    if(exportMask & kCreaseIndices)
    {
      VtArray<int> creasesIndices;
      UsdAttribute creasesAttr = geom.GetCreaseIndicesAttr();
      creasesAttr.Get(&creasesIndices, timeCode);

      const uint32_t numCreaseIndices = creasesIndices.size();
      MUintArray mayaCreaseIndices;
      mayaCreaseIndices.setLength(mayaEdgeCreaseIndices.length() * 2);
      for(uint32_t i = 0; i < numCreaseIndices; ++i)
      {
        int2 edge;
        mesh.getEdgeVertices(mayaEdgeCreaseIndices[i], edge);
        mayaCreaseIndices[2 * i    ] = edge[0];
        mayaCreaseIndices[2 * i + 1] = edge[1];
      }

      const uint32_t numMayaCreaseIndices = mayaCreaseIndices.length();
      if(numMayaCreaseIndices != numCreaseIndices)
      {
        result |= kCreaseIndices;
      }
      else
      if(numMayaCreaseIndices && !usd::utils::compareArray((const int32_t*)&mayaCreaseIndices[0], creasesIndices.cdata(), numMayaCreaseIndices, numCreaseIndices))
      {
        result |= kCreaseIndices;
      }
    }

    if(exportMask & kCreaseWeights)
    {
      VtArray<float> creasesWeights;
      UsdAttribute creasesAttr = geom.GetCreaseSharpnessesAttr();
      creasesAttr.Get(&creasesWeights, timeCode);

      const uint32_t numCreaseWeights = creasesWeights.size();
      const uint32_t numMayaCreaseWeights = mayaCreaseWeights.length();
      if(numMayaCreaseWeights != numCreaseWeights)
      {
        result |= kCreaseWeights;
      }
      else
      if(numMayaCreaseWeights && !usd::utils::compareArray(&mayaCreaseWeights[0], creasesWeights.cdata(), numMayaCreaseWeights, numCreaseWeights))
      {
        result |= kCreaseWeights;
      }
    }
  }

  //
  if(exportMask & (kCornerIndices | kCornerSharpness))
  {
    UsdAttribute cornerIndices = geom.GetCornerIndicesAttr();
    UsdAttribute cornerSharpness = geom.GetCornerSharpnessesAttr();

    VtArray<int32_t> vertexIdValues;
    VtArray<float> creaseValues;
    cornerIndices.Get(&vertexIdValues);
    cornerSharpness.Get(&creaseValues);

    MUintArray mayaVertexIdValues;
    MDoubleArray mayaCreaseValues;
    mesh.getCreaseVertices(mayaVertexIdValues, mayaCreaseValues);

    const uint32_t numVertexIds = vertexIdValues.size();
    const uint32_t numMayaVertexIds = mayaVertexIdValues.length();

    if(numVertexIds != numMayaVertexIds)
    {
      result |= kCornerIndices;
    }
    else
    {
      if(numMayaVertexIds && !usd::utils::compareArray((const int32_t*)&mayaVertexIdValues[0], vertexIdValues.cdata(), numVertexIds, numMayaVertexIds))
      {
        result |= kCornerIndices;
      }
    }

    const uint32_t numCreaseValues = creaseValues.size();
    const uint32_t numMayaCreaseValues = mayaCreaseValues.length();
    if(numCreaseValues != numMayaCreaseValues)
    {
      result |= kCornerSharpness;
    }
    else
    {
      if(numMayaCreaseValues && !usd::utils::compareArray(&mayaCreaseValues[0], creaseValues.cdata(), numCreaseValues, numMayaCreaseValues))
      {
        result |= kCornerSharpness;
      }
    }
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------
MStringArray hasNewColourSet(UsdGeomMesh& geom, MFnMesh& mesh, PrimVarDiffReport& report)
{
  const std::vector<UsdGeomPrimvar> primvars = geom.GetPrimvars();

  MStringArray existingSetNames;
  std::vector<UsdGeomPrimvar> existingPrimVars;

  MStringArray setNames;
  mesh.getColorSetNames(setNames);
  for(int i = 0; i < setNames.length(); )
  {
    for(auto it = primvars.begin(), end = primvars.end(); it != end; ++it)
    {
      const UsdGeomPrimvar& primvar = *it;
      TfToken name, interpolation;
      SdfValueTypeName typeName;
      int elementSize;
      primvar.GetDeclarationInfo(&name, &typeName, &interpolation, &elementSize);
      if(name.GetString() == setNames[i].asChar())
      {
        existingSetNames.append(setNames[i]);
        existingPrimVars.push_back(primvar);
        setNames.remove(i);
        continue;
      }
    }
    ++i;
  }

  for(uint32_t i = 0; i < existingSetNames.length(); ++i)
  {
    MColorArray colours;
    mesh.getColors(colours, &existingSetNames[i]);

    const UsdGeomPrimvar& primvar = existingPrimVars[i];
    TfToken name, interpolation;
    SdfValueTypeName typeName;
    int elementSize;
    primvar.GetDeclarationInfo(&name, &typeName, &interpolation, &elementSize);

    VtValue vtValue;
    if (primvar.Get(&vtValue, UsdTimeCode::Default()))
    {
      if(interpolation == UsdGeomTokens->constant)
      {
        MGlobal::displayError("\"constant\" colour set data currently unsupported");
      }
      else
      if(interpolation == UsdGeomTokens->uniform)
      {
        MGlobal::displayError("\"uniform\" colour set data currently unsupported");
      }
      else
      if(interpolation == UsdGeomTokens->varying)
      {
        MGlobal::displayError("\"varying\" colour set data currently unsupported");
      }
      else
      if(interpolation == UsdGeomTokens->vertex)
      {
        MGlobal::displayError("\"vertex\" colour set data currently unsupported");
      }
      else
      if(interpolation == UsdGeomTokens->faceVarying)
      {
        bool col_indices_have_changed = false;
        if(primvar.IsIndexed())
        {
          VtIntArray usdindices;
          primvar.GetIndices(&usdindices);

          const uint32_t numColourIndices = usdindices.size();
          const uint32_t numMayaColourIndices = mesh.numFaceVertices();

          // resize to the correct size
          std::vector<int32_t> colourIndices(mesh.numFaceVertices());

          MItMeshFaceVertex iter(mesh.object());
          for(size_t j = 0; !iter.isDone(); iter.next(), ++j)
          {
            mesh.getColorIndex(iter.faceId(), iter.vertId(), colourIndices[j], &existingSetNames[i]);
          }

          if(!usd::utils::compareArray(colourIndices.data(), usdindices.cdata(), numMayaColourIndices, numColourIndices))
          {
            col_indices_have_changed = true;
          }
        }

        if (vtValue.IsHolding<VtArray<GfVec4f> >())
        {
          const VtArray<GfVec4f> rawVal = vtValue.Get<VtArray<GfVec4f> >();
          const uint32_t numColours = rawVal.size() * 4;
          const uint32_t numMayaColours = colours.length() * 4;
          if(numMayaColours != numColours)
          {
            report.emplace_back(primvar, existingSetNames[i], true, col_indices_have_changed, true);
          }
          else
          if(numMayaColours && !usd::utils::compareArray((const float*)rawVal.cdata(), (const float*)&colours[0], numColours, numMayaColours))
          {
            report.emplace_back(primvar, existingSetNames[i], true, col_indices_have_changed, true);
          }
        }
        else
        if (vtValue.IsHolding<VtArray<GfVec3f> >())
        {
          const VtArray<GfVec3f> rawVal = vtValue.Get<VtArray<GfVec3f> >();
          const uint32_t numColours = rawVal.size();
          const uint32_t numMayaColours = colours.length();
          if(numMayaColours != numColours)
          {
            report.emplace_back(primvar, existingSetNames[i], true, col_indices_have_changed, true);
          }
          else
          if(numMayaColours && !usd::utils::compareArray3Dto4D((const float*)rawVal.cdata(), (const float*)&colours[0], numColours, numMayaColours))
          {
            report.emplace_back(primvar, existingSetNames[i], true, col_indices_have_changed, true);
          }
        }
      }
    }
  }
  return setNames;
}

//----------------------------------------------------------------------------------------------------------------------
MStringArray hasNewUvSet(UsdGeomMesh& geom, const MFnMesh& mesh, PrimVarDiffReport& report)
{
  const std::vector<UsdGeomPrimvar> primvars = geom.GetPrimvars();

  MStringArray existingSetNames;
  MStringArray setNames;
  mesh.getUVSetNames(setNames);
  std::vector<UsdGeomPrimvar> existingPrimVars;

  for(int i = 0; i < setNames.length(); )
  {
    for(auto it = primvars.begin(), end = primvars.end(); it != end; ++it)
    {
      const UsdGeomPrimvar& primvar = *it;
      TfToken name, interpolation;
      SdfValueTypeName typeName;
      int elementSize;
      primvar.GetDeclarationInfo(&name, &typeName, &interpolation, &elementSize);
      MString uvSetName = setNames[i];
      if (uvSetName == "map1")
      {
        uvSetName = "st";
      }

      if(name.GetString() == uvSetName.asChar())
      {
        existingSetNames.append(setNames[i]);
        existingPrimVars.push_back(primvar);
        setNames.remove(i);
        continue;
      }
    }
    ++i;
  }

  for(uint32_t i = 0; i < existingSetNames.length(); ++i)
  {
    MFloatArray u;
    MFloatArray v;
    mesh.getUVs(u, v, &existingSetNames[i]);

    const UsdGeomPrimvar& primvar = existingPrimVars[i];
    TfToken name, interpolation;
    SdfValueTypeName typeName;
    int elementSize;
    primvar.GetDeclarationInfo(&name, &typeName, &interpolation, &elementSize);

    if(interpolation == UsdGeomTokens->constant)
    {
      VtValue vtValue;
      if (primvar.Get(&vtValue, UsdTimeCode::Default()))
      {
        if (vtValue.IsHolding<VtArray<GfVec2f> >())
        {
          bool uvs_have_changed = false;
          const VtArray<GfVec2f> rawVal = vtValue.Get<VtArray<GfVec2f> >();
          if(rawVal.size() != 1)
          {
            uvs_have_changed = true;
          }
          else
          if(!usd::utils::compareUvArray(rawVal[0][0], rawVal[0][1], (const float*)&u[0], (const float*)&v[0], u.length()))
          {
            uvs_have_changed = true;
          }
          if(uvs_have_changed)
          {
            report.emplace_back(primvar, existingSetNames[i], false, true, uvs_have_changed);
          }
        }
      }
    }
    else
    if(interpolation == UsdGeomTokens->uniform)
    {
      MIntArray uvCounts, mayaUvIndices;
      if(mesh.getAssignedUVs(uvCounts, mayaUvIndices, &existingSetNames[i]))
      {
        bool indices_modified = false;
        for(uint32_t i = 0, j = 0, npolys = uvCounts.length(); !indices_modified && i < npolys; ++i)
        {
          int index = mayaUvIndices[j];
          int nverts = uvCounts[i];
          for(uint32_t k = 1; k < nverts; ++k)
          {
            if(index != mayaUvIndices[j + k])
            {
              indices_modified = true;
              break;
            }
          }
          j += nverts;
        }

        if(indices_modified)
        {
          report.emplace_back(primvar, existingSetNames[i], false, true, true);
        }
        else
        {
          VtValue vtValue;
          if (primvar.Get(&vtValue, UsdTimeCode::Default()))
          {
            const VtArray<GfVec2f> rawVal = vtValue.Get<VtArray<GfVec2f> >();
            const uint32_t numUVs = rawVal.size();
            const uint32_t numMayaUVs = u.length();
            if(numMayaUVs != numUVs)
            {
              report.emplace_back(primvar, existingSetNames[i], false, false, true);
            }
            else
            if(numMayaUVs && !usd::utils::compareUvArray((const float*)&u[0], (const float*)&v[0], (const float*)rawVal.cdata(), numUVs, numMayaUVs))
            {
              report.emplace_back(primvar, existingSetNames[i], false, false, true);
            }
          }
        }
      }
    }
    else
    if(interpolation == UsdGeomTokens->varying)
    {
    }
    else
    if(interpolation == UsdGeomTokens->vertex)
    {
    }
    else
    if(interpolation == UsdGeomTokens->faceVarying)
    {
      VtValue vtValue;
      if (primvar.Get(&vtValue, UsdTimeCode::Default()))
      {
        if (vtValue.IsHolding<VtArray<GfVec2f> >())
        {
          bool uvs_have_changed = false;
          bool uv_indices_have_changed = false;

          const VtArray<GfVec2f> rawVal = vtValue.Get<VtArray<GfVec2f> >();
          const uint32_t numUVs = rawVal.size();
          const uint32_t numMayaUVs = u.length();
          if(numMayaUVs != numUVs)
          {
            uvs_have_changed = true;
          }
          else
          if(numMayaUVs && !usd::utils::compareUvArray((const float*)&u[0], (const float*)&v[0], (const float*)rawVal.cdata(), numUVs, numMayaUVs))
          {
            uvs_have_changed = true;
          }

          MIntArray uvCounts, mayaUvIndices;
          if(mesh.getAssignedUVs(uvCounts, mayaUvIndices, &existingSetNames[i]))
          {
            VtIntArray usdindices;
            primvar.GetIndices(&usdindices);

            const uint32_t numUvIndices = usdindices.size();
            const uint32_t numMayaUvIndices = mayaUvIndices.length();
            if(numMayaUvIndices != numUvIndices)
            {
              uv_indices_have_changed = true;
            }
            else
            if(numMayaUvIndices && !usd::utils::compareArray((const int32_t*)&mayaUvIndices[0], usdindices.cdata(), numMayaUvIndices, numUvIndices))
            {
              uv_indices_have_changed = true;
            }
          }

          if(uvs_have_changed || uv_indices_have_changed)
          {
            report.emplace_back(primvar, existingSetNames[i], false, uv_indices_have_changed, uvs_have_changed);
          }
        }
      }
    }
  }
  return setNames;
}

//----------------------------------------------------------------------------------------------------------------------
} // utils
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
