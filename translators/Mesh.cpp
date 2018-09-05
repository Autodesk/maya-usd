
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

#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "maya/MFloatPointArray.h"
#include "maya/MVectorArray.h"
#include "maya/MIntArray.h"
#include "maya/MFnMesh.h"
#include "maya/MFnSet.h"
#include "maya/MFileIO.h"
#include "maya/MNodeClass.h"

#include "AL/usdmaya/utils/DiffPrimVar.h"
#include "AL/usdmaya/utils/MeshUtils.h"

#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/fileio/translators/DagNodeTranslator.h"
#include "AL/usdmaya/fileio/AnimationTranslator.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/Metadata.h"
#include "pxr/usd/usdGeom/mesh.h"

#include "Mesh.h"

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

AL_USDMAYA_DEFINE_TRANSLATOR(Mesh, PXR_NS::UsdGeomMesh)

//----------------------------------------------------------------------------------------------------------------------
MStatus Mesh::initialize()
{
  //Initialise all the class plugs
  return MStatus::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus Mesh::import(const UsdPrim& prim, MObject& parent, MObject& createdObj)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("Mesh::import prim=%s\n", prim.GetPath().GetText());

  const UsdGeomMesh mesh(prim);

  TranslatorContextPtr ctx = context();
  UsdTimeCode timeCode = (ctx && ctx->getForceDefaultRead()) ? UsdTimeCode::Default() : UsdTimeCode::EarliestTime();

  bool parentUnmerged = false;
  TfToken val;
  if(prim.GetParent().GetMetadata(AL::usdmaya::Metadata::mergedTransform, &val))
  {
    parentUnmerged = (val == AL::usdmaya::Metadata::unmerged);
  }
  MString dagName = prim.GetName().GetString().c_str();
  if(!parentUnmerged)
  {
    dagName += "Shape";
  }

  AL::usdmaya::utils::MeshImportContext importContext(mesh, parent, dagName, timeCode);
  importContext.applyVertexNormals();
  importContext.applyHoleFaces();
  importContext.applyVertexCreases();
  importContext.applyEdgeCreases();
  importContext.applyGlimpseSubdivParams();

  MObject initialShadingGroup;
  DagNodeTranslator::initialiseDefaultShadingGroup(initialShadingGroup);
  // Apply default material to Shape
  MStatus status;
  MFnSet fn(initialShadingGroup, &status);
  AL_MAYA_CHECK_ERROR(status, "Unable to attach MfnSet to initialShadingGroup");
  
  createdObj = importContext.getPolyShape();
  fn.addMember(createdObj);
  importContext.applyPrimVars();

  if (ctx)
  {
    ctx->addExcludedGeometry(prim.GetPath());
    ctx->insertItem(prim, createdObj);
  }
  return MStatus::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
UsdPrim Mesh::exportObject(UsdStageRefPtr stage, MDagPath dagPath, const SdfPath& usdPath, const ExporterParams& params)
{
  if(!params.m_meshes)
    return UsdPrim();

  UsdGeomMesh mesh = UsdGeomMesh::Define(stage, usdPath);

  MStatus status;
  MFnMesh fnMesh(dagPath, &status);
  AL_MAYA_CHECK_ERROR2(status, MString("unable to attach function set to mesh") + dagPath.fullPathName());
  if(status)
  {
    UsdAttribute pointsAttr = mesh.GetPointsAttr();
    if (params.m_animTranslator && AnimationTranslator::isAnimatedMesh(dagPath))
    {
      params.m_animTranslator->addMesh(dagPath, pointsAttr);
    }
    uint32_t options = 0;
    if(params.m_dynamicAttributes)
    {
      options |= kDynamicAttributes;
    }
    writeEdits(dagPath, mesh, options);
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
MStatus Mesh::update(const UsdPrim& path)
{
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus Mesh::preTearDown(UsdPrim& prim)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MeshTranslator::preTearDown prim=%s\n", prim.GetPath().GetText());\
  if(!prim.IsValid())
  {
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("Mesh::preTearDown prim invalid\n");
    return MS::kFailure;
  }
  TranslatorBase::preTearDown(prim);

  /* TODO
   * This block was put in since writeEdits modifies USD and thus triggers the OnObjectsChanged callback which will then tearDown
   * the this Mesh prim. The writeEdits method will then continue attempting to copy maya mesh data to USD but will end up crashing
   * since the maya mesh has now been removed by the tearDown.
   *
   * I have tried turning off the TfNotice but I get the 'Detected usd threading violation. Concurrent changes to layer(s) composed'
   * error.
   *
   * This crash and error seems to be happening mainly when switching out a variant that contains a Mesh, and that Mesh has been
   * force translated into Maya.
   */
  TfNotice::Block block;
  // Write the overrides back to the path it was imported at
  MObjectHandle obj;
  context()->getMObject(prim, obj, MFn::kInvalid);
  if(obj.isValid())
  {
    MFnDagNode fn(obj.object());
    MDagPath path;
    fn.getPath(path);
    MStatus status;
    MFnMesh fnMesh(path, &status);
    AL_MAYA_CHECK_ERROR(status, MString("unable to attach function set to mesh: ") + path.fullPathName());

    UsdGeomMesh geomPrim(prim);
    writeEdits(path, geomPrim, kPerformDiff | kDynamicAttributes);
  }
  else
  {
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("Unable to find the corresponding Maya Handle at prim path '%s'\n", prim.GetPath().GetText());
    return MS::kFailure;
  }
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
void Mesh::writeEdits(MDagPath& dagPath, UsdGeomMesh& geomPrim, uint32_t options)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MeshTranslator::writing edits to prim='%s'\n", geomPrim.GetPath().GetText());
  UsdTimeCode t = UsdTimeCode::Default();
  AL::usdmaya::utils::MeshExportContext context(dagPath, geomPrim, t, options & kPerformDiff);
  if(context)
  {
    context.copyVertexData(t);
    context.copyGlimpseTesselationAttributes();
    context.copyNormalData(t);
    context.copyFaceConnectsAndPolyCounts();
    context.copyInvisibleHoles();
    context.copyCreaseVertices();
    context.copyCreaseEdges();
    context.copyUvSetData();
    context.copyColourSetData();
    if(options & kDynamicAttributes)
    {
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

