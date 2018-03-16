
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

#include "AL/usdmaya/utils/MeshUtils.h"

#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/fileio/translators/DagNodeTranslator.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
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
MStatus Mesh::import(const UsdPrim& prim, MObject& parent)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("Mesh::import prim=%s\n", prim.GetPath().GetText());

  const AL::usdmaya::nodes::ProxyShape* proxyShape = context()->getProxyShape();

  const UsdGeomMesh mesh(prim);
  TfToken orientation;
  bool leftHanded = (mesh.GetOrientationAttr().Get(&orientation) and orientation == UsdGeomTokens->leftHanded);

  MFnMesh fnMesh;
  MFloatPointArray points;
  MVectorArray normals;
  MIntArray counts, connects;
  AL::usdmaya::utils::gatherFaceConnectsAndVertices(mesh, points, normals, counts, connects, leftHanded);

  MObject polyShape = fnMesh.create(points.length(), counts.length(), points, counts, connects, parent);

  MIntArray normalsFaceIds;
  normalsFaceIds.setLength(connects.length());
  int32_t* normalsFaceIdsPtr = &normalsFaceIds[0];
  if (normals.length() == fnMesh.numFaceVertices())
  {
    for (uint32_t i = 0, k = 0, n = counts.length(); i < n; i++)
    {
      for (uint32_t j = 0, m = counts[i]; j < m; j++, ++k)
      {
        normalsFaceIdsPtr[k] = k;
      }
    }
    if (fnMesh.setFaceVertexNormals(normals, normalsFaceIds, connects) != MS::kSuccess)
    {
    }
  }

  MFnDagNode fnDag(polyShape);
  fnDag.setName(std::string(prim.GetName().GetString() + std::string("Shape")).c_str());

  AL::usdmaya::utils::applyHoleFaces(mesh, fnMesh);
  if(!AL::usdmaya::utils::applyVertexCreases(mesh, fnMesh))
  {
    AL::usdmaya::utils::applyAnimalVertexCreases(prim, fnMesh);
  }
  if(!AL::usdmaya::utils::applyEdgeCreases(mesh, fnMesh))
  {
    AL::usdmaya::utils::applyAnimalEdgeCreases(prim, fnMesh);
  }
  AL::usdmaya::utils::applyGlimpseSubdivParams(prim, fnMesh);

  MObject initialShadingGroup;
  DagNodeTranslator::initialiseDefaultShadingGroup(initialShadingGroup);
  // Apply default material to Shape
  MStatus status;
  MFnSet fn(initialShadingGroup, &status);
  AL_MAYA_CHECK_ERROR(status, "Unable to attach MfnSet to initialShadingGroup");
  fn.addMember(polyShape);

  AL::usdmaya::utils::applyAnimalColourSets(prim, fnMesh, counts);
  AL::usdmaya::utils::applyPrimVars(mesh, fnMesh, counts, connects);
  context()->addExcludedGeometry(prim.GetPath());

  MFnDagNode mayaNode(parent);
  MDagPath mayaDagPath;
  mayaNode.getPath(mayaDagPath);

  context()->insertItem(prim, parent);
  return MStatus::kSuccess;
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

  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MeshTranslator::preTearDown prim=%s\n", prim.GetPath().GetText());
  TranslatorBase::preTearDown(prim);

  if(!prim.IsValid())
  {
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("Mesh::preTearDown prim invalid\n");
    return MS::kFailure;
  }

  // Write the overrides back to the path it was imported at
  MObjectHandle obj;
  context()->getMObject(prim, obj, MFn::kInvalid);

  if(obj.isValid())
  {
    UsdGeomMesh geomPrim(prim);

    MFnDagNode fn(obj.object());
    MDagPath path;
    fn.getPath(path);

    MStatus status;
    MFnMesh fnMesh(path, &status);
    AL_MAYA_CHECK_ERROR2(status, MString("unable to attach function set to mesh") + path.fullPathName());
    if(status)
    {
      UsdAttribute pointsAttr = geomPrim.GetPointsAttr();

      AL::usdmaya::utils::copyVertexData(fnMesh, pointsAttr);
      AL::usdmaya::utils::copyFaceConnectsAndPolyCounts(geomPrim, fnMesh);
      AL::usdmaya::utils::copyInvisibleHoles(geomPrim, fnMesh);
      //TODO: Control the Handness of the UV's
      AL::usdmaya::utils::copyUvSetData(geomPrim, fnMesh, false);

      AL::usdmaya::utils::copyColourSetData(geomPrim, fnMesh);
      AL::usdmaya::utils::copyCreaseVertices(geomPrim, fnMesh);
      AL::usdmaya::utils::copyCreaseEdges(geomPrim, fnMesh);

      //TODO: Control the copying of dynamicAttributes
      DgNodeTranslator::copyDynamicAttributes(obj.object(), prim);
    }
  }
  else
  {
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("Unable to find the corresponding Maya Handle at prim path '%s'\n", prim.GetPath().GetText());
  }

  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------

