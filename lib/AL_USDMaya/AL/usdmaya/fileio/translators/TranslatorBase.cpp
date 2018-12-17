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
#include "AL/usdmaya/fileio/translators/TranslatorBase.h"
#include "AL/usdmaya/DebugCodes.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/type.h"
#include "pxr/usd/usd/schemaBase.h"

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

//----------------------------------------------------------------------------------------------------------------------
TranslatorManufacture::TranslatorManufacture(TranslatorContextPtr context)
{
  std::set<TfType> loadedTypes;
  std::set<TfType> derivedTypes;

  bool keepGoing = true;
  while (keepGoing)
  {
    keepGoing = false;
    derivedTypes.clear();
    PlugRegistry::GetAllDerivedTypes<TranslatorBase>(&derivedTypes);
    for (const TfType& t : derivedTypes)
    {
      const auto insertResult = loadedTypes.insert(t);
      if (insertResult.second)
      {
        // TfType::GetFactory may cause additional plugins to be loaded
        // may means potentially more translator types. We need to re-iterate
        // over the derived types just to be sure...
        keepGoing = true;
        if (auto* factory = t.GetFactory<TranslatorFactoryBase>())
        {
          if (TranslatorRefPtr ptr = factory->create(context))
          {
            m_translatorsMap.emplace(ptr->getTranslatedType().GetTypeName(), ptr);
          }
        }
      }
    }
  }

  derivedTypes.clear();
  PlugRegistry::GetAllDerivedTypes<SchemaPluginBase>(&derivedTypes);
  for (const TfType& t : derivedTypes)
  {
    // TfType::GetFactory may cause additional plugins to be loaded
    // may means potentially more translator types. We need to re-iterate
    // over the derived types just to be sure...
    if (auto* factory = t.GetFactory<SchemaApiTranslatorFactoryBase>())
    {
      if (auto ptr = factory->create(context))
      {
        m_apiPlugins.push_back(ptr);
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
TranslatorManufacture::RefPtr TranslatorManufacture::get(const TfToken type_name)
{

  TfType type = TfType::FindDerivedByName<UsdSchemaBase>(type_name);
  std::string typeName(type.GetTypeName());
  if (m_translatorsMap.find(typeName)!= m_translatorsMap.end())
  {
    return m_translatorsMap[typeName];
  }
  return TfNullPtr;
}

//----------------------------------------------------------------------------------------------------------------------
TranslatorManufacture::RefPtr TranslatorManufacture::get(const MObject& mayaObject)
{
  TranslatorManufacture::RefPtr base;
  TranslatorManufacture::RefPtr derived;
  for(auto& it : m_translatorsMap)
  {
    ExportFlag mode = it.second->canExport(mayaObject);
    switch(mode)
    {
    case ExportFlag::kNotSupported: break;
    case ExportFlag::kFallbackSupport: base = it.second; break;
    case ExportFlag::kSupported: derived = it.second; break;
    default:
      break;
    }
  }
  return derived ? derived : base;
}

//----------------------------------------------------------------------------------------------------------------------
std::vector<TranslatorManufacture::SchemaPluginPtr> TranslatorManufacture::getAPI(const MObject& mayaObject)
{
  std::vector<TranslatorManufacture::SchemaPluginPtr> ptrs;
  for(auto plugin : m_apiPlugins)
  {
    MFn::Type type = plugin->getFnType();
    if(mayaObject.hasFn(type))
    {
      switch(type)
      {
      case MFn::kPluginMotionPathNode:
      case MFn::kPluginDependNode:
      case MFn::kPluginLocatorNode:
      case MFn::kPluginDeformerNode:
      case MFn::kPluginShape:
      case MFn::kPluginFieldNode:
      case MFn::kPluginEmitterNode:
      case MFn::kPluginSpringNode:
      case MFn::kPluginIkSolver:
      case MFn::kPluginHardwareShader:
      case MFn::kPluginHwShaderNode:
      case MFn::kPluginTransformNode:
      case MFn::kPluginObjectSet:
      case MFn::kPluginImagePlaneNode:
      case MFn::kPluginConstraintNode:
      case MFn::kPluginManipulatorNode:
      case MFn::kPluginSkinCluster:
      case MFn::kPluginGeometryFilter:
      case MFn::kPluginBlendShape:
        {
          const MString typeName = plugin->getPluginTypeName();
          MFnDependencyNode fn(mayaObject);
          if(fn.typeName() != typeName)
          {
            continue;
          }
        }
        break;
      default:
        break;
      }
      ptrs.push_back(plugin);
    }
  }
  return ptrs;
}

//----------------------------------------------------------------------------------------------------------------------
TF_REGISTRY_FUNCTION(TfType)
{
  TfType::Define<TranslatorBase>();
}

//----------------------------------------------------------------------------------------------------------------------
} // translators
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
