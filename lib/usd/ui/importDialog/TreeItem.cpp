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

const QPixmap* TreeItem::checkBoxOn = nullptr;
const QPixmap* TreeItem::checkBoxOnDisabled = nullptr;
const QPixmap* TreeItem::checkBoxOff = nullptr;
const QPixmap* TreeItem::checkBoxOffDisabled = nullptr;

TreeItem::TreeItem(const UsdPrim& prim, bool isDefaultPrim, Column column) noexcept
    : ParentClass()
    , _prim(prim)
    , _column(column)
    , _checkState(CheckState::kChecked_Disabled)
    , _variantSelectionModified(false)
{
    initializeItem(isDefaultPrim);
}

UsdPrim TreeItem::prim() const { return _prim; }

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
    if (checkBoxOn == nullptr) {
        checkBoxOn = createPixmap(":/ImportDialog/checkboxOn.png");
        checkBoxOnDisabled = createPixmap(":/ImportDialog/checkboxOnDisabled.png");
        checkBoxOff = createPixmap(":/ImportDialog/checkboxOff.png");
        checkBoxOffDisabled = createPixmap(":/ImportDialog/checkboxOffDisabled.png");
    }

    switch (_checkState) {
    case CheckState::kChecked: return *checkBoxOn;
    case CheckState::kChecked_Disabled: return *checkBoxOnDisabled;
    case CheckState::kUnchecked: return *checkBoxOff;
    case CheckState::kUnchecked_Disabled: return *checkBoxOffDisabled;
    default: return *checkBoxOffDisabled;
    }
}

void TreeItem::setCheckState(TreeItem::CheckState st)
{
    assert(_column == kColumnLoad);
    if (_column == kColumnLoad)
        _checkState = st;
}

void TreeItem::setVariantSelectionModified()
{
    assert(_column == kColumnVariants);
    if (_column == kColumnVariants)
        _variantSelectionModified = true;
}

void TreeItem::initializeItem(bool isDefaultPrim)
{
    switch (_column) {
    case kColumnLoad: _checkState = CheckState::kChecked_Disabled; break;
    case kColumnName:
        if (_prim.IsPseudoRoot())
            setText("Root");
        else
            setText(QString::fromStdString(_prim.GetName().GetString()));
        if (isDefaultPrim) {
            if (const QPixmap* pixmap = TreeModel::getDefaultPrimPixmap())
                setData(*pixmap, Qt::DecorationRole);
        }
        break;
    case kColumnType: setText(QString::fromStdString(_prim.GetTypeName().GetString())); break;
    case kColumnVariants: {
        if (_prim.HasVariantSets()) {
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
