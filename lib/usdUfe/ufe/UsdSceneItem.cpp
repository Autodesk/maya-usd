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

#ifdef UFE_SCENEITEM_HAS_METADATA
#include "Utils.h"

#include <usdUfe/ufe/UsdUndoClearSceneItemMetadataCommand.h>
#include <usdUfe/ufe/UsdUndoSetSceneItemMetadataCommand.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/base/vt/value.h>

#endif // UFE_SCENEITEM_HAS_METADATA

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

#ifdef UFE_SCENEITEM_HAS_METADATA

Ufe::Value UsdSceneItem::getMetadata(const std::string& key) const
{
    PXR_NS::VtValue data = prim().GetCustomDataByKey(PXR_NS::TfToken(key));
    if (data.IsEmpty()) {
        return Ufe::Value();
    }

    return vtValueToUfeValue(data);
}

Ufe::UndoableCommandPtr
UsdSceneItem::setMetadataCmd(const std::string& key, const Ufe::Value& value)
{
    return std::make_shared<SetSceneItemMetadataCommand>(prim(), key, value);
}

Ufe::UndoableCommandPtr UsdSceneItem::clearMetadataCmd(const std::string& key)
{
    return std::make_shared<ClearSceneItemMetadataCommand>(prim(), "", key);
}

Ufe::Value UsdSceneItem::getGroupMetadata(const std::string& group, const std::string& key) const
{
    PXR_NS::VtValue data = prim().GetCustomDataByKey(PXR_NS::TfToken(group));
    if (data.IsEmpty()) {
        return Ufe::Value();
    }

    if (!data.IsHolding<PXR_NS::VtDictionary>()) {
        return Ufe::Value();
    }

    PXR_NS::VtValue value;
    if (TfMapLookup(data.UncheckedGet<PXR_NS::VtDictionary>(), key, &value)) {
        return vtValueToUfeValue(value);
    }

    return Ufe::Value();
}

Ufe::UndoableCommandPtr UsdSceneItem::setGroupMetadataCmd(
    const std::string& group,
    const std::string& key,
    const Ufe::Value&  value)
{
    return std::make_shared<SetSceneItemMetadataCommand>(prim(), group, key, value);
}

Ufe::UndoableCommandPtr
UsdSceneItem::clearGroupMetadataCmd(const std::string& group, const std::string& key)
{
    return std::make_shared<ClearSceneItemMetadataCommand>(prim(), group, key);
}

#endif // UFE_SCENEITEM_HAS_METADATA

} // namespace USDUFE_NS_DEF
