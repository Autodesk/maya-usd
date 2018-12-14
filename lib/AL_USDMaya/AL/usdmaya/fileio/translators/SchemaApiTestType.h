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
#pragma once

#include "../../Api.h"

#include "pxr/pxr.h"
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_USING_DIRECTIVE


namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A type used purely to test the API schema plugins
//----------------------------------------------------------------------------------------------------------------------
#ifndef AL_GENERATING_DOCS
class SchemaApiTestType
  : public UsdAPISchemaBase
{
public:

  static const UsdSchemaType schemaType = UsdSchemaType::SingleApplyAPI;

  explicit SchemaApiTestType(const UsdPrim& prim=UsdPrim())
      : UsdAPISchemaBase(prim)
  {
  }

  explicit SchemaApiTestType(const UsdSchemaBase& schemaObj)
      : UsdAPISchemaBase(schemaObj)
  {
  }

  AL_USDMAYA_PUBLIC
  virtual ~SchemaApiTestType();

  AL_USDMAYA_PUBLIC
  static const TfTokenVector& GetSchemaAttributeNames(bool includeInherited=true);

  AL_USDMAYA_PUBLIC
  static SchemaApiTestType Get(const UsdStagePtr &stage, const SdfPath &path);


  AL_USDMAYA_PUBLIC
  static SchemaApiTestType Apply(const UsdPrim &prim);

protected:
  AL_USDMAYA_PUBLIC
  virtual UsdSchemaType _GetSchemaType() const;

private:
  friend class UsdSchemaRegistry;
  AL_USDMAYA_PUBLIC
  static const TfType &_GetStaticTfType();

  static bool _IsTypedSchema();

  AL_USDMAYA_PUBLIC
  virtual const TfType &_GetTfType() const;


};
#endif

//----------------------------------------------------------------------------------------------------------------------
} // translators
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
