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
  unsigned int translatorCount = 0;
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
          if (TranslatorRefPtr ptr = factory->create(context))
            m_translatorsMap.emplace(ptr->getTranslatedType().GetTypeName(), ptr);
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
