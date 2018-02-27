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
#include "AL/usdmaya/fileio/translators/DagNodeTranslator.h"

#include "maya/MAngle.h"
#include "maya/MDGModifier.h"
#include "maya/MFnDagNode.h"
#include "maya/MFnSet.h"
#include "maya/MGlobal.h"
#include "maya/MObject.h"
#include "maya/MPlug.h"
#include "maya/MNodeClass.h"
#include "maya/MSelectionList.h"
#include "maya/MStatus.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/xformCommonAPI.h"

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {
//----------------------------------------------------------------------------------------------------------------------
MObject DagNodeTranslator::m_visible = MObject::kNullObj;
MObject DagNodeTranslator::m_initialShadingGroup = MObject::kNullObj;

//----------------------------------------------------------------------------------------------------------------------
MStatus DagNodeTranslator::registerType()
{
  const char* const errorString = "Unable to extract attribute for DagNodeTranslator";
  MNodeClass fn("transform");
  MStatus status;

  m_visible = fn.attribute("v", &status);
  AL_MAYA_CHECK_ERROR(status, errorString);

  DagNodeTranslator::initialiseDefaultShadingGroup(m_initialShadingGroup);

  return MS::kSuccess;
}

void DagNodeTranslator::initialiseDefaultShadingGroup(MObject& target)
{
  MSelectionList sl;
  MGlobal::selectByName("initialShadingGroup", MGlobal::kReplaceList);
  MGlobal::getActiveSelectionList(sl);

  MObject shadingGroup;
  sl.getDependNode(0, target);
}

//----------------------------------------------------------------------------------------------------------------------
MObject DagNodeTranslator::createNode(const UsdPrim& from, MObject parent, const char* nodeType, const ImporterParams& params)
{
  MFnDagNode fn;
  MObject to = fn.create(nodeType);

  MStatus status = copyAttributes(from, to, params);
  AL_MAYA_CHECK_ERROR_RETURN_NULL_MOBJECT(status, "Dag node translator: unable t/o get attributes");

  return to;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus DagNodeTranslator::copyAttributes(const UsdPrim& from, MObject to, const ImporterParams& params)
{
  AL_MAYA_CHECK_ERROR2(DgNodeTranslator::copyAttributes(from, to, params), "Errr");

  const UsdGeomXform xformSchema(from);
  return copyBool(to, m_visible, xformSchema.GetVisibilityAttr());
}

//----------------------------------------------------------------------------------------------------------------------
MStatus DagNodeTranslator::applyDefaultMaterialOnShape(MObject shape)
{
  MStatus status;
  MFnSet fn(m_initialShadingGroup, &status);
  AL_MAYA_CHECK_ERROR(status, "Unable to attach MfnSet to initialShadingGroup");
  return fn.addMember(shape);
}

//----------------------------------------------------------------------------------------------------------------------
} // translators
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
