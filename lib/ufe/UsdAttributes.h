// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#pragma once

#include "../base/api.h"

#include "UsdSceneItem.h"
#include "UsdAttribute.h"

#include "ufe/attributes.h"

#include "pxr/usd/usd/prim.h"

#include <unordered_map>

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface for USD Attributes.
class UsdAttributes : public Ufe::Attributes
{
public:
	typedef std::shared_ptr<UsdAttributes> Ptr;

	UsdAttributes(const UsdSceneItem::Ptr& item);
	~UsdAttributes() override;

	// Delete the copy/move constructors assignment operators.
	UsdAttributes(const UsdAttributes&) = delete;
	UsdAttributes& operator=(const UsdAttributes&) = delete;
	UsdAttributes(UsdAttributes&&) = delete;
	UsdAttributes& operator=(UsdAttributes&&) = delete;

	//! Create a UsdAttributes.
	static UsdAttributes::Ptr create(const UsdSceneItem::Ptr& item);

	// Ufe::Attributes overrides
	Ufe::SceneItem::Ptr sceneItem() const override;
	Ufe::Attribute::Type attributeType(const std::string& name) override;
	Ufe::Attribute::Ptr attribute(const std::string& name) override;
	std::vector<std::string> attributeNames() const override;
	bool hasAttribute(const std::string& name) const override;

private:
	Ufe::Attribute::Type getUfeTypeForAttribute(const PXR_NS::UsdAttribute& usdAttr) const;

private:
	UsdSceneItem::Ptr fItem;
	PXR_NS::UsdPrim fPrim;

	typedef std::unordered_map<std::string, Ufe::Attribute::Ptr> AttributeMap;
	AttributeMap fAttributes;

}; // UsdAttributes

} // namespace ufe
} // namespace MayaUsd
