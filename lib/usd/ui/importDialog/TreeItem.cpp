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

#include <mayaUsdUI/ui/IMayaMQtUtil.h>
#include <mayaUsdUI/ui/ItemDelegate.h>
#include <mayaUsdUI/ui/TreeModel.h>

#include <maya/MQtUtil.h>

#include <cassert>

namespace MAYAUSD_NS_DEF {

const QPixmap* TreeItem::fsCheckBoxOn = nullptr;
const QPixmap* TreeItem::fsCheckBoxOnDisabled = nullptr;
const QPixmap* TreeItem::fsCheckBoxOff = nullptr;
const QPixmap* TreeItem::fsCheckBoxOffDisabled = nullptr;

TreeItem::TreeItem(const UsdPrim& prim, bool isDefaultPrim, Column column) noexcept
    : ParentClass()
    , fPrim(prim)
    , fColumn(column)
    , fCheckState(CheckState::kChecked_Disabled)
    , fVariantSelectionModified(false)
{
    initializeItem(isDefaultPrim);
}

UsdPrim TreeItem::prim() const { return fPrim; }

int TreeItem::type() const { return ParentClass::ItemType::UserType; }

const QPixmap* TreeItem::createPixmap(const char* pixmapURL) const
{
    const QPixmap* pixmap = nullptr;

    const TreeModel* treeModel = qobject_cast<const TreeModel*>(model());
    if (treeModel != nullptr) {
        pixmap = treeModel->mayaQtUtil().createPixmap(pixmapURL);
    } else {
        // The tree model should never be null, but we can recover here if it is.
        TF_RUNTIME_ERROR("Unexpected null tree model");
        pixmap = new QPixmap(pixmapURL);
    }

    // If the resource fails to load, return a non-null pointer.
    static const QPixmap empty;
    if (!pixmap)
        pixmap = &empty;

    return pixmap;
}

const QPixmap& TreeItem::checkImage() const
{
    if (fsCheckBoxOn == nullptr) {
        fsCheckBoxOn = createPixmap(":/ImportDialog/checkboxOn.png");
        fsCheckBoxOnDisabled = createPixmap(":/ImportDialog/checkboxOnDisabled.png");
        fsCheckBoxOff = createPixmap(":/ImportDialog/checkboxOff.png");
        fsCheckBoxOffDisabled = createPixmap(":/ImportDialog/checkboxOffDisabled.png");
    }

    switch (fCheckState) {
    case CheckState::kChecked: return *fsCheckBoxOn;
    case CheckState::kChecked_Disabled: return *fsCheckBoxOnDisabled;
    case CheckState::kUnchecked: return *fsCheckBoxOff;
    case CheckState::kUnchecked_Disabled: return *fsCheckBoxOffDisabled;
    default: return *fsCheckBoxOffDisabled;
    }
}

void TreeItem::setCheckState(TreeItem::CheckState st)
{
    assert(fColumn == kColumnLoad);
    if (fColumn == kColumnLoad)
        fCheckState = st;
}

void TreeItem::setVariantSelectionModified()
{
    assert(fColumn == kColumnVariants);
    if (fColumn == kColumnVariants)
        fVariantSelectionModified = true;
}

void TreeItem::initializeItem(bool isDefaultPrim)
{
    switch (fColumn) {
    case kColumnLoad: fCheckState = CheckState::kChecked_Disabled; break;
    case kColumnName:
        if (fPrim.IsPseudoRoot())
            setText("Root");
        else
            setText(QString::fromStdString(fPrim.GetName().GetString()));
        if (isDefaultPrim) {
            if (const QPixmap* pixmap = TreeModel::getDefaultPrimPixmap())
                setData(*pixmap, Qt::DecorationRole);
        }
        break;
    case kColumnType: setText(QString::fromStdString(fPrim.GetTypeName().GetString())); break;
    case kColumnVariants: {
        if (fPrim.HasVariantSets()) {
            // We set a special role flag when this prim has variant sets.
            // So we know when to create the label and combo box(es) for the variant
            // sets and to override the drawing in the styled item delegate.
            setData(ItemDelegate::kVariants, ItemDelegate::kTypeRole);
        }
    } break;
    default: break;
    }
}

} // namespace MAYAUSD_NS_DEF
