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
#pragma once
#include "AL/usdmaya/fileio/translators/TranslatorBase.h"
#include "AL/usdmaya/fileio/translators/TranslatorContext.h"

#include "pxr/pxr.h"
#include <pxr/base/tf/token.h>

#include <unordered_set>
#include <string>
#include "AL/maya/utils/ForwardDeclares.h"
#include "AL/usd/utils/ForwardDeclares.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace fileio {

/// the prim typename tokens
extern const TfToken ALSchemaType;
extern const TfToken ALExcludedPrimSchema;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  a method called to import a schema prim into maya
/// \param  usdPrim the usd prim to be imported into Maya
/// \param  parent the parent transform for the prim
/// \param  created the returned MObject of the created node (can be null)
/// \param  context a custom context to use when importing the prim
/// \param  translator the custom translator to use to import the prim
/// \param  forceImport overrides the default import state of the translator and forces and import.
/// \return true if the import succeeded, false otherwise
/// \ingroup   fileio
//----------------------------------------------------------------------------------------------------------------------
bool importSchemaPrim(
    const UsdPrim& usdPrim,
    MObject& parent,
    MObject* created = 0,
    translators::TranslatorContextPtr context = TfNullPtr,
    const translators::TranslatorRefPtr translator = TfNullPtr,
    const fileio::translators::TranslatorParameters& param = fileio::translators::TranslatorParameters());

//----------------------------------------------------------------------------------------------------------------------
/// \brief  utility function to determine whether the prim specified is of the given type
/// \param  prim the prim to query
/// \param  typeToken the type
/// \return true if the specified prim is of the specified type
/// \ingroup   fileio
//----------------------------------------------------------------------------------------------------------------------
bool isSchemaOfType(const UsdPrim& prim, const TfToken& typeToken);

//----------------------------------------------------------------------------------------------------------------------
/// \brief  utility class to determine whether a usd transform chain should be created
/// \ingroup   fileio
//----------------------------------------------------------------------------------------------------------------------
class SchemaPrimsUtils
{
public:

  /// \brief  ctor
  /// \param  manufacture the translator registry
  SchemaPrimsUtils(fileio::translators::TranslatorManufacture& manufacture);

  /// \brief  utility function to determine if a prim is one of our custom schema prims
  /// \param  prim the USD prim to test
  /// \return the corresponding translator of the schema prim
  fileio::translators::TranslatorRefPtr isSchemaPrim(const UsdPrim& prim);

  /// \brief  returns true if the prim specified requires a transform when importing custom nodes into the maya scene
  /// \param  prim the USD prim to check
  /// \return true if the prim requires a parent transform on import, false otherwise
  bool needsTransformParent(const UsdPrim& prim);

private:
  fileio::translators::TranslatorManufacture& m_manufacture;
  //std::unordered_set<std::string> m_nonTransformParentTypes;
};


//----------------------------------------------------------------------------------------------------------------------
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
