//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
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
#include "AL/usdmaya/fileio/translators/TranslatorTestType.h"

#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/schemaRegistry.h>
#include <pxr/usd/usd/typed.h>

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

//----------------------------------------------------------------------------------------------------------------------
TF_REGISTRY_FUNCTION(TfType) { TfType::Define<TranslatorTestType, TfType::Bases<UsdTyped>>(); }

//----------------------------------------------------------------------------------------------------------------------
TranslatorTestType::~TranslatorTestType() { }

//----------------------------------------------------------------------------------------------------------------------
TranslatorTestType TranslatorTestType::Get(const UsdStagePtr& stage, const SdfPath& path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return TranslatorTestType();
    }
    return TranslatorTestType(stage->GetPrimAtPath(path));
}

//----------------------------------------------------------------------------------------------------------------------
TranslatorTestType TranslatorTestType::Define(const UsdStagePtr& stage, const SdfPath& path)
{
    static TfToken usdPrimTypeName("AL::usdmaya::fileio::translators::TranslatorTestType");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return TranslatorTestType();
    }
    return TranslatorTestType(stage->DefinePrim(path, usdPrimTypeName));
}

//----------------------------------------------------------------------------------------------------------------------
const TfType& TranslatorTestType::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<TranslatorTestType>();
    return tfType;
}

//----------------------------------------------------------------------------------------------------------------------
bool TranslatorTestType::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

//----------------------------------------------------------------------------------------------------------------------
const TfType& TranslatorTestType::_GetTfType() const { return _GetStaticTfType(); }

//----------------------------------------------------------------------------------------------------------------------
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left, const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}

//----------------------------------------------------------------------------------------------------------------------
const TfTokenVector& TranslatorTestType::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {};
    static TfTokenVector allNames
        = _ConcatenateAttributeNames(UsdTyped::GetSchemaAttributeNames(true), localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
