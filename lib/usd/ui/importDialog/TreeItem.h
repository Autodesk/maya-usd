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

#ifndef MAYAUSDUI_TREE_ITEM_H
#define MAYAUSDUI_TREE_ITEM_H

#include <mayaUsd/mayaUsd.h>
#include <mayaUsdUI/ui/api.h>

#include <pxr/usd/usd/prim.h>

#include <QtGui/QPixmap>
#include <QtGui/QStandardItem>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

/**
 * \brief Item representing a node used to build a Qt TreeModel.
 * \remarks This item is intended to hold references to USD Prims in the future, so additional
 * information can be displayed to the User when interacting with Tree content.
 */
class MAYAUSD_UI_PUBLIC TreeItem : public QStandardItem
{
public:
    using ParentClass = QStandardItem;

    enum Column
    {
        kColumnLoad,
        kColumnName,
        kColumnType,
        kColumnVariants,
        kColumnLast
    };

    enum class CheckState
    {
        kChecked,
        kChecked_Disabled,
        kUnchecked,
        kUnchecked_Disabled
    };

    /**
     * \brief Constructor.
     * \param prim The USD Prim to represent with this item.
     * \param text Column text to display on the View of the the Qt TreeModel.
     */
    explicit TreeItem(const UsdPrim& prim, bool isDefaultPrim, Column column) noexcept;

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
    TreeItem::CheckState checkState() const { return _checkState; }
    const QPixmap&       checkImage() const;

    //! Sets the checkstate of this tree item.
    //! Only valid for kLoad type.
    void setCheckState(TreeItem::CheckState st);

    //! Returns true if the variant selection for this item was modified.
    //! Only valid for kVariants type.
    bool variantSelectionModified() const { return _variantSelectionModified; }

    //! Special flag set when the variant selection (of this item) is modified.
    //! Only valid for kVariants type.
    void setVariantSelectionModified();

    //! Reset the flag that is set to track if the variant selection has been modified.
    //! Only valid for kVariants type.
    void resetVariantSelectionModified() { _variantSelectionModified = false; }

private:
    void           initializeItem(bool isDefaultPrim);
    const QPixmap* createPixmap(const char* pixmapURL) const;

protected:
    // The USD Prim that the item represents in the TreeModel.
    UsdPrim _prim;

    // The column of this item.
    Column _column;

    // For the LOAD column, the check state.
    CheckState _checkState;

    // Special flag set when the variant selection was modified.
    bool _variantSelectionModified;

    static const QPixmap* checkBoxOn;
    static const QPixmap* checkBoxOnDisabled;
    static const QPixmap* checkBoxOff;
    static const QPixmap* checkBoxOffDisabled;
};

} // namespace MAYAUSD_NS_DEF

#endif
