//
// Copyright 2016 Pixar
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
#include "ModelAPI.h"

#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/schemaRegistry.h>
#include <pxr/usd/usd/tokens.h>
#include <pxr/usd/usd/typed.h>

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType) { TfType::Define<AL_usd_ModelAPI, TfType::Bases<UsdModelAPI>>(); }

/* virtual */
AL_usd_ModelAPI::~AL_usd_ModelAPI() { }

/* static */
AL_usd_ModelAPI AL_usd_ModelAPI::Get(const UsdStagePtr& stage, const SdfPath& path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return AL_usd_ModelAPI();
    }
    return AL_usd_ModelAPI(stage->GetPrimAtPath(path));
}

#if PXR_VERSION >= 2108
/* virtual */
UsdSchemaKind AL_usd_ModelAPI::_GetSchemaKind() const { return AL_usd_ModelAPI::schemaKind; }
#else
/* virtual */
UsdSchemaType AL_usd_ModelAPI::_GetSchemaType() const { return AL_usd_ModelAPI::schemaType; }
#endif

/* static */
const TfType& AL_usd_ModelAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<AL_usd_ModelAPI>();
    return tfType;
}

/* static */
bool AL_usd_ModelAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType& AL_usd_ModelAPI::_GetTfType() const { return _GetStaticTfType(); }

/*static*/
const TfTokenVector& AL_usd_ModelAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames = UsdModelAPI::GetSchemaAttributeNames(true);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

PXR_NAMESPACE_OPEN_SCOPE

void AL_usd_ModelAPI::SetSelectability(const TfToken& selectability)
{
    if (!GetPrim().IsValid()) {
        return;
    }

    if (selectability == AL_USDMayaSchemasTokens->selectability_selectable) {
        GetPrim().SetMetadata(
            AL_USDMayaSchemasTokens->selectability,
            AL_USDMayaSchemasTokens->selectability_selectable);
    } else if (selectability == AL_USDMayaSchemasTokens->selectability_unselectable) {
        GetPrim().SetMetadata(
            AL_USDMayaSchemasTokens->selectability,
            AL_USDMayaSchemasTokens->selectability_unselectable);
    } else if (selectability == AL_USDMayaSchemasTokens->selectability_inherited) {
        GetPrim().SetMetadata(
            AL_USDMayaSchemasTokens->selectability,
            AL_USDMayaSchemasTokens->selectability_inherited);
    }
}

TfToken AL_usd_ModelAPI::ComputeHierarchical(const UsdPrim& prim, const ComputeLogic& logic) const
{
    TfToken value;
    bool    keepLooking = logic(prim, value);

    if (keepLooking) {
        if (UsdPrim parent = prim.GetParent()) {
            return ComputeHierarchical(parent, logic);
        }
    }

    return value;
}

TfToken AL_usd_ModelAPI::ComputeSelectabilty() const
{
    if (!GetPrim().IsValid()) {
        return TfToken();
    }

    ComputeLogic determineSelectability = [](const UsdPrim& prim, TfToken& outValue) {
        AL_usd_ModelAPI modelApi(prim);
        if (modelApi.GetSelectability() == AL_USDMayaSchemasTokens->selectability_unselectable) {
            outValue = AL_USDMayaSchemasTokens->selectability_unselectable;
            return false;
        }

        outValue = AL_USDMayaSchemasTokens->selectability_inherited;
        return true;
    };

    TfToken foundValue = ComputeHierarchical(GetPrim(), determineSelectability);

    return foundValue;
}

TfToken AL_usd_ModelAPI::GetSelectability() const
{
    if (!GetPrim().IsValid()) {
        return TfToken();
    }

    if (!GetPrim().HasMetadata(AL_USDMayaSchemasTokens->selectability)) {
        return AL_USDMayaSchemasTokens->selectability_inherited;
    }
    TfToken selectabilityValue;
    GetPrim().GetMetadata<TfToken>(AL_USDMayaSchemasTokens->selectability, &selectabilityValue);
    return selectabilityValue;
}

void AL_usd_ModelAPI::SetLock(const TfToken& lock)
{
    if (!GetPrim()) {
        return;
    }
    if (lock == AL_USDMayaSchemasTokens->lock_transform) {
        GetPrim().SetMetadata(
            AL_USDMayaSchemasTokens->lock, AL_USDMayaSchemasTokens->lock_transform);
    } else if (lock == AL_USDMayaSchemasTokens->lock_inherited) {
        GetPrim().SetMetadata(
            AL_USDMayaSchemasTokens->lock, AL_USDMayaSchemasTokens->lock_inherited);
    } else if (lock == AL_USDMayaSchemasTokens->lock_unlocked) {
        GetPrim().SetMetadata(
            AL_USDMayaSchemasTokens->lock, AL_USDMayaSchemasTokens->lock_unlocked);
    }
}

TfToken AL_usd_ModelAPI::GetLock() const
{
    if (!GetPrim())
        return TfToken();
    if (!GetPrim().HasMetadata(AL_USDMayaSchemasTokens->lock)) {
        return AL_USDMayaSchemasTokens->lock_inherited;
    }
    TfToken lockValue;
    GetPrim().GetMetadata<TfToken>(AL_USDMayaSchemasTokens->lock, &lockValue);
    return lockValue;
}

TfToken AL_usd_ModelAPI::ComputeLock() const
{
    if (!GetPrim())
        return TfToken();
    ComputeLogic determineLock = [](const UsdPrim& prim, TfToken& outValue) {
        if (!prim.HasMetadata(AL_USDMayaSchemasTokens->lock)) {
            outValue = AL_USDMayaSchemasTokens->lock_inherited;
            return true;
        }
        prim.GetMetadata<TfToken>(AL_USDMayaSchemasTokens->lock, &outValue);
        if (outValue != AL_USDMayaSchemasTokens->lock_inherited) {
            return false;
        }
        return true;
    };

    TfToken foundValue = ComputeHierarchical(GetPrim(), determineLock);
    return foundValue;
}

PXR_NAMESPACE_CLOSE_SCOPE
