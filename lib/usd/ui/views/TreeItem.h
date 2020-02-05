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

#include <mayaUsd/ui/api.h>

#include <QtGui/QStandardItem>

#include <pxr/usd/usd/prim.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {

/**
 * \brief Item representing a node used to build a Qt TreeModel.
 * \remarks This item is intended to hold references to USD Prims in the future, so additional information can be
 * displayed to the User when interacting with Tree content.
 */
class MAYAUSD_UI_PUBLIC TreeItem : public QStandardItem
{
public:
	using ParentClass = QStandardItem;

	enum class Type
	{
		kLoad,
		kName,
		kType,
		kVariants
	};

	/**
	 * \brief Constructor.
	 * \param prim The USD Prim to represent with this item.
	 * \param text Column text to display on the View of the the Qt TreeModel.
	 */
	explicit TreeItem(const UsdPrim& prim, Type t) noexcept;

	/**
	 * \brief Destructor.
	 */
	virtual ~TreeItem() = default;

	/**
	 * \brief Return the USD Prim that is represented by the item.
	 * \return The USD Prim that is represented by the item.
	 */
	UsdPrim prim() const;

	/**
	 * \brief Return a flag indicating the type of the item.
	 * \remarks This is used by Qt to distinguish custom items from the base class.
	 * \return A flag indicating that the type is a custom item, different from the base class.
	 */
	int type() const override;

	//! Returns the check state of this tree item.
	//! Only valid for kLoad type.
	Qt::CheckState checkState() const { return fCheckState; }

	//! Sets the checkstate of this tree item.
	//! Only valid for kLoad type.
	void setCheckState(Qt::CheckState st);

	//! Returns true if the variant selection for this item was modified.
	//! Only valid for kVariants type.
	bool variantSelectionModified() const { return fVariantSelectionModified; }

	//! Special flag set when the variant selection (of this item) is modified.
	//! Only valid for kVariants type.
	void setVariantSelectionModified();

private:
	void initializeItem();

protected:
	// The USD Prim that the item represents in the TreeModel.
	UsdPrim fPrim;

	// The type of this item.
	Type fType;

	// For the LOAD column, the check state.
	Qt::CheckState fCheckState;

	// Special flag set when the variant selection was modified.
	bool fVariantSelectionModified;
};

} // namespace MayaUsd
