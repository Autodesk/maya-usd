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
#pragma once

#include "UsdSceneItem.h"

#include <ufe/attribute.h>

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/attribute.h>

// Ufe::Attribute overrides (minus the type method)
#define UFE_ATTRIBUTE_OVERRIDES																\
	bool hasValue() const override { return UsdAttribute::hasValue(); }						\
	std::string name() const override { return UsdAttribute::name(); }						\
	std::string documentation() const override { return UsdAttribute::documentation(); }	\
	std::string string() const override { return UsdAttribute::string(); }

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Internal helper class to implement the pure virtual methods from Ufe::Attribute.
class UsdAttribute
{
public:
	UsdAttribute(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr);
	~UsdAttribute();

	// Ufe::Attribute override methods that we've mimic'd here.
	bool hasValue() const;
	std::string name() const;
	std::string documentation() const;
	std::string string() const;

public:
	PXR_NS::UsdPrim fPrim;
	PXR_NS::UsdAttribute fUsdAttr;
}; // UsdAttribute

//! \brief Interface for USD attributes which don't match any defined type.
class UsdAttributeGeneric : public Ufe::AttributeGeneric, private UsdAttribute
{
public:
	typedef std::shared_ptr<UsdAttributeGeneric> Ptr;

	UsdAttributeGeneric(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr);

	//! Create a UsdAttributeGeneric.
	static UsdAttributeGeneric::Ptr create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr);

	// Ufe::Attribute overrides
	UFE_ATTRIBUTE_OVERRIDES

	// Ufe::AttributeGeneric overrides
	std::string nativeType() const override;
}; // UsdAttributeGeneric

//! \brief Interface for USD token attributes.
class UsdAttributeEnumString : public Ufe::AttributeEnumString, private UsdAttribute
{
public:
	typedef std::shared_ptr<UsdAttributeEnumString> Ptr;

	UsdAttributeEnumString(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr);

	//! Create a UsdAttributeEnumString.
	static UsdAttributeEnumString::Ptr create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr);

	// Ufe::Attribute overrides
	UFE_ATTRIBUTE_OVERRIDES

	// Ufe::AttributeEnumString overrides
	std::string get() const override;
	void set(const std::string& value) override;
	EnumValues getEnumValues() const override;
}; // UsdAttributeEnumString

//! \brief Internal helper template class to implement the get/set methods from Ufe::TypeAttribute.
template<typename T>
class TypedUsdAttribute : public Ufe::TypedAttribute<T>, private UsdAttribute
{
public:
	TypedUsdAttribute(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr);

	// Ufe::Attribute overrides
	UFE_ATTRIBUTE_OVERRIDES

	// Ufe::TypedAttribute overrides
	T get() const override;
	void set(const T& value) override;
}; // TypedUsdAttribute

//! \brief Interface for USD bool attributes.
class UsdAttributeBool : public TypedUsdAttribute<bool>
{
public:
	typedef std::shared_ptr<UsdAttributeBool> Ptr;

	using TypedUsdAttribute<bool>::TypedUsdAttribute;

	//! Create a UsdAttributeBool.
	static UsdAttributeBool::Ptr create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr);
}; // UsdAttributeBool

//! \brief Interface for USD int attributes.
class UsdAttributeInt : public TypedUsdAttribute<int>
{
public:
	typedef std::shared_ptr<UsdAttributeInt> Ptr;

	using TypedUsdAttribute<int>::TypedUsdAttribute;

	//! Create a UsdAttributeInt.
	static UsdAttributeInt::Ptr create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr);
}; // UsdAttributeInt

//! \brief Interface for USD float attributes.
class UsdAttributeFloat : public TypedUsdAttribute<float>
{
public:
	typedef std::shared_ptr<UsdAttributeFloat> Ptr;

	using TypedUsdAttribute<float>::TypedUsdAttribute;

	//! Create a UsdAttributeFloat.
	static UsdAttributeFloat::Ptr create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr);
}; // UsdAttributeFloat

//! \brief Interface for USD double attributes.
class UsdAttributeDouble : public TypedUsdAttribute<double>
{
public:
	typedef std::shared_ptr<UsdAttributeDouble> Ptr;

	using TypedUsdAttribute<double>::TypedUsdAttribute;

	//! Create a UsdAttributeDouble.
	static UsdAttributeDouble::Ptr create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr);
}; // UsdAttributeDouble

//! \brief Interface for USD string/token attributes.
class UsdAttributeString : public TypedUsdAttribute<std::string>
{
public:
	typedef std::shared_ptr<UsdAttributeString> Ptr;

	using TypedUsdAttribute<std::string>::TypedUsdAttribute;

	//! Create a UsdAttributeString.
	static UsdAttributeString::Ptr create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr);
}; // UsdAttributeString

//! \brief Interface for USD RGB color (float) attributes.
class UsdAttributeColorFloat3 : public TypedUsdAttribute<Ufe::Color3f>
{
public:
	typedef std::shared_ptr<UsdAttributeColorFloat3> Ptr;

	using TypedUsdAttribute<Ufe::Color3f>::TypedUsdAttribute;

	//! Create a UsdAttributeColorFloat3.
	static UsdAttributeColorFloat3::Ptr create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr);
}; // UsdAttributeColorFloat3

//! \brief Interface for USD Vector3i (int) attributes.
class UsdAttributeInt3 : public TypedUsdAttribute<Ufe::Vector3i>
{
public:
	typedef std::shared_ptr<UsdAttributeInt3> Ptr;

	using TypedUsdAttribute<Ufe::Vector3i>::TypedUsdAttribute;

	//! Create a UsdAttributeInt3.
	static UsdAttributeInt3::Ptr create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr);
}; // UsdAttributeInt3

//! \brief Interface for USD Vector3f (float) attributes.
class UsdAttributeFloat3 : public TypedUsdAttribute<Ufe::Vector3f>
{
public:
	typedef std::shared_ptr<UsdAttributeFloat3> Ptr;

	using TypedUsdAttribute<Ufe::Vector3f>::TypedUsdAttribute;

	//! Create a UsdAttributeFloat3.
	static UsdAttributeFloat3::Ptr create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr);
}; // UsdAttributeFloat3

//! \brief Interface for USD Vector3d (double) attributes.
class UsdAttributeDouble3 : public TypedUsdAttribute<Ufe::Vector3d>
{
public:
	typedef std::shared_ptr<UsdAttributeDouble3> Ptr;

	using TypedUsdAttribute<Ufe::Vector3d>::TypedUsdAttribute;

	//! Create a UsdAttributeDouble3.
	static UsdAttributeDouble3::Ptr create(const UsdSceneItem::Ptr& item, const PXR_NS::UsdAttribute& usdAttr);
}; // UsdAttributeDouble3

} // namespace ufe
} // namespace MayaUsd
