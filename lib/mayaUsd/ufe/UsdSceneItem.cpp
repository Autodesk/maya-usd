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
#if USD_VERSION_NUM < 2005
#include <pxr/usd/usd/schemaBase.h>
#else
#include <pxr/usd/usd/primTypeInfo.h>
#endif
#if USD_VERSION_NUM >= 2008
#include <pxr/usd/usd/schemaRegistry.h>
#endif

MAYAUSD_NS_DEF {
namespace ufe {

UsdSceneItem::UsdSceneItem(const Ufe::Path& path, const UsdPrim& prim)
	: Ufe::SceneItem(path)
	, fPrim(prim)
{
}

/*static*/
UsdSceneItem::Ptr UsdSceneItem::create(const Ufe::Path& path, const UsdPrim& prim)
{
	return std::make_shared<UsdSceneItem>(path, prim);
}

//------------------------------------------------------------------------------
// Ufe::SceneItem overrides
//------------------------------------------------------------------------------

std::string UsdSceneItem::nodeType() const
{
	return fPrim.GetTypeName();
}

#if UFE_PREVIEW_VERSION_NUM >= 2020
std::vector<std::string> UsdSceneItem::ancestorNodeTypes() const
{
	std::vector<std::string> strAncestorTypes;

#if USD_VERSION_NUM < 2005
	static const TfType schemaBaseType = TfType::Find<UsdSchemaBase>();
	const TfType schemaType = schemaBaseType.FindDerivedByName(fPrim.GetTypeName().GetString());
#else
	// Get the actual schema type from the prim definition.
	const TfType& schemaType = fPrim.GetPrimTypeInfo().GetSchemaType();
#endif
	if (!schemaType) {
		TF_CODING_ERROR("Could not find prim type '%s' for prim %s",
			fPrim.GetTypeName().GetText(), UsdDescribe(fPrim).c_str());
		return strAncestorTypes;
	}

	// According to the USD docs GetAllAncestorTypes() is expensive, so we keep a cache.
	static std::map<TfType, std::vector<std::string>> ancestorTypesCache;
	const auto iter = ancestorTypesCache.find(schemaType);
	if (iter != ancestorTypesCache.end()) {
		return iter->second;
	}

	std::vector<TfType> tfAncestorTypes;
	schemaType.GetAllAncestorTypes(&tfAncestorTypes);
	for (const TfType& ty : tfAncestorTypes)
	{
#if USD_VERSION_NUM >= 2008
		// If there is a concrete schema type name, we'll return that since it is what
		// is used/shown in the UI (ex: 'Xform' vs 'UsdGeomXform').
		auto concreteType = UsdSchemaRegistry::GetConcreteSchemaTypeName(ty);
		strAncestorTypes.emplace_back(!concreteType.IsEmpty() ? concreteType : ty.GetTypeName());
#else
		strAncestorTypes.emplace_back(ty.GetTypeName());
#endif
	}
	ancestorTypesCache[schemaType] = strAncestorTypes;
	return strAncestorTypes;
}
#endif

} // namespace ufe
} // namespace MayaUsd
