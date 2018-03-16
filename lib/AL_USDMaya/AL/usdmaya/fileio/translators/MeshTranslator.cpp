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
#include "AL/maya/utils/Utils.h"
#include "AL/usdmaya/utils/SIMD.h"
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
#include "maya/MFnCompoundAttribute.h"
#include "maya/MFnNumericAttribute.h"
#include "maya/MFnTypedAttribute.h"
#include "maya/MFnEnumAttribute.h"

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
  const char* alusd_prefix = "alusd_";
  const std::string& str = usdAttr.GetName().GetString();
  if(!std::strncmp(alusd_prefix, str.c_str(), 6))
  {
    return true;
  }

  const char* const glimpse_prefix = "glimpse";
  const std::string& nmsp = usdAttr.GetNamespace();
  if(!std::strncmp(glimpse_prefix, nmsp.c_str(), 7))
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

static void applyGlimpseUserDataParams(const UsdPrim& from, MFnMesh& fnMesh)
{
  // TODO: glimpse user data can be set on any DAG node, push up to DagNodeTranslator?
  // TODO: a schema, similar to that of primvars, should be used
  static const std::string glimpse_namespace("glimpse:userData");

  MStatus status;
  MPlug plug = fnMesh.findPlug("gUserData", &status);
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
  std::vector<UsdProperty> userDataProperties = from.GetPropertiesInNamespace(glimpse_namespace);
  for(auto &userDataProperty : userDataProperties) {
    if (UsdAttribute attr = userDataProperty.As<UsdAttribute>())
    {
      UsdDataType dataType = getAttributeType(attr);
      switch(dataType)
      {
        case UsdDataType::kInt:
          {
            int value;
            attr.Get(&value);

            MPlug elementPlug = plug.elementByLogicalIndex(logicalIndex++);
            applyUserData(elementPlug, attr.GetBaseName().GetString(), GlimpseUserDataTypes::kInt, std::to_string(value));
          }
          break;
        case UsdDataType::kVec2i:
          {
            GfVec2i vec;
            attr.Get(&vec);

            std::stringstream ss;
            ss << vec[0] << ' ' << vec[1];

            MPlug elementPlug = plug.elementByLogicalIndex(logicalIndex++);
            applyUserData(elementPlug, attr.GetBaseName().GetString(), GlimpseUserDataTypes::kInt2, ss.str());
          }
          break;
        case UsdDataType::kVec3i:
          {
            GfVec3i vec;
            attr.Get(&vec);

            std::stringstream ss;
            ss << vec[0] << ' ' << vec[1] << ' ' << vec[2];

            MPlug elementPlug = plug.elementByLogicalIndex(logicalIndex++);
            applyUserData(elementPlug, attr.GetBaseName().GetString(), GlimpseUserDataTypes::kInt3, ss.str());
          }
          break;
        case UsdDataType::kFloat:
          {
            float value;
            attr.Get(&value);

            MPlug elementPlug = plug.elementByLogicalIndex(logicalIndex++);
            applyUserData(elementPlug, attr.GetBaseName().GetString(), GlimpseUserDataTypes::kFloat,
                          std::to_string(value));
          }
          break;
        case UsdDataType::kString:
          {
            std::string value;
            attr.Get(&value);

            MPlug elementPlug = plug.elementByLogicalIndex(logicalIndex++);
            applyUserData(elementPlug, attr.GetBaseName().GetString(), GlimpseUserDataTypes::kString, value);
          }
          break;
        case UsdDataType::kVec3f:
          {
            GfVec3f vec;
            attr.Get(&vec);

            std::stringstream ss;
            ss << vec[0] << ' ' << vec[1] << ' ' << vec[2];

            MPlug elementPlug = plug.elementByLogicalIndex(logicalIndex++);
            applyUserData(elementPlug, attr.GetBaseName().GetString(), GlimpseUserDataTypes::kVector, ss.str());
          }
          break;
        case UsdDataType::kColor3f:
          {
            GfVec3f vec;
            attr.Get(&vec);

            std::stringstream ss;
            ss << vec[0] << ' ' << vec[1] << ' ' << vec[2];

            MPlug elementPlug = plug.elementByLogicalIndex(logicalIndex++);
            applyUserData(elementPlug, attr.GetBaseName().GetString(), GlimpseUserDataTypes::kColor, ss.str());
          }
          break;
        case UsdDataType::kMatrix4d:
          {
            GfMatrix4d matrix;
            attr.Get(&matrix);

            std::stringstream ss;
            ss << matrix[0][0] << ' ' << matrix[0][1] << ' ' << matrix[0][2] << ' ';
            ss << matrix[1][0] << ' ' << matrix[1][1] << ' ' << matrix[1][2] << ' ';
            ss << matrix[2][0] << ' ' << matrix[2][1] << ' ' << matrix[2][2] << ' ';
            ss << matrix[3][0] << ' ' << matrix[3][1] << ' ' << matrix[3][2];

            MPlug elementPlug = plug.elementByLogicalIndex(logicalIndex++);
            applyUserData(elementPlug, attr.GetBaseName().GetString(), GlimpseUserDataTypes::kMatrix, ss.str());
          }
          break;
        default:
          break;
      }
    }
  }

}


static void copyGlimpseUserDataAttibutes(UsdGeomMesh& mesh, const MFnMesh& fnMesh)
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
      for(int i = 0; i < length; i++)
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
      for(int i = 0; i < length; i++)
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
            prim.CreateAttribute(nameToken, SdfValueTypeNames->Int, false).Set(value.asInt());
          }
          break;
        case GlimpseUserDataTypes::kInt2:
          {
            // int2
            if(tokens.length() == 2 && allInts(tokens))
            {
              GfVec2i vec(tokens[0].asInt(), tokens[1].asInt());
              prim.CreateAttribute(nameToken, SdfValueTypeNames->Int2, false).Set(vec);
            }
          }
          break;
        case GlimpseUserDataTypes::kInt3:
          {
            // int3
            if(tokens.length() == 3 && allInts(tokens))
            {
              GfVec3i vec(tokens[0].asInt(), tokens[1].asInt(), tokens[2].asInt());
              prim.CreateAttribute(nameToken, SdfValueTypeNames->Int3, false).Set(vec);
            }
          }
          break;
        case GlimpseUserDataTypes::kFloat:
          {
            // float
            prim.CreateAttribute(nameToken, SdfValueTypeNames->Float, false).Set(value.asFloat());
          }
          break;
        case GlimpseUserDataTypes::kVector:
          {
            // vector
            if(tokens.length() == 3 && allFloats(tokens))
            {
              GfVec3f vec(tokens[0].asFloat(), tokens[1].asFloat(), tokens[2].asFloat());
              prim.CreateAttribute(nameToken, SdfValueTypeNames->Vector3f, false).Set(vec);
            }
          }
          break;
        case GlimpseUserDataTypes::kColor:
          {
            // color
            if(tokens.length() == 3 && allFloats(tokens))
            {
              GfVec3f vec(tokens[0].asFloat(), tokens[1].asFloat(), tokens[2].asFloat());
              prim.CreateAttribute(nameToken, SdfValueTypeNames->Color3f, false).Set(vec);
            }
          }
          break;
        case GlimpseUserDataTypes::kString:
          {
            // string
            prim.CreateAttribute(nameToken, SdfValueTypeNames->String, false).Set(AL::maya::utils::convert(value));
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

  plug = fnMesh.findPlug("gUserData");
  if(status && plug.isCompound() && plug.isArray())
  {
    for(int i = 0; i < plug.numElements(); i++)
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
UsdPrim MeshTranslator::exportObject(UsdStageRefPtr stage, MDagPath path, const SdfPath& usdPath, const ExporterParams& params)
{
  if(!params.m_meshes)
    return UsdPrim();

  UsdTimeCode usdTime;
  UsdGeomMesh mesh = UsdGeomMesh::Define(stage, usdPath);

  MStatus status;
  MFnMesh fnMesh(path, &status);
  AL_MAYA_CHECK_ERROR2(status, MString("unable to attach function set to mesh") + path.fullPathName());
  if(status)
  {
    UsdAttribute pointsAttr = mesh.GetPointsAttr();
    if (params.m_animTranslator && AnimationTranslator::isAnimatedMesh(path))
    {
      params.m_animTranslator->addMesh(path, pointsAttr);
    }

    AL::usdmaya::utils::copyVertexData(fnMesh, pointsAttr);
    AL::usdmaya::utils::copyFaceConnectsAndPolyCounts(mesh, fnMesh);
    AL::usdmaya::utils::copyInvisibleHoles(mesh, fnMesh);
    AL::usdmaya::utils::copyUvSetData(mesh, fnMesh, params.m_leftHandedUV);

    if(params.m_useAnimalSchema)
    {
      AL::usdmaya::utils::copyAnimalFaceColours(mesh, fnMesh);
      AL::usdmaya::utils::copyAnimalCreaseVertices(mesh, fnMesh);
      AL::usdmaya::utils::copyAnimalCreaseEdges(mesh, fnMesh);
      AL::usdmaya::utils::copyGlimpseTesselationAttributes(mesh, fnMesh);
      copyGlimpseUserDataAttibutes(mesh, fnMesh);
    }
    else
    {
      AL::usdmaya::utils::copyColourSetData(mesh, fnMesh);
      AL::usdmaya::utils::copyCreaseVertices(mesh, fnMesh);
      AL::usdmaya::utils::copyCreaseEdges(mesh, fnMesh);
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
  MStatus status;
  MFnMesh fnMesh(path, &status);
  AL_MAYA_CHECK_ERROR2(status, MString("unable to attach function set to mesh") + path.fullPathName());
  if (status)
  {
    UsdGeomMesh mesh(overPrim);
    AL::usdmaya::utils::copyUvSetData(mesh, fnMesh, params.m_leftHandedUV);
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
    for(uint32_t i = 0, k = 0, n = counts.length(); i < n; i++)
    {
      for(uint32_t j = 0, m = counts[i]; j < m; j++, ++k)
      {
        normalsFaceIdsPtr[k] = k;
      }
    }
    if (fnMesh.setFaceVertexNormals(normals, normalsFaceIds, connects) != MS::kSuccess)
    {
    }
   }

  MFnDagNode fnDag(polyShape);
  fnDag.setName(std::string(from.GetName().GetString() + std::string("Shape")).c_str());

  AL::usdmaya::utils::applyHoleFaces(mesh, fnMesh);
  if(!AL::usdmaya::utils::applyVertexCreases(mesh, fnMesh))
  {
    AL::usdmaya::utils::applyAnimalVertexCreases(from, fnMesh);
  }
  if(!AL::usdmaya::utils::applyEdgeCreases(mesh, fnMesh))
  {
    AL::usdmaya::utils::applyAnimalEdgeCreases(from, fnMesh);
  }
  AL::usdmaya::utils::applyGlimpseSubdivParams(from, fnMesh);
  applyGlimpseUserDataParams(from, fnMesh);
  applyDefaultMaterialOnShape(polyShape);
  AL::usdmaya::utils::applyAnimalColourSets(from, fnMesh, counts);
  AL::usdmaya::utils::applyPrimVars(mesh, fnMesh, counts, connects);

  return polyShape;
}

//----------------------------------------------------------------------------------------------------------------------
} // translators
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
