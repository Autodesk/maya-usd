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
  std::set<TfType> result;
  PlugRegistry::GetAllDerivedTypes<TranslatorBase>(&result);
  if(!result.empty())
  {
    // Manufacture an instance.
    for(auto t: result)
    {
      if(TranslatorFactoryBase* factory = t.GetFactory<TranslatorFactoryBase>())
      {
        TranslatorRefPtr ptr = factory->create(context);
        if (ptr)
        {
          m_translatorsMap.emplace(ptr->getTranslatedType().GetTypeName(), ptr);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
TranslatorRefPtr TranslatorManufacture::get(const TfToken type_name)
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
