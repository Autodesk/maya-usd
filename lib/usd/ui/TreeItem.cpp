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
#include "TreeItem.h"

#include <cassert>

#include <maya/MQtUtil.h>

#include <mayaUsdUI/ui/TreeModel.h>
#include <mayaUsdUI/ui/ItemDelegate.h>
#include <mayaUsdUI/ui/IMayaMQtUtil.h>

namespace MAYAUSD_NS_DEF {

QPixmap* TreeItem::fsCheckBoxOn = nullptr;
QPixmap* TreeItem::fsCheckBoxOnDisabled = nullptr;
QPixmap* TreeItem::fsCheckBoxOff = nullptr;
QPixmap* TreeItem::fsCheckBoxOffDisabled = nullptr;

TreeItem::TreeItem(const UsdPrim& prim, Type t) noexcept
	: ParentClass()
	, fPrim(prim)
	, fType(t)
	, fCheckState(CheckState::kChecked_Disabled)
	, fVariantSelectionModified(false)
{
	initializeItem();
}

UsdPrim TreeItem::prim() const
{
	return fPrim;
}

int TreeItem::type() const
{
	return ParentClass::ItemType::UserType;
}

const QPixmap& TreeItem::checkImage() const
{
	if (fsCheckBoxOn == nullptr)
	{
		const TreeModel* treeModel = qobject_cast<const TreeModel*>(model());
		if (treeModel != nullptr)
		{
			fsCheckBoxOn = treeModel->mayaQtUtil().createPixmap("checkboxOn.png");
			fsCheckBoxOnDisabled = treeModel->mayaQtUtil().createPixmap("checkboxOnDisabled.png");
			fsCheckBoxOff = treeModel->mayaQtUtil().createPixmap("checkboxOff.png");
			fsCheckBoxOffDisabled = treeModel->mayaQtUtil().createPixmap("checkboxOffDisabled.png");
		}
		else
		{
			// The tree model should never be null, but we can recover here if it is.
			TF_RUNTIME_ERROR("Unexpected null tree model");
			fsCheckBoxOn = new QPixmap(":/checkboxOn.png");
			fsCheckBoxOnDisabled = new QPixmap(":/checkboxOnDisabled.png");
			fsCheckBoxOff = new QPixmap(":/checkboxOff.png");
			fsCheckBoxOffDisabled = new QPixmap(":/checkboxOffDisabled.png");
		}
	}

	switch (fCheckState)
	{
	case CheckState::kChecked:
		return *fsCheckBoxOn;
	case CheckState::kChecked_Disabled:
		return *fsCheckBoxOnDisabled;
	case CheckState::kUnchecked:
		return *fsCheckBoxOff;
	case CheckState::kUnchecked_Disabled:
		return *fsCheckBoxOffDisabled;
	default:
		return *fsCheckBoxOffDisabled;
	}
}

void TreeItem::setCheckState(TreeItem::CheckState st)
{
	assert(fType == Type::kLoad);
	if (fType == Type::kLoad)
		fCheckState = st;
}

void TreeItem::setVariantSelectionModified()
{
	assert(fType == Type::kVariants);
	if (fType == Type::kVariants)
		fVariantSelectionModified = true;
}

void TreeItem::initializeItem()
{
	switch (fType)
	{
	case Type::kLoad:
		fCheckState = CheckState::kChecked_Disabled;
		break;
	case Type::kName:
		if (fPrim.IsPseudoRoot())
			setText("Root");
		else
			setText(QString::fromStdString(fPrim.GetName().GetString()));
		break;
	case Type::kType:
		setText(QString::fromStdString(fPrim.GetTypeName().GetString()));
		break;
	case Type::kVariants:
	{
		if (fPrim.HasVariantSets())
		{
			// We set a special role flag when this prim has variant sets.
			// So we know when to create the label and combo box(es) for the variant
			// sets and to override the drawing in the styled item delegate.
			setData(ItemDelegate::kVariants, ItemDelegate::kTypeRole);
		}
	}
		break;
	default:
		break;
	}
}

} // namespace MayaUsd
