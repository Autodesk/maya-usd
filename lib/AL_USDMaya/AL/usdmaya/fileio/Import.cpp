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
#include "AL/usdmaya/CodeTimings.h"

#include "AL/usdmaya/utils/DgNodeHelper.h"
#include "AL/usdmaya/utils/Utils.h"
#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/Metadata.h"
#include "AL/usdmaya/fileio/Import.h"
#include "AL/usdmaya/fileio/NodeFactory.h"
#include "AL/usdmaya/fileio/SchemaPrims.h"
#include "AL/usdmaya/fileio/TransformIterator.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"

#include "maya/MAnimControl.h"
#include "maya/MArgDatabase.h"
#include "maya/MArgList.h"
#include "maya/MDagModifier.h"
#include "maya/MEulerRotation.h"
#include "maya/MFileIO.h"
#include "maya/MFnCamera.h"
#include "maya/MFnDagNode.h"
#include "maya/MFnTransform.h"
#include "maya/MFnIkJoint.h"
#include "maya/MGlobal.h"
#include "maya/MNodeClass.h"
#include "maya/MPlug.h"
#include "maya/MSelectionList.h"
#include "maya/MSyntax.h"
#include "maya/MStringArray.h"

#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformCommonAPI.h"
#include "pxr/base/gf/transform.h"

#include <sstream>

namespace AL {
namespace usdmaya {
namespace fileio {


AL_MAYA_DEFINE_COMMAND(ImportCommand, AL_usdmaya);

//----------------------------------------------------------------------------------------------------------------------
Import::Import(const ImporterParams& params)
  : m_params(params), m_success(false)
{
  doImport();
}

//----------------------------------------------------------------------------------------------------------------------
Import::~Import()
{
}

//----------------------------------------------------------------------------------------------------------------------
void Import::doImport()
{
  AL::usdmaya::Profiler::clearAll();
  AL_BEGIN_PROFILE_SECTION(doImport);

  translators::TranslatorContextPtr context = translators::TranslatorContext::create(nullptr);
  translators::TranslatorManufacture manufacture(context);

  UsdStageRefPtr stage;
  if(m_params.m_rootLayer)
  {
    //stage = UsdStage::Open(m_params.m_rootLayer, m_params.m_sessionLayer);
  }
  else
  {
    AL_BEGIN_PROFILE_SECTION(OpenStage);
    stage = UsdStage::Open(m_params.m_fileName.asChar(), m_params.m_stageUnloaded ? UsdStage::LoadNone : UsdStage::LoadAll);
    AL_END_PROFILE_SECTION();
  }

  if(stage != UsdStageRefPtr())
  {
    // set timeline range if animation is enabled
    if(m_params.m_animations)
    {
      const char* const timeError = "ALUSDImport: error setting time range";
      const MTime startTimeCode = stage->GetStartTimeCode();
      const MTime endTimeCode = stage->GetEndTimeCode();
      AL_MAYA_CHECK_ERROR2(MAnimControl::setMinTime(startTimeCode), timeError);
      AL_MAYA_CHECK_ERROR2(MAnimControl::setMaxTime(endTimeCode), timeError);
    }

    UsdPrim usdRootPrim = stage->GetDefaultPrim();
    if (!usdRootPrim)
    {
      usdRootPrim = stage->GetPseudoRoot();
    }

    NodeFactory& factory = getNodeFactory();
    factory.setImportParams(&m_params);

    MFnDependencyNode fn;

    fileio::SchemaPrimsUtils utils(manufacture);

    auto createParentTransform = [&](const UsdPrim& prim, TransformIterator& it)
        {
          MObject parent = it.parent();
          const char * transformType = "transform";
          std::string ttype;
          prim.GetMetadata(Metadata::transformType, &ttype);
          if(!ttype.empty()){
            transformType = ttype.c_str();
          }
          TF_DEBUG(ALUSDMAYA_COMMANDS).Msg("Import::doImport::createParentTransform prim=%s transformType=%s\n", prim.GetPath().GetText(), transformType);
          MObject obj = factory.createNode(prim, transformType, parent);
          it.append(obj);
          return obj;
        };

    for(TransformIterator it(stage, m_params.m_parentPath); !it.done(); it.next())
    {
      const UsdPrim& prim = it.prim();
      // fallback in cases where either the node is NOT an assembly, or the attempt to load the
      // assembly failed.
      {

        if(prim.GetTypeName() == "Mesh")
        {
          AL_BEGIN_PROFILE_SECTION(ImportingMesh);
          MObject obj = createParentTransform(prim, it);
          if(m_params.m_meshes)
          {
            MObject mesh = factory.createNode(prim, "mesh", obj);
            MFnTransform fnP(obj);
            fnP.addChild(mesh, MFnTransform::kNextPos, true);
          }
          AL_END_PROFILE_SECTION();
        }
        else
        if(prim.GetTypeName() == "NurbsCurves")
        {
          AL_BEGIN_PROFILE_SECTION(ImportingNurbsCurves);
          MObject obj = createParentTransform(prim, it);
          if(m_params.m_nurbsCurves)
          {
            MObject mesh = factory.createNode(prim, "nurbsCurve", obj);
            MFnTransform fnP(obj);
            fnP.addChild(mesh, MFnTransform::kNextPos, true);
          }
          AL_END_PROFILE_SECTION();
        }
        else
        if(utils.isSchemaPrim(prim))
        {
          AL_BEGIN_PROFILE_SECTION(ImportingSchemaPrim);
          MObject obj = createParentTransform(prim, it);
          MObject created;
          if(!importSchemaPrim(prim, obj, &created))
          {
            MGlobal::displayWarning(MString("Unable to create prim ") + prim.GetPath().GetText());
          }
          AL_END_PROFILE_SECTION();
        }
        else
        {
          AL_BEGIN_PROFILE_SECTION(ImportingTransform);
          MObject obj = createParentTransform(prim, it);
          it.append(obj);
          AL_END_PROFILE_SECTION();
        }
      }
    }

    m_success = true;
  }
  else
  {
    MGlobal::displayError(MString("Unable to open USD file \"") + m_params.m_fileName + "\"");
  }

  AL_END_PROFILE_SECTION();

  std::stringstream strstr;
  strstr << "Breakdown for file: " << m_params.m_fileName << std::endl;
  AL::usdmaya::Profiler::printReport(strstr);
  MGlobal::displayInfo(AL::maya::utils::convert(strstr.str()));
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ImportCommand::doIt(const MArgList& args)
{
  MStatus status;
  MArgDatabase argData(syntax(), args, &status);
  AL_MAYA_CHECK_ERROR(status, "ImportCommand: failed to match arguments");

  // fetch filename and ensure it's valid
  if(!argData.isFlagSet("-f", &status))
  {
    MGlobal::displayError("ImportCommand: \"file\" argument must be set");
    return MS::kFailure;
  }

  AL_MAYA_CHECK_ERROR(argData.getFlagArgument("-f", 0, m_params.m_fileName), "ImportCommand: Unable to fetch \"file\" argument");

  // check for parent path flag. Convert to MDagPath if found.
  if(argData.isFlagSet("-p", &status))
  {
    MString parent;
    AL_MAYA_CHECK_ERROR(argData.getFlagArgument("-p", 0, parent), "ImportCommand: Unable to fetch \"parent\" argument");

    MSelectionList sl, sl2;
    MGlobal::getActiveSelectionList(sl);
    MGlobal::selectByName(parent, MGlobal::kReplaceList);
    MGlobal::getActiveSelectionList(sl2);
    MGlobal::setActiveSelectionList(sl);
    if(sl2.length())
    {
      sl2.getDagPath(0, m_params.m_parentPath);
    }
  }

  if(argData.isFlagSet("-un", &status))
  {
    AL_MAYA_CHECK_ERROR(argData.getFlagArgument("-un", 0, m_params.m_stageUnloaded), "ImportCommand: Unable to fetch \"unloaded\" argument")
  }

  if(argData.isFlagSet("-a", &status))
  {
    AL_MAYA_CHECK_ERROR(argData.getFlagArgument("-a", 0, m_params.m_animations), "ImportCommand: Unable to fetch \"animation\" argument");
  }

  if(argData.isFlagSet("-da", &status))
  {
    AL_MAYA_CHECK_ERROR(argData.getFlagArgument("-da", 0, m_params.m_dynamicAttributes), "ImportCommand: Unable to fetch \"dynamicAttributes\" argument");
  }

  if(argData.isFlagSet("-m", &status))
  {
    AL_MAYA_CHECK_ERROR(argData.getFlagArgument("-m", 0, m_params.m_meshes), "ImportCommand: Unable to fetch \"meshes\" argument");
  }

  if(argData.isFlagSet("-nc", &status))
  {
    AL_MAYA_CHECK_ERROR(argData.getFlagArgument("-nc", 0, m_params.m_nurbsCurves), "ImportCommand: Unable to fetch \"nurbs curves\" argument");
  }

  return redoIt();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ImportCommand::redoIt()
{
  Import importer(m_params);
  return importer ? MS::kSuccess : MS::kFailure;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ImportCommand::undoIt()
{
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MSyntax ImportCommand::createSyntax()
{
  const char* const errorString = "ImportCommand: failed to create syntax";

  MSyntax syntax;
  AL_MAYA_CHECK_ERROR2(syntax.addFlag("-a", "-anim"), errorString);
  AL_MAYA_CHECK_ERROR2(syntax.addFlag("-f", "-file", MSyntax::kString), errorString);
  AL_MAYA_CHECK_ERROR2(syntax.addFlag("-un", "-unloaded", MSyntax::kBoolean), errorString);
  AL_MAYA_CHECK_ERROR2(syntax.addFlag("-p", "-parent", MSyntax::kString), errorString);
  AL_MAYA_CHECK_ERROR2(syntax.addFlag("-da", "-dynamicAttribute", MSyntax::kBoolean), errorString);
  AL_MAYA_CHECK_ERROR2(syntax.addFlag("-m", "-meshes", MSyntax::kBoolean), errorString);
  AL_MAYA_CHECK_ERROR2(syntax.addFlag("-nc", "-nurbsCurves", MSyntax::kBoolean), errorString);
  syntax.makeFlagMultiUse("-arp");
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

//----------------------------------------------------------------------------------------------------------------------
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
