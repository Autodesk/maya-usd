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
#include "AL/usdmaya/Utils.h"
#include "AL/usdmaya/fileio/NodeFactory.h"
#include "AL/usdmaya/fileio/SchemaPrims.h"
#include "AL/usdmaya/fileio/TransformIterator.h"
#include "AL/usdmaya/fileio/translators/TranslatorBase.h"
#include "AL/usdmaya/nodes/ProxyShape.h"

#include "maya/MAnimControl.h"
#include "maya/MDagPath.h"
#include "maya/MDGModifier.h"
#include "maya/MGlobal.h"
#include "maya/MObject.h"
#include "maya/MPlug.h"
#include "maya/MTime.h"
#include "maya/MFileIO.h"
#include "maya/MFnTransform.h"

#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/type.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/schemaBase.h"

namespace AL {
namespace usdmaya {
namespace fileio {

// printf debugging
#if 0 || AL_ENABLE_TRACE
# define Trace(X) std::cout << X << std::endl;
#else
# define Trace(X)
#endif

/// the prim typename tokens
const TfToken ALSchemaType("ALType");
const TfToken ALExcludedPrimSchema("ALExcludedPrim");

//----------------------------------------------------------------------------------------------------------------------
/// \brief  hunt for the camera underneath the specified transform
/// \param  cameraNode the returned camera
/// \param  dagPath  path to the cameras transform node
//----------------------------------------------------------------------------------------------------------------------
void huntForParentCamera(MObject& cameraNode, const MDagPath &dagPath)
{
  MDagPath cameraPath = dagPath;
  cameraPath.pop();
  MFnDagNode cameraXform(cameraPath);
  for(uint32_t i = 0; i < cameraXform.childCount(); ++i)
  {
    if(cameraXform.child(i).hasFn(MFn::kCamera))
    {
      cameraNode = cameraXform.child(i);
      break;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
bool isSchemaOfType(const UsdPrim& prim, const TfToken& typeToken)
{
  if(prim.GetTypeName() == typeToken)
  {
    return true;
  }

  // Check to see if the prim has been tagged with an ALType.
  if(prim.HasCustomDataKey(ALSchemaType))
  {
    VtValue typeValue = prim.GetCustomDataByKey(ALSchemaType);

    // Check to see if the custom dataType matches the typeName passed in
    std::string foundFutureSchemaType = typeValue.Get<std::string>();
    if(foundFutureSchemaType == typeToken)
    {
      return true;
    }
  }

  return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool importSchemaPrim(  const UsdPrim& prim,
                        MObject& parent,
                        MObject* created,
                        translators::TranslatorContextPtr context,
                        const translators::TranslatorRefPtr torBase)
{
  if(torBase)
  {
    Trace("Translator-Import: import prim: " << prim.GetPath().GetText());
    if(torBase->import(prim, parent) != MS::kSuccess)
    {
      std::cerr << "Failed to import schema prim \"" << prim.GetPath().GetText() << "\"\n";
      return false;
    }
  }
  else
  {
    Trace("Failed to find a translator for '" << prim.GetName() << "' type: '" << prim.GetTypeName() << "'");
    return false;
  }
  if(context)
    context->registerItem(prim, parent);
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
SchemaPrimsUtils::SchemaPrimsUtils(fileio::translators::TranslatorManufacture& manufacture)
  : m_manufacture(manufacture)
{
}

//----------------------------------------------------------------------------------------------------------------------
bool SchemaPrimsUtils::needsTransformParent(const UsdPrim& prim)
{
  TfType type = TfType::FindDerivedByName<UsdSchemaBase>(prim.GetTypeName());
  auto translator = m_manufacture.get(TfToken(type.GetTypeName()));
  return translator->needsTransformParent();
}

//----------------------------------------------------------------------------------------------------------------------
bool SchemaPrimsUtils::isSchemaPrim(const UsdPrim& prim)
{
  // the plugin system will return a null pointer if it doesn't know how to
  // translate this prim type
  fileio::translators::TranslatorRefPtr torBase = m_manufacture.get(prim.GetTypeName());
  return torBase;
}


//----------------------------------------------------------------------------------------------------------------------
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
