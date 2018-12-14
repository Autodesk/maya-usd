//
// Copyright 2018 Animal Logic
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
#include "AL/usdmaya/fileio/translators/SchemaApiTestType.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"


namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<SchemaApiTestType,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (SchemaApiTestType)
);

/* virtual */
SchemaApiTestType::~SchemaApiTestType()
{
}

/* static */
SchemaApiTestType
SchemaApiTestType::Get(const UsdStagePtr &stage, const SdfPath &path)
{
  if (!stage) {
      TF_CODING_ERROR("Invalid stage");
      return SchemaApiTestType();
  }
  return SchemaApiTestType(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaType SchemaApiTestType::_GetSchemaType() const {
  return SchemaApiTestType::schemaType;
}

/* static */
SchemaApiTestType
SchemaApiTestType::Apply(const UsdPrim &prim)
{
  static TfToken usdTypeName("AL::usdmaya::fileio::translators::SchemaApiTestType");
  return UsdAPISchemaBase::_ApplyAPISchema<SchemaApiTestType>(prim, usdTypeName);
}

/* static */
const TfType &
SchemaApiTestType::_GetStaticTfType()
{
  static TfType tfType = TfType::Find<SchemaApiTestType>();
  return tfType;
}

/* static */
bool 
SchemaApiTestType::_IsTypedSchema()
{
  static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
  return isTyped;
}

/* virtual */
const TfType &
SchemaApiTestType::_GetTfType() const
{
  return _GetStaticTfType();
}

} // translators
} // fileio
} // usdmaya
} // AL
