//
// Copyright 2019 Autodesk
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
#include "UsdSceneItem.h"

#include <pxr/base/tf/type.h>
#include <pxr/pxr.h>
#include <pxr/usd/usd/primTypeInfo.h>
#include <pxr/usd/usd/schemaRegistry.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace USDUFE_NS_DEF {

UsdSceneItem::UsdSceneItem(const Ufe::Path& path, const UsdPrim& prim, int instanceIndex)
    : Ufe::SceneItem(path)
    , fPrim(prim)
    , _instanceIndex(instanceIndex)
{
}

/*static*/
UsdSceneItem::Ptr
UsdSceneItem::create(const Ufe::Path& path, const UsdPrim& prim, int instanceIndex)
{
    return std::make_shared<UsdSceneItem>(path, prim, instanceIndex);
}

//------------------------------------------------------------------------------
// Ufe::SceneItem overrides
//------------------------------------------------------------------------------

std::string UsdSceneItem::nodeType() const { return fPrim ? fPrim.GetTypeName() : std::string(); }

std::vector<std::string> UsdSceneItem::ancestorNodeTypes() const
{
    std::vector<std::string> strAncestorTypes;

    if (!fPrim)
        return strAncestorTypes;

    // Get the actual schema type from the prim definition.
    const TfType& schemaType = fPrim.GetPrimTypeInfo().GetSchemaType();
    if (!schemaType) {
        // No schema type, return empty ancestor types.
        return strAncestorTypes;
    }

    // According to the USD docs GetAllAncestorTypes() is expensive, so we keep a cache.
    static std::map<TfType, std::vector<std::string>> ancestorTypesCache;
    const auto                                        iter = ancestorTypesCache.find(schemaType);
    if (iter != ancestorTypesCache.end()) {
        return iter->second;
    }

    std::vector<TfType> tfAncestorTypes;
    schemaType.GetAllAncestorTypes(&tfAncestorTypes);
    for (const TfType& ty : tfAncestorTypes) {
        // If there is a concrete schema type name, we'll return that since it is what
        // is used/shown in the UI (ex: 'Xform' vs 'UsdGeomXform').
        const auto& schemaReg = UsdSchemaRegistry::GetInstance();
        strAncestorTypes.emplace_back(
            schemaReg.IsConcrete(ty) ? schemaReg.GetSchemaTypeName(ty) : ty.GetTypeName());
    }
    ancestorTypesCache[schemaType] = strAncestorTypes;
    return strAncestorTypes;
}

} // namespace USDUFE_NS_DEF
