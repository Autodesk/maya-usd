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

#include "UsdAttribute.h"

#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/usd/schemaRegistry.h>
#include <pxr/usd/sdf/attributeSpec.h>

// We unconditionally want the UFE asserts here (even in release builds).
// The UFE_ASSERT_MSG has a built-in throw which we want to use for error handling.
#define UFE_ENABLE_ASSERTS
#include <ufe/ufeAssert.h>

#include <unordered_map>
#include <unordered_set>
#include <iostream>

// Note: normally we would use this using directive, but here we cannot because
//		 our class is called UsdAttribute which is exactly the same as the one
//		 in USD.
// PXR_NAMESPACE_USING_DIRECTIVE

static constexpr char kErrorMsgFailedSet[] = "Failed to set USD attribute with new value";
static constexpr char kErrorMsgFailedConvertToString[] = "Could not convert the attribute to a string";
static constexpr char kErrorMsgInvalidType[] = "USD attribute does not match created attribute class type";
static constexpr char kErrorMsgEnumNoValue[] = "Enum string attribute has no value";

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------

namespace
{

std::string getUsdAttributeValueAsString(const PXR_NS::UsdAttribute& attr)
{
	if (!attr.HasValue()) return std::string();

	PXR_NS::VtValue v;
	if (attr.Get(&v))
	{
		if (v.CanCast<std::string>())
		{
			PXR_NS::VtValue v_str = v.Cast<std::string>();
			return v_str.Get<std::string>();
		}

		std::ostringstream os;
		os << v;
		return os.str();
	}

	UFE_ASSERT_MSG(false, kErrorMsgFailedConvertToString);
	return std::string();
}

template<typename T, typename U>
U getUsdAttributeVectorAsUfe(const PXR_NS::UsdAttribute& attr)
{
	if (!attr.HasValue()) return U();

	PXR_NS::VtValue vt;
	if (attr.Get(&vt) && vt.IsHolding<T>())
	{
		T gfVec = vt.UncheckedGet<T>();
		U ret(gfVec[0], gfVec[1], gfVec[2]);
		return ret;
	}

	UFE_ASSERT_MSG(false, kErrorMsgInvalidType);
	return U();
}

template<typename T, typename U>
void setUsdAttributeVectorFromUfe(PXR_NS::UsdAttribute& attr, const U& value)
{
	T vec;
	UFE_ASSERT_MSG(attr.Get<T>(&vec), kErrorMsgInvalidType);
	vec.Set(value.x(), value.y(), value.z());
	bool b = attr.Set<T>(vec);
	UFE_ASSERT_MSG(b, kErrorMsgFailedSet);
}

} // end namespace

MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// UsdAttribute:
//------------------------------------------------------------------------------

UsdAttribute::UsdAttribute(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
 	: fUsdAttr(usdAttr)
{
	fPrim = item->prim();
}

UsdAttribute::~UsdAttribute()
{
}

bool UsdAttribute::hasValue() const
{
	return fUsdAttr.HasValue();
}

std::string UsdAttribute::name() const
{
	// Should be the same as the name we were created with.
	return fUsdAttr.GetName().GetString();
}

std::string UsdAttribute::documentation() const
{
	return fUsdAttr.GetDocumentation();
}

std::string UsdAttribute::string() const
{
	return getUsdAttributeValueAsString(fUsdAttr);
}

//------------------------------------------------------------------------------
// UsdAttributeGeneric:
//------------------------------------------------------------------------------

UsdAttributeGeneric::UsdAttributeGeneric(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
	: Ufe::AttributeGeneric(item)
	, UsdAttribute(item, usdAttr)
{
}

/*static*/
UsdAttributeGeneric::Ptr UsdAttributeGeneric::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
	auto attr = std::make_shared<UsdAttributeGeneric>(item, usdAttr);
	return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeGeneric - Ufe::AttributeGeneric overrides
//------------------------------------------------------------------------------

std::string UsdAttributeGeneric::nativeType() const
{
	return fUsdAttr.GetTypeName().GetType().GetTypeName();
}

//------------------------------------------------------------------------------
// UsdAttributeEnumString:
//------------------------------------------------------------------------------

UsdAttributeEnumString::UsdAttributeEnumString(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
	: Ufe::AttributeEnumString(item)
	, UsdAttribute(item, usdAttr)
{
}

/*static*/
UsdAttributeEnumString::Ptr UsdAttributeEnumString::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
	auto attr = std::make_shared<UsdAttributeEnumString>(item, usdAttr);
	return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeEnumString - Ufe::AttributeEnumString overrides
//------------------------------------------------------------------------------

std::string UsdAttributeEnumString::get() const
{
	UFE_ASSERT_MSG(hasValue(), kErrorMsgEnumNoValue);
	PXR_NS::VtValue vt;
	if (fUsdAttr.Get(&vt) && vt.IsHolding<TfToken>())
	{
		TfToken tok = vt.UncheckedGet<TfToken>();
		return tok.GetString();
	}

	UFE_ASSERT_MSG(false, kErrorMsgInvalidType);
	return std::string();
}

void UsdAttributeEnumString::set(const std::string& value)
{
	PXR_NS::TfToken dummy;
	UFE_ASSERT_MSG(fUsdAttr.Get<PXR_NS::TfToken>(&dummy), kErrorMsgInvalidType);
	PXR_NS::TfToken tok(value);
	bool b = fUsdAttr.Set<PXR_NS::TfToken>(tok);
	UFE_ASSERT_MSG(b, kErrorMsgFailedSet);
}

Ufe::AttributeEnumString::EnumValues UsdAttributeEnumString::getEnumValues() const
{
	PXR_NS::TfToken tk(name());
	auto attrDefn = PXR_NS::UsdSchemaRegistry::GetAttributeDefinition(fPrim.GetTypeName(), tk);
	if (attrDefn && attrDefn->HasAllowedTokens())
	{
		auto tokenArray = attrDefn->GetAllowedTokens();
		std::vector<PXR_NS::TfToken> tokenVec(tokenArray.begin(), tokenArray.end());
		EnumValues tokens = PXR_NS::TfToStringVector(tokenVec);
		return tokens;
	}

	return EnumValues();
}

//------------------------------------------------------------------------------
// TypedUsdAttribute<T>:
//------------------------------------------------------------------------------

template<typename T>
TypedUsdAttribute<T>::TypedUsdAttribute(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
	: Ufe::TypedAttribute<T>(item)
	, UsdAttribute(item, usdAttr)
{
}

//------------------------------------------------------------------------------
// TypedUsdAttribute<T> - Ufe::TypedAttribute overrides
//------------------------------------------------------------------------------

template<>
std::string TypedUsdAttribute<std::string>::get() const
{
	if (!hasValue()) return std::string();

	PXR_NS::VtValue vt;
	if (fUsdAttr.Get(&vt))
	{
		// The USDAttribute can be holding either TfToken or string.
		if (vt.IsHolding<TfToken>())
		{
			TfToken tok = vt.UncheckedGet<TfToken>();
			return tok.GetString();
		}
		else if (vt.IsHolding<std::string>())
		{
			return vt.UncheckedGet<std::string>();
		}
	}

	UFE_ASSERT_MSG(false, kErrorMsgInvalidType);
	return std::string();
}

template <>
void TypedUsdAttribute<std::string>::set(const std::string& value)
{
	// We need to figure out if the USDAttribute is holding a TfToken or string.
	const PXR_NS::SdfValueTypeName typeName = fUsdAttr.GetTypeName();
	if (typeName.GetHash() == SdfValueTypeNames->String.GetHash())
	{
		std::string dummy;
		UFE_ASSERT_MSG(fUsdAttr.Get<std::string>(&dummy), kErrorMsgInvalidType);
		bool b = fUsdAttr.Set<std::string>(value);
		UFE_ASSERT_MSG(b, kErrorMsgFailedSet);
		return;
	}
	else if (typeName.GetHash() == SdfValueTypeNames->Token.GetHash())
	{
		PXR_NS::TfToken dummy;
		UFE_ASSERT_MSG(fUsdAttr.Get<PXR_NS::TfToken>(&dummy), kErrorMsgInvalidType);
		PXR_NS::TfToken tok(value);
		bool b = fUsdAttr.Set<PXR_NS::TfToken>(tok);
		UFE_ASSERT_MSG(b, kErrorMsgFailedSet);
		return;
	}

	// If we get here it means the USDAttribute type wasn't TfToken or string.
	UFE_ASSERT_MSG(false, kErrorMsgInvalidType);
}

template<>
Ufe::Color3f TypedUsdAttribute<Ufe::Color3f>::get() const
{
	return getUsdAttributeVectorAsUfe<GfVec3f, Ufe::Color3f>(fUsdAttr);
}

// Note: cannot use setUsdAttributeVectorFromUfe since it relies on x/y/z
template<>
void TypedUsdAttribute<Ufe::Color3f>::set(const Ufe::Color3f& value)
{
	GfVec3f vec;
	UFE_ASSERT_MSG(fUsdAttr.Get<GfVec3f>(&vec), kErrorMsgInvalidType);
	vec.Set(value.r(), value.g(), value.b());
	bool b = fUsdAttr.Set<GfVec3f>(vec);
	UFE_ASSERT_MSG(b, kErrorMsgFailedSet);
}

template<>
Ufe::Vector3i TypedUsdAttribute<Ufe::Vector3i>::get() const
{
	return getUsdAttributeVectorAsUfe<GfVec3i, Ufe::Vector3i>(fUsdAttr);
}

template<>
void TypedUsdAttribute<Ufe::Vector3i>::set(const Ufe::Vector3i& value)
{
	setUsdAttributeVectorFromUfe<GfVec3i, Ufe::Vector3i>(fUsdAttr, value);
}

template<>
Ufe::Vector3f TypedUsdAttribute<Ufe::Vector3f>::get() const
{
	return getUsdAttributeVectorAsUfe<GfVec3f, Ufe::Vector3f>(fUsdAttr);
}

template<>
void TypedUsdAttribute<Ufe::Vector3f>::set(const Ufe::Vector3f& value)
{
	setUsdAttributeVectorFromUfe<GfVec3f, Ufe::Vector3f>(fUsdAttr, value);
}

template<>
Ufe::Vector3d TypedUsdAttribute<Ufe::Vector3d>::get() const
{
	return getUsdAttributeVectorAsUfe<GfVec3d, Ufe::Vector3d>(fUsdAttr);
}

template<>
void TypedUsdAttribute<Ufe::Vector3d>::set(const Ufe::Vector3d& value)
{
	setUsdAttributeVectorFromUfe<GfVec3d, Ufe::Vector3d>(fUsdAttr, value);
}

template<typename T>
T TypedUsdAttribute<T>::get() const
{
	if (!hasValue()) return T();

	PXR_NS::VtValue vt;
	if (fUsdAttr.Get(&vt) && vt.IsHolding<T>())
	{
		return vt.UncheckedGet<T>();
	}

	UFE_ASSERT_MSG(false, kErrorMsgInvalidType);
	return T();
}

template<typename T>
void TypedUsdAttribute<T>::set(const T& value)
{
	T dummy;
	UFE_ASSERT_MSG(fUsdAttr.Get<T>(&dummy), kErrorMsgInvalidType);
	bool b = fUsdAttr.Set<T>(value);
	UFE_ASSERT_MSG(b, kErrorMsgFailedSet);
}

//------------------------------------------------------------------------------
// UsdAttributeBool:
//------------------------------------------------------------------------------

/*static*/
UsdAttributeBool::Ptr UsdAttributeBool::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
	auto attr = std::make_shared<UsdAttributeBool>(item, usdAttr);
	return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeInt:
//------------------------------------------------------------------------------

/*static*/
UsdAttributeInt::Ptr UsdAttributeInt::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
	auto attr = std::make_shared<UsdAttributeInt>(item, usdAttr);
	return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeFloat:
//------------------------------------------------------------------------------

/*static*/
UsdAttributeFloat::Ptr UsdAttributeFloat::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
	auto attr = std::make_shared<UsdAttributeFloat>(item, usdAttr);
	return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeDouble:
//------------------------------------------------------------------------------

/*static*/
UsdAttributeDouble::Ptr UsdAttributeDouble::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
	auto attr = std::make_shared<UsdAttributeDouble>(item, usdAttr);
	return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeString:
//------------------------------------------------------------------------------

/*static*/
UsdAttributeString::Ptr UsdAttributeString::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
	auto attr = std::make_shared<UsdAttributeString>(item, usdAttr);
	return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeColorFloat3:
//------------------------------------------------------------------------------

/*static*/
UsdAttributeColorFloat3::Ptr UsdAttributeColorFloat3::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
	auto attr = std::make_shared<UsdAttributeColorFloat3>(item, usdAttr);
	return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeInt3:
//------------------------------------------------------------------------------

/*static*/
UsdAttributeInt3::Ptr UsdAttributeInt3::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
	auto attr = std::make_shared<UsdAttributeInt3>(item, usdAttr);
	return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeFloat3:
//------------------------------------------------------------------------------

/*static*/
UsdAttributeFloat3::Ptr UsdAttributeFloat3::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
	auto attr = std::make_shared<UsdAttributeFloat3>(item, usdAttr);
	return attr;
}

//------------------------------------------------------------------------------
// UsdAttributeDouble3:
//------------------------------------------------------------------------------

/*static*/
UsdAttributeDouble3::Ptr UsdAttributeDouble3::create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr)
{
	auto attr = std::make_shared<UsdAttributeDouble3>(item, usdAttr);
	return attr;
}

#if 0
// Note: if we were to implement generic attribute setting (via string) this
//		 would be the way it could be done.
bool UsdAttribute::setValue(const std::string& value)
{
	// Put the input string into a VtValue so we can cast it to the proper type.
	PXR_NS::VtValue val(value.c_str());

	// Attempt to cast the value to what we want.  Get a default value for this
	// attribute's type name.
	PXR_NS::VtValue defVal = fUsdAttr.GetTypeName().GetDefaultValue();

	// Attempt to cast the given string to the default value's type.
	// If casting fails, attempt to continue with the given value.
	PXR_NS::VtValue cast = PXR_NS::VtValue::CastToTypeOf(val, defVal);
	if (!cast.IsEmpty())
		cast.Swap(val);

	return fUsdAttr.Set(val);
}
#endif

} // namespace ufe
} // namespace MayaUsd
