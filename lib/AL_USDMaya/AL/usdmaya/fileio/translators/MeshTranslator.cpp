//
// Copyright 2017 Animal Logic
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
#include "AL/usd/utils/SIMD.h"
#include "AL/usdmaya/Metadata.h"
#include "AL/usdmaya/utils/Utils.h"
#include "AL/usdmaya/utils/MeshUtils.h"
#include "AL/usdmaya/fileio/ExportParams.h"
#include "AL/usdmaya/fileio/ImportParams.h"
#include "AL/usdmaya/fileio/AnimationTranslator.h"
#include "AL/usdmaya/fileio/translators/MeshTranslator.h"

#include "maya/MAnimUtil.h"
#include "maya/MColorArray.h"
#include "maya/MDagPath.h"
#include "maya/MDoubleArray.h"
#include "maya/MFnMesh.h"
#include "maya/MFloatArray.h"
#include "maya/MFloatPointArray.h"
#include "maya/MGlobal.h"
#include "maya/MIntArray.h"
#include "maya/MItMeshPolygon.h"
#include "maya/MItMeshVertex.h"
#include "maya/MObject.h"
#include "maya/MPlug.h"
#include "maya/MUintArray.h"
#include "maya/MVector.h"
#include "maya/MVectorArray.h"

#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/mesh.h"

#include <algorithm>
#include <cstring>

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {


//----------------------------------------------------------------------------------------------------------------------
bool MeshTranslator::attributeHandled(const UsdAttribute& usdAttr)
{
  const std::string& str = usdAttr.GetName().GetString();
  const char* glimpse_prefix = "glimpse_";
  if(!std::strncmp(glimpse_prefix, str.c_str(), 8))
  {
    return true;
  }

  return DagNodeTranslator::attributeHandled(usdAttr);
}


//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
// Export
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
void MeshTranslator::copyNormalData(const MFnMesh& fnMesh, const UsdAttribute& normalsAttr, UsdTimeCode time)
{
  MStatus status;
  const uint32_t numNormals = fnMesh.numNormals();
  VtArray<GfVec3f> normals(numNormals);
  const float* normalsData = fnMesh.getRawNormals(&status);
  if(status)
  {
    memcpy((GfVec3f*)normals.data(), normalsData, sizeof(float) * 3 * numNormals);
    normalsAttr.Set(normals, time);
  }
  else
  {
    MGlobal::displayError(MString("Unable to access mesh normals on mesh: ") + fnMesh.fullPathName());
  }
}

//----------------------------------------------------------------------------------------------------------------------
UsdPrim MeshTranslator::exportObject(UsdStageRefPtr stage, MDagPath path, const SdfPath& usdPath, const ExporterParams& params)
{
  if(!params.m_meshes)
    return UsdPrim();

  UsdGeomMesh mesh = UsdGeomMesh::Define(stage, usdPath);
  
  AL::usdmaya::utils::MeshExportContext context(path, mesh, params.m_timeCode, false, (AL::usdmaya::utils::MeshExportContext::CompactionLevel)params.m_compactionLevel);
  if(context)
  {
    UsdAttribute pointsAttr = mesh.GetPointsAttr();
    if (params.m_animTranslator && AnimationTranslator::isAnimatedMesh(path))
    {
      params.m_animTranslator->addMesh(path, pointsAttr);
    }

    if(params.m_meshPoints)
    {
      context.copyVertexData(context.timeCode());
    }
    if(params.m_meshConnects)
    {
      context.copyFaceConnectsAndPolyCounts();
    }
    if(params.m_meshHoles)
    {
      context.copyInvisibleHoles();
    }
    if(params.m_meshUvs)
    {
      context.copyUvSetData();
    }
    if(params.m_meshNormals)
    {
      context.copyNormalData(context.timeCode());
    }
    context.copyGlimpseTesselationAttributes();
    if(params.m_meshColours)
    {
      context.copyColourSetData();
    }
    if(params.m_meshVertexCreases)
    {
      context.copyCreaseVertices();
    }
    if(params.m_meshEdgeCreases)
    {
      context.copyCreaseEdges();
    }

    // pick up any additional attributes attached to the mesh node (these will be added alongside the transform attributes)
    if(params.m_dynamicAttributes)
    {
      UsdPrim prim = mesh.GetPrim();
      DgNodeTranslator::copyDynamicAttributes(path.node(), prim);
    }
  }
  return mesh.GetPrim();
}

//----------------------------------------------------------------------------------------------------------------------
UsdPrim MeshTranslator::exportUV(UsdStageRefPtr stage, MDagPath path, const SdfPath& usdPath, const ExporterParams& params)
{
  UsdPrim overPrim = stage->OverridePrim(usdPath);
  UsdGeomMesh mesh(overPrim);
  AL::usdmaya::utils::MeshExportContext context(path, mesh, params.m_timeCode);
  if (context)
  {
    context.copyUvSetData();
  }
  return overPrim;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus MeshTranslator::registerType()
{
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MObject MeshTranslator::createNode(const UsdPrim& from, MObject parent, const char* nodeType, const ImporterParams& params)
{
  if(!params.m_meshes)
    return MObject::kNullObj;

  const UsdGeomMesh mesh(from);
  
  bool parentUnmerged = false;
  TfToken val;
  if(from.GetParent().GetMetadata(AL::usdmaya::Metadata::mergedTransform, &val))
  {
    parentUnmerged = (val == AL::usdmaya::Metadata::unmerged);
  }
  MString dagName = from.GetName().GetString().c_str();
  if(!parentUnmerged)
  {
    dagName += "Shape";
  }
  
  UsdTimeCode timeCode = params.m_forceDefaultRead ? UsdTimeCode::Default() : UsdTimeCode::EarliestTime();
  
  AL::usdmaya::utils::MeshImportContext context(mesh, parent, dagName, timeCode);
  context.applyVertexNormals();
  context.applyHoleFaces();
  context.applyVertexCreases();
  context.applyEdgeCreases();
  context.applyGlimpseSubdivParams();
  context.applyGlimpseUserDataParams();
  applyDefaultMaterialOnShape(context.getPolyShape());
  context.applyPrimVars();
  return context.getPolyShape();
}

//----------------------------------------------------------------------------------------------------------------------
} // translators
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
