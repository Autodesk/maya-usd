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
#include "AL/usdmaya/fileio/AnimationTranslator.h"
#include "AL/usdmaya/fileio/Export.h"
#include "AL/usdmaya/fileio/NodeFactory.h"
#include "AL/usdmaya/fileio/translators/CameraTranslator.h"
#include "AL/usdmaya/fileio/translators/MeshTranslator.h"
#include "AL/usdmaya/fileio/translators/NurbsCurveTranslator.h"
#include "AL/usdmaya/fileio/translators/TransformTranslator.h"
#include "AL/usdmaya/TransformOperation.h"

#include "maya/MAnimControl.h"
#include "maya/MAnimUtil.h"
#include "maya/MArgDatabase.h"
#include "maya/MDagPath.h"
#include "maya/MFnCamera.h"
#include "maya/MFnTransform.h"
#include "maya/MGlobal.h"
#include "maya/MItDag.h"
#include "maya/MSyntax.h"
#include "maya/MNodeClass.h"
#include "maya/MObjectArray.h"
#include "maya/MPlug.h"
#include "maya/MPlugArray.h"
#include "maya/MSelectionList.h"
#include "maya/MUuid.h"

#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformCommonAPI.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/base/gf/transform.h"
#include "pxr/usd/usdGeom/camera.h"

#include <unordered_set>
#include <algorithm>
#include "AL/usdmaya/utils/Utils.h"
#include "AL/maya/utils/MObjectMap.h"
#include <functional>

namespace AL {
namespace usdmaya {
namespace fileio {

AL_MAYA_DEFINE_COMMAND(ExportCommand, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
static inline std::string toString(const MString& str)
{
  return std::string(str.asChar(), str.length());
}

//----------------------------------------------------------------------------------------------------------------------
static unsigned int mayaDagPathToSdfPath(char* dagPathBuffer, unsigned int dagPathSize)
{
  char* writer = dagPathBuffer;
  char* backTrack = writer;

  for (const char *reader = dagPathBuffer, *end = reader + dagPathSize; reader != end; ++reader)
  {
    const char c = *reader;
    switch (c)
    {
      case ':':
        writer = backTrack;
        break;

      case '|':
        *writer = '/';
        ++writer;
        backTrack = writer;
        break;

      default:
        if (reader != writer)
          *writer = c;

        ++writer;
        break;
    }
  }

  return writer - dagPathBuffer;
}

//----------------------------------------------------------------------------------------------------------------------
static inline SdfPath makeUsdPath(const MDagPath& rootPath, const MDagPath& path)
{
  // if the rootPath is empty, we can just use the entire path
  const uint32_t rootPathLength = rootPath.length();
  if(!rootPathLength)
  {
    std::string fpn = toString(path.fullPathName());
    fpn.resize(mayaDagPathToSdfPath(&fpn[0], fpn.size()));
    return SdfPath(fpn);
  }

  // otherwise we need to do a little fiddling.
  MString rootPathString, pathString, newPathString;
  rootPathString = rootPath.fullPathName();
  pathString = path.fullPathName();

  // trim off the root path from the object we are exporting
  newPathString = pathString.substring(rootPathString.length(), pathString.length());

  std::string fpn = toString(newPathString);
  fpn.resize(mayaDagPathToSdfPath(&fpn[0], fpn.size()));
  return SdfPath(fpn);
}

//----------------------------------------------------------------------------------------------------------------------
/// Internal USD exporter implementation
//----------------------------------------------------------------------------------------------------------------------
struct Export::Impl
{
  inline bool contains(const MFnDependencyNode& fn)
  {
    #if AL_UTILS_ENABLE_SIMD
    union
    {
      __m128i sse;
      AL::maya::utils::guid uuid;
    };
    fn.uuid().get(uuid.uuid);
    bool contains = m_nodeMap.find(sse) != m_nodeMap.end();
    if(!contains)
      m_nodeMap.insert(std::make_pair(sse, fn.object()));
    #else
    AL::maya::utils::guid uuid;
    fn.uuid().get(uuid.uuid);
    bool contains = m_nodeMap.find(uuid) != m_nodeMap.end();
    if(!contains)
      m_nodeMap.insert(std::make_pair(uuid, fn.object()));
    #endif
    return contains;
  }

  inline bool contains(const MObject& obj)
  {
    MFnDependencyNode fn(obj);
    return contains(fn);
  }

  inline bool setStage(UsdStageRefPtr ptr)
  {
    m_stage = ptr;
    return m_stage;
  }

  inline UsdStageRefPtr stage()
  {
    return m_stage;
  }

  void setAnimationFrame(double minFrame, double maxFrame)
  {
    m_stage->SetStartTimeCode(minFrame);
    m_stage->SetEndTimeCode(maxFrame);
  }

  void setDefaultPrimIfOnlyOneRoot(SdfPath defaultPrim)
  {
    UsdPrim psuedo = m_stage->GetPseudoRoot();
    auto children = psuedo.GetChildren();
    auto first = children.begin();
    if(first != children.end())
    {
      auto next = first;
      ++next;
      if(next == children.end())
      {
        // if we get here, there is only one prim at the root level.
        // set that prim as the default prim.
        m_stage->SetDefaultPrim(*first);
      }
    }
    if (!m_stage->HasDefaultPrim() and !defaultPrim.IsEmpty())
    {
      m_stage->SetDefaultPrim(m_stage->GetPrimAtPath(defaultPrim));
    }
  }

  void filterSample()
  {
    std::vector<double> timeSamples;
    std::vector<double> dupSamples;
    for (auto prim : m_stage->Traverse())
    {
      std::vector<UsdAttribute> attributes = prim.GetAuthoredAttributes();
      for (auto attr : attributes)
      {
        timeSamples.clear();
        dupSamples.clear();
        attr.GetTimeSamples(&timeSamples);
        VtValue prevSampleBlob;
        for (auto sample : timeSamples)
        {
          VtValue currSampleBlob;
          attr.Get(&currSampleBlob, sample);
          if (prevSampleBlob == currSampleBlob)
          {
            dupSamples.emplace_back(sample);
          }
          else
          {
            prevSampleBlob = currSampleBlob;
            // only clear samples between constant segment
            if (dupSamples.size() > 1)
            {
              dupSamples.pop_back();
              for (auto dup : dupSamples)
              {
                attr.ClearAtTime(dup);
              }
            }
            dupSamples.clear();
          }
        }
        for (auto dup : dupSamples)
        {
          attr.ClearAtTime(dup);
        }
      }
    }
  }

  void doExport(const char* const filename, bool toFilter = false, SdfPath defaultPrim = SdfPath())
  {
    setDefaultPrimIfOnlyOneRoot(defaultPrim);
    if (toFilter)
    {
      filterSample();
    }
    m_stage->Export(filename, false);
    m_nodeMap.clear();
  }

private:
  #if AL_UTILS_ENABLE_SIMD
  std::map<i128, MObject, guid_compare> m_nodeMap;
  #else
  std::map<AL::maya::utils::guid, MObject, AL::maya::utils::guid_compare> m_nodeMap;
  #endif
  UsdStageRefPtr m_stage;
};

static MObject g_transform_rotateAttr = MObject::kNullObj;
static MObject g_transform_translateAttr = MObject::kNullObj;
static MObject g_handle_startJointAttr = MObject::kNullObj;
static MObject g_effector_handleAttr = MObject::kNullObj;
static MObject g_geomConstraint_targetAttr = MObject::kNullObj;

//----------------------------------------------------------------------------------------------------------------------
Export::Export(const ExporterParams& params)
  : m_params(params), m_impl(new Export::Impl)
{
  if(g_transform_rotateAttr == MObject::kNullObj)
  {
    MNodeClass nct("transform");
    MNodeClass nch("ikHandle");
    MNodeClass nce("ikEffector");
    MNodeClass ngc("geometryConstraint");
    g_transform_rotateAttr = nct.attribute("r");
    g_transform_translateAttr = nct.attribute("t");
    g_handle_startJointAttr = nch.attribute("hsj");
    g_effector_handleAttr = nce.attribute("hp");
    g_geomConstraint_targetAttr = ngc.attribute("tg");
  }

  if(m_impl->setStage(UsdStage::CreateNew(m_params.m_fileName.asChar())))
  {
    doExport();
  }
}

//----------------------------------------------------------------------------------------------------------------------
Export::~Export()
{
  delete m_impl;
}

//----------------------------------------------------------------------------------------------------------------------
UsdPrim Export::exportMesh(MDagPath path, const SdfPath& usdPath)
{
  return translators::MeshTranslator::exportObject(m_impl->stage(), path, usdPath, m_params);
}

//----------------------------------------------------------------------------------------------------------------------
UsdPrim Export::exportMeshUV(MDagPath path, const SdfPath& usdPath)
{
  return translators::MeshTranslator::exportUV(m_impl->stage(), path, usdPath, m_params);
}

//----------------------------------------------------------------------------------------------------------------------
UsdPrim Export::exportNurbsCurve(MDagPath path, const SdfPath& usdPath)
{
  return translators::NurbsCurveTranslator::exportObject(m_impl->stage(), path, usdPath, m_params);
}

//----------------------------------------------------------------------------------------------------------------------
UsdPrim Export::exportAssembly(MDagPath path, const SdfPath& usdPath)
{
  UsdGeomXform mesh = UsdGeomXform::Define(m_impl->stage(), usdPath);
  return mesh.GetPrim();
}

//----------------------------------------------------------------------------------------------------------------------
UsdPrim Export::exportPluginLocatorNode(MDagPath path, const SdfPath& usdPath)
{
  UsdGeomXform mesh = UsdGeomXform::Define(m_impl->stage(), usdPath);
  return mesh.GetPrim();
}

//----------------------------------------------------------------------------------------------------------------------
UsdPrim Export::exportPluginShape(MDagPath path, const SdfPath& usdPath)
{
  UsdGeomXform mesh = UsdGeomXform::Define(m_impl->stage(), usdPath);
  return mesh.GetPrim();
}

//----------------------------------------------------------------------------------------------------------------------
UsdPrim Export::exportCamera(MDagPath path, const SdfPath& usdPath)
{
  UsdGeomCamera camera = UsdGeomCamera::Define(m_impl->stage(), usdPath);
  UsdPrim prim = camera.GetPrim();

  MStatus status;
  MFnCamera fnCamera(path, &status);
  AL_MAYA_CHECK_ERROR2(status, "Export: Failed to create cast into a MFnCamera.");

  MObject cameraObject = fnCamera.object(&status);
  AL_MAYA_CHECK_ERROR2(status, "Export: Failed to retrieve object.");
  translators::CameraTranslator::copyAttributes(cameraObject, prim, m_params);

  return camera.GetPrim();
}

//----------------------------------------------------------------------------------------------------------------------
void Export::exportGeometryConstraint(MDagPath constraintPath, const SdfPath& usdPath)
{
  auto animTranslator = m_params.m_animTranslator;
  if(!animTranslator)
  {
    return;
  }

  MPlug plug(constraintPath.node(), g_geomConstraint_targetAttr);
  for(uint32_t i = 0, n = plug.numElements(); i < n; ++i)
  {
    MPlug geom = plug.elementByLogicalIndex(i).child(0);
    MPlugArray connected;
    geom.connectedTo(connected, true, true);
    if(connected.length())
    {
      MPlug inputGeom = connected[0];
      MFnDagNode fn(inputGeom.node());
      MDagPath geomPath;
      fn.getPath(geomPath);
      if(AnimationTranslator::isAnimatedMesh(geomPath))
      {
        auto stage = m_impl->stage();

        // move to the constrained node
        constraintPath.pop();

        SdfPath newPath = usdPath.GetParentPath();

        UsdPrim prim = stage->GetPrimAtPath(newPath);
        if(prim)
        {
          UsdGeomXform xform(prim);
          bool reset;
          std::vector<UsdGeomXformOp> ops = xform.GetOrderedXformOps(&reset);
          for(auto op : ops)
          {
            const TransformOperation thisOp = xformOpToEnum(op.GetBaseName());
            if(thisOp == kTranslate)
            {
              animTranslator->forceAddPlug(MPlug(constraintPath.node(), g_transform_translateAttr), op.GetAttr());
              break;
            }
          }
          return;
        }
        else
        {
          std::cout << "prim not valid " << newPath.GetText() << std::endl;
        }
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void Export::exportIkChain(MDagPath effectorPath, const SdfPath& usdPath)
{
  auto animTranslator = m_params.m_animTranslator;
  if(!animTranslator)
  {
    return;
  }

  MPlug hp(effectorPath.node(), g_effector_handleAttr);
  hp = hp.elementByLogicalIndex(0);
  MPlugArray connected;
  hp.connectedTo(connected, true, true);
  if(connected.length())
  {
    // grab the handle node
    MObject handleObj = connected[0].node();

    MPlug translatePlug(handleObj, g_transform_translateAttr);

    // if the translation values on the ikHandle is animated, then we can assume the rotation values on the
    // joint chain between the effector and the start joint will also be animated
    bool handleIsAnimated = AnimationTranslator::isAnimated(translatePlug, true);
    if(handleIsAnimated)
    {
      // locate the start joint in the chain
      MPlug startJoint(handleObj, g_handle_startJointAttr);
      MPlugArray connected;
      startJoint.connectedTo(connected, true, true);
      if(connected.length())
      {
        // this will be the top chain in the system
        MObject startNode = connected[0].node();

        auto stage = m_impl->stage();
        SdfPath newPath = usdPath;

        UsdPrim prim;
        // now step up from the effector to the start joint and output the rotations
        do
        {
          // no point handling the effector
          effectorPath.pop();
          newPath = newPath.GetParentPath();

          prim = stage->GetPrimAtPath(newPath);
          if(prim)
          {
            UsdGeomXform xform(prim);
            MPlug rotatePlug(effectorPath.node(), g_transform_rotateAttr);
            bool reset;
            std::vector<UsdGeomXformOp> ops = xform.GetOrderedXformOps(&reset);
            for(auto op : ops)
            {
              const float radToDeg = 180.0f / 3.141592654f;
              bool added = false;
              switch(op.GetOpType())
              {
              case UsdGeomXformOp::TypeRotateXYZ:
              case UsdGeomXformOp::TypeRotateXZY:
              case UsdGeomXformOp::TypeRotateYXZ:
              case UsdGeomXformOp::TypeRotateYZX:
              case UsdGeomXformOp::TypeRotateZXY:
              case UsdGeomXformOp::TypeRotateZYX:
                added = true;
                animTranslator->forceAddPlug(rotatePlug, op.GetAttr(), radToDeg);
                break;
              }
              if(added) break;
            }
          }
          else
          {
            std::cout << "prim not valid" << std::endl;
          }
        }
        while(effectorPath.node() != startNode);
      }
    }
  }
  return;
}

//----------------------------------------------------------------------------------------------------------------------
void Export::copyTransformParams(UsdPrim prim, MFnTransform& fnTransform)
{
  translators::TransformTranslator::copyAttributes(fnTransform.object(), prim, m_params);
  if(m_params.m_dynamicAttributes)
  {
    translators::DgNodeTranslator::copyDynamicAttributes(fnTransform.object(), prim);
  }
}

//----------------------------------------------------------------------------------------------------------------------
void Export::exportShapesCommonProc(MDagPath shapePath, MFnTransform& fnTransform, SdfPath& usdPath)
{
  UsdPrim transformPrim;
  if(shapePath.node().hasFn(MFn::kMesh))
  {
    transformPrim = exportMesh(shapePath, usdPath);
  }
  else
  if (shapePath.node().hasFn(MFn::kNurbsCurve))
  {
    transformPrim = exportNurbsCurve(shapePath, usdPath);
  }
  else
  if (shapePath.node().hasFn(MFn::kAssembly))
  {
    transformPrim = exportAssembly(shapePath, usdPath);
  }
  else
  if (shapePath.node().hasFn(MFn::kPluginLocatorNode))
  {
    transformPrim = exportPluginLocatorNode(shapePath, usdPath);
  }
  else
  if (shapePath.node().hasFn(MFn::kPluginShape))
  {
    transformPrim = exportPluginShape(shapePath, usdPath);
  }
  else
  if (shapePath.node().hasFn(MFn::kCamera))
  {
    transformPrim = exportCamera(shapePath, usdPath);
  }

  // if we haven't created a transform for this shape (possible if we chose not to export it)
  // create a transform shape for the prim.
  if (!transformPrim)
  {
    UsdGeomXform xform = UsdGeomXform::Define(m_impl->stage(), usdPath);
    transformPrim = xform.GetPrim();
  }

  copyTransformParams(transformPrim, fnTransform);
}

//----------------------------------------------------------------------------------------------------------------------
void Export::exportShapesOnlyUVProc(MDagPath shapePath, MFnTransform& fnTransform, SdfPath& usdPath)
{
  if(shapePath.node().hasFn(MFn::kMesh))
  {
    exportMeshUV(shapePath, usdPath);
  }
  else
  {
    m_impl->stage()->OverridePrim(usdPath);
  }
}

//----------------------------------------------------------------------------------------------------------------------
void Export::exportSceneHierarchy(MDagPath rootPath, SdfPath& defaultPrim)
{
  MDagPath parentPath = rootPath;
  parentPath.pop();
  static const TfToken xformToken("Xform");

  MItDag it(MItDag::kDepthFirst);
  it.reset(rootPath, MItDag::kDepthFirst, MFn::kTransform);

  std::function<void(MDagPath, MFnTransform&, SdfPath&)> exportShapeProc =
      [this] (MDagPath shapePath, MFnTransform& fnTransform, SdfPath& usdPath)
  {
    this->exportShapesCommonProc(shapePath, fnTransform, usdPath);
  };
  std::function<void(MDagPath, MFnTransform&, SdfPath&)> exportTransformFunc =
      [this] (MDagPath transformPath, MFnTransform& fnTransform, SdfPath& usdPath)
  {
    UsdGeomXform xform = UsdGeomXform::Define(m_impl->stage(), usdPath);
    UsdPrim transformPrim = xform.GetPrim();
    this->copyTransformParams(transformPrim, fnTransform);
  };

  // choose right proc required by meshUV option
  if (m_params.m_meshUV)
  {
    exportShapeProc =
        [this](MDagPath shapePath, MFnTransform& fnTransform, SdfPath& usdPath)
    {
      this->exportShapesOnlyUVProc(shapePath, fnTransform, usdPath);
    };
    exportTransformFunc =
          [this] (MDagPath transformPath, MFnTransform& fnTransform, SdfPath& usdPath)
    {
      m_impl->stage()->OverridePrim(usdPath);
    };
  }

  MFnTransform fnTransform;
  // loop through transforms only
  while(!it.isDone())
  {
    // assign transform function set
    MDagPath transformPath;
    it.getPath(transformPath);

    fnTransform.setObject(transformPath);

    // Make sure we haven't seen this transform before.
    bool transformHasBeenExported = m_impl->contains(fnTransform);
    if(transformHasBeenExported)
    {
      // We have an instanced shape!
      std::cout << "encountered transform instance " << fnTransform.fullPathName().asChar() << std::endl;
    }

    if(!transformHasBeenExported || m_params.m_duplicateInstances)
    {
      // generate a USD path from the current path
      SdfPath usdPath;

      // we should take a look to see whether the name was changed on import.
      // If it did change, make sure we save it using the original name, and not the new one.
      MStatus status;
      MPlug originalNamePlug = fnTransform.findPlug("alusd_originalPath", &status);
      if(!status)
      {
        usdPath = makeUsdPath(parentPath, transformPath);
      }

      if(transformPath.node().hasFn(MFn::kIkEffector))
      {
        exportIkChain(transformPath, usdPath);
      }
      else
      if(transformPath.node().hasFn(MFn::kGeometryConstraint))
      {
        exportGeometryConstraint(transformPath, usdPath);
      }

      // for UV only exporting, record first prim as default
      if (m_params.m_meshUV && defaultPrim.IsEmpty())
      {
        defaultPrim = usdPath;
      }

      if(transformPath.node().hasFn(MFn::kIkEffector))
      {
        exportIkChain(transformPath, usdPath);
      }
      else
      if(transformPath.node().hasFn(MFn::kGeometryConstraint))
      {
        exportGeometryConstraint(transformPath, usdPath);
      }

      // how many shapes are directly under this transform path?
      uint32_t numShapes;
      transformPath.numberOfShapesDirectlyBelow(numShapes);
      if(numShapes)
      {
        // This is a slight annoyance about the way that USD has no concept of
        // shapes (it merges shapes into transforms usually). This means if we have
        // 1 transform, with 4 shapes parented underneath, it means we'll end up with
        // the transform data duplicated four times.

        for(uint32_t j = 0; j < numShapes; ++j)
        {
          MDagPath shapePath = transformPath;
          shapePath.extendToShapeDirectlyBelow(j);

          bool shapeNotYetExported = !m_impl->contains(shapePath.node());
          if(shapeNotYetExported || m_params.m_duplicateInstances)
          {
            // if the path has a child shape, process the shape now
            exportShapeProc(shapePath, fnTransform, usdPath);
          }
          else
          {
            // We have an instanced shape!
            // How do we reference that in USD?
            // What do we do about the additional transform information?

            // Possible answer:
            // We can create the prim and copy all the addition transform information onto the prim.
            // then we can inherit the Master prim.
          }
        }
      }
      else
      {
        exportTransformFunc(transformPath, fnTransform, usdPath);
      }
    }
    else
    {
      // We have an instanced transform
      // How do we reference that here?
    }

    it.next();
  }
}

//----------------------------------------------------------------------------------------------------------------------
void Export::doExport()
{
  // make sure the node factory has been initialised as least once prior to use
  getNodeFactory();

  const MTime oldCurTime = MAnimControl::currentTime();
  if(m_params.m_animTranslator)
  {
    // try to ensure that we have some sort of consistent output for each run by forcing the export to the first frame
    MAnimControl::setCurrentTime(m_params.m_minFrame);
  }

  MObjectArray objects;
  const MSelectionList& sl = m_params.m_nodes;
  SdfPath defaultPrim;
  for(uint32_t i = 0, n = sl.length(); i < n; ++i)
  {
    MDagPath path;
    if(sl.getDagPath(i, path))
    {
      if(path.node().hasFn(MFn::kTransform))
      {
        exportSceneHierarchy(path, defaultPrim);
      }
      else
      if(path.node().hasFn(MFn::kShape))
      {
        path.pop();
        exportSceneHierarchy(path, defaultPrim);
      }
    }
    else
    {
      MObject obj;
      sl.getDependNode(i, obj);
      objects.append(obj);
    }
  }

  if(m_params.m_animTranslator)
  {
    m_params.m_animTranslator->exportAnimation(m_params);
    m_impl->setAnimationFrame(m_params.m_minFrame, m_params.m_maxFrame);

    // return user to their original frame
    MAnimControl::setCurrentTime(oldCurTime);
  }

  m_impl->doExport(m_params.m_fileName.asChar(), m_params.m_filterSample, defaultPrim);
}

//----------------------------------------------------------------------------------------------------------------------
ExportCommand::ExportCommand()
 : m_params()
{
}

//----------------------------------------------------------------------------------------------------------------------
ExportCommand::~ExportCommand()
{
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ExportCommand::doIt(const MArgList& args)
{
  MStatus status;
  MArgDatabase argData(syntax(), args, &status);
  AL_MAYA_CHECK_ERROR(status, "ALUSDExport: failed to match arguments");
  AL_MAYA_COMMAND_HELP(argData, g_helpText);

  // fetch filename and ensure it's valid
  if(!argData.isFlagSet("f", &status))
  {
    MGlobal::displayError("ALUSDExport: \"file\" argument must be set");
    return MS::kFailure;
  }
  AL_MAYA_CHECK_ERROR(argData.getFlagArgument("f", 0, m_params.m_fileName), "ALUSDExport: Unable to fetch \"file\" argument");
  if(argData.isFlagSet("sl", &status))
  {
    AL_MAYA_CHECK_ERROR(argData.getFlagArgument("sl", 0, m_params.m_selected), "ALUSDExport: Unable to fetch \"selected\" argument");
  }
  if(argData.isFlagSet("da", &status))
  {
    AL_MAYA_CHECK_ERROR(argData.getFlagArgument("da", 0, m_params.m_dynamicAttributes), "ALUSDExport: Unable to fetch \"dynamic\" argument");
  }
  if(argData.isFlagSet("di", &status))
  {
    AL_MAYA_CHECK_ERROR(argData.getFlagArgument("di", 0, m_params.m_duplicateInstances), "ALUSDExport: Unable to fetch \"duplicateInstances\" argument");
  }
  if(argData.isFlagSet("m", &status))
  {
    AL_MAYA_CHECK_ERROR(argData.getFlagArgument("m", 0, m_params.m_meshes), "ALUSDExport: Unable to fetch \"meshes\" argument");
  }
  if(argData.isFlagSet("muv", &status))
  {
    AL_MAYA_CHECK_ERROR(argData.getFlagArgument("muv", 0, m_params.m_meshUV), "ALUSDExport: Unable to fetch \"meshUV\" argument");
  }
  if(argData.isFlagSet("luv", &status))
  {
    AL_MAYA_CHECK_ERROR(argData.getFlagArgument("luv", 0, m_params.m_leftHandedUV), "ALUSDExport: Unable to fetch \"m_leftHanded\" argument");
  }
  if(argData.isFlagSet("as", &status))
  {
    AL_MAYA_CHECK_ERROR(argData.getFlagArgument("uas", 0, m_params.m_useAnimalSchema), "ALUSDExport: Unable to fetch \"use animal schema\" argument");
  }
  if(argData.isFlagSet("mt", &status))
  {
    AL_MAYA_CHECK_ERROR(argData.getFlagArgument("mt", 0, m_params.m_mergeTransforms), "ALUSDExport: Unable to fetch \"merge transforms\" argument");
  }
  if(argData.isFlagSet("nc", &status))
  {
    AL_MAYA_CHECK_ERROR(argData.getFlagArgument("nc", 0, m_params.m_nurbsCurves), "ALUSDExport: Unable to fetch \"nurbs curves\" argument");
  }
  if(argData.isFlagSet("fr", &status))
  {
    AL_MAYA_CHECK_ERROR(argData.getFlagArgument("fr", 0, m_params.m_minFrame), "ALUSDExport: Unable to fetch \"frame range\" argument");
    AL_MAYA_CHECK_ERROR(argData.getFlagArgument("fr", 1, m_params.m_maxFrame), "ALUSDExport: Unable to fetch \"frame range\" argument");
    m_params.m_animation = true;
  }
  else
  if(argData.isFlagSet("ani", &status))
  {
    m_params.m_animation = true;
    m_params.m_minFrame = MAnimControl::minTime().value();
    m_params.m_maxFrame = MAnimControl::maxTime().value();
  }
  if (argData.isFlagSet("fs", &status))
  {
    AL_MAYA_CHECK_ERROR(argData.getFlagArgument("fs", 0, m_params.m_filterSample), "ALUSDExport: Unable to fetch \"filter sample\" argument");
  }

  if(m_params.m_animation)
  {
    m_params.m_animTranslator = new AnimationTranslator;
  }
  return redoIt();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ExportCommand::redoIt()
{
  static const std::unordered_set<std::string> ignoredNodes
  {
    "persp",
    "front",
    "top",
    "side"
  };

  if(m_params.m_selected)
  {
    MGlobal::getActiveSelectionList(m_params.m_nodes);
  }
  else
  {
    MDagPath path;
    MItDag it(MItDag::kDepthFirst, MFn::kTransform);
    while(!it.isDone())
    {
      it.getPath(path);
      const MString name = path.partialPathName();
      const std::string s(name.asChar(), name.length());
      if(ignoredNodes.find(s) == ignoredNodes.end())
      {
        m_params.m_nodes.add(path);
      }
      it.prune();
      it.next();
    }
  }

  Export exporter(m_params);
  delete m_params.m_animTranslator;

  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ExportCommand::undoIt()
{
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MSyntax ExportCommand::createSyntax()
{
  const char* const errorString = "ALUSDExport: failed to create syntax";

  MStatus status;
  MSyntax syntax;
  status = syntax.addFlag("-f" , "-file", MSyntax::kString);
  AL_MAYA_CHECK_ERROR2(status, errorString);
  status = syntax.addFlag("-sl" , "-selected", MSyntax::kBoolean);
  AL_MAYA_CHECK_ERROR2(status, errorString);
  status = syntax.addFlag("-da" , "-dynamic", MSyntax::kBoolean);
  AL_MAYA_CHECK_ERROR2(status, errorString);
  status = syntax.addFlag("-m" , "-meshes", MSyntax::kBoolean);
  AL_MAYA_CHECK_ERROR2(status, errorString);
  status = syntax.addFlag("-muv" , "-meshUV", MSyntax::kBoolean);
  AL_MAYA_CHECK_ERROR2(status, errorString);
  status = syntax.addFlag("-luv" , "-leftHandedUV", MSyntax::kBoolean);
  AL_MAYA_CHECK_ERROR2(status, errorString);
  status = syntax.addFlag("-nc" , "-nurbsCurves", MSyntax::kBoolean);
  AL_MAYA_CHECK_ERROR2(status, errorString);
  status = syntax.addFlag("-di" , "-duplicateInstances", MSyntax::kBoolean);
  AL_MAYA_CHECK_ERROR2(status, errorString);
  status = syntax.addFlag("-uas", "-useAnimalSchema", MSyntax::kBoolean);
  AL_MAYA_CHECK_ERROR2(status, errorString);
  status = syntax.addFlag("-mt", "-mergeTransforms", MSyntax::kBoolean);
  AL_MAYA_CHECK_ERROR2(status, errorString);
  status = syntax.addFlag("-ani", "-animation", MSyntax::kNoArg);
  AL_MAYA_CHECK_ERROR2(status, errorString);
  status = syntax.addFlag("-fr", "-frameRange", MSyntax::kDouble, MSyntax::kDouble);
  AL_MAYA_CHECK_ERROR2(status, errorString);
  status = syntax.addFlag("-fs", "-filterSample", MSyntax::kBoolean);
  AL_MAYA_CHECK_ERROR2(status, errorString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);

  return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
const char* const ExportCommand::g_helpText = R"(
ExportCommand Overview:

  This command will export your maya scene into the USD format. If you want the export to happen from 
  a certain point in the hierarchy then select the node in maya and pass the parameter selected=True, otherwise
  it will export from the root of the scene.

  If you want to export keeping the time sampled data, you can do so by passing these flags
    1. AL_usdmaya_ExportCommand -f "<path/to/out/file.usd>" -animation

  Exporting attributes that are dynamic attributes can be done by:
    1. AL_usdmaya_ExportCommand -f "<path/to/out/file.usd>" -dynamic

  Exporting samples over a framerange can be done a few ways:
    1. AL_usdmaya_ExportCommand -f "<path/to/out/file.usd>" -frameRange 0 24
    2. AL_usdmaya_ExportCommand -f "<path/to/out/file.usd>" -ani

  Nurbs curves can be exported by passing the corresponding parameters:
    1. AL_usdmaya_ExportCommand -f "<path/to/out/file.usd>" -nc
  
  The exporter can remove samples that contain the same data for adjacent samples
    1. AL_usdmaya_ExportCommand -f "<path/to/out/file.usd>" -fs
)";

//----------------------------------------------------------------------------------------------------------------------
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
