//
// Copyright 2020 Autodesk
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

#include "UsdContextOps.h"
#include "Utils.h"

#include <ufe/attributes.h>
#include <ufe/attribute.h>

#include <pxr/usd/usd/variantSets.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/usdGeom/tokens.h>

#include <maya/MGlobal.h>

#include <cassert>

namespace {

//! \brief Undoable command for variant selection change
class SetVariantSelectionUndoableCommand : public Ufe::UndoableCommand
{
public:
    
    SetVariantSelectionUndoableCommand(
        const UsdPrim&                   prim,
        const Ufe::ContextOps::ItemPath& itemPath
    ) : fVarSet(prim.GetVariantSets().GetVariantSet(itemPath[1])),
        fOldSelection(fVarSet.GetVariantSelection()),
        fNewSelection(itemPath[2])
    {}

    void undo() override { fVarSet.SetVariantSelection(fOldSelection); }

    void redo() override { fVarSet.SetVariantSelection(fNewSelection); }

private:

    UsdVariantSet     fVarSet;
    const std::string fOldSelection;
    const std::string fNewSelection;
};

}

MAYAUSD_NS_DEF {
namespace ufe {

UsdContextOps::UsdContextOps()
	: Ufe::ContextOps()
{
}

UsdContextOps::~UsdContextOps()
{
}

/*static*/
UsdContextOps::Ptr UsdContextOps::create()
{
	return std::make_shared<UsdContextOps>();
}

void UsdContextOps::setItem(const UsdSceneItem::Ptr& item)
{
	fPrim = item->prim();
	fItem = item;
}

const Ufe::Path& UsdContextOps::path() const
{
	return fItem->path();
}

//------------------------------------------------------------------------------
// Ufe::ContextOps overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr UsdContextOps::sceneItem() const
{
	return fItem;
}

Ufe::ContextOps::Items UsdContextOps::getItems(
    const Ufe::ContextOps::ItemPath& itemPath
) const
{
    Ufe::ContextOps::Items items;
    if (itemPath.empty()) {
        // Top-level items.  Variant sets and visibility.
        if (fPrim.HasVariantSets()) {
            items.emplace_back(
                "Variant Sets", "Variant Sets", Ufe::ContextItem::kHasChildren);
        }
        auto attributes = Ufe::Attributes::attributes(sceneItem());
        if (attributes) {
            auto visibility =
                std::dynamic_pointer_cast<Ufe::AttributeEnumString>(
                    attributes->attribute(UsdGeomTokens->visibility));
            if (visibility) {
                auto current = visibility->get();
                const std::string l = (current == UsdGeomTokens->invisible) ?
                    std::string("Make Visible") : std::string("Make Invisible");
                items.emplace_back("Toggle Visibility", l);
            }
        }
    }
    else {
        if (itemPath[0] == "Variant Sets") {
            UsdVariantSets varSets = fPrim.GetVariantSets();
            std::vector<std::string> varSetsNames;
            varSets.GetNames(&varSetsNames);

            if (itemPath.size() == 1u) {
                // Variant sets list.
                for (auto i = varSetsNames.crbegin(); i != varSetsNames.crend(); ++i) {
                    
                    items.emplace_back(*i, *i, Ufe::ContextItem::kHasChildren);
                }
            }
            else {
                // Variants of a given variant set.  Second item in the path is
                // the variant set name.
                assert(itemPath.size() == 2u);

                UsdVariantSet varSet = varSets.GetVariantSet(itemPath[1]);
                auto selected = varSet.GetVariantSelection();

                const auto varNames = varSet.GetVariantNames();
                for (const auto& vn : varNames) {
                    const bool checked(vn == selected);
                    items.emplace_back(vn, vn, Ufe::ContextItem::kNoChildren,
                                       Ufe::ContextItem::kCheckable, checked,
                                       Ufe::ContextItem::kExclusive);
                }
            } // Variants of a variant set
        } // Variant sets
    } // Top-level items

    return items;
}

Ufe::UndoableCommand::Ptr UsdContextOps::doOpCmd(const ItemPath& itemPath)
{
    // Empty argument means no operation was specified, error.
    if (itemPath.empty()) {
        TF_CODING_ERROR("Empty path means no operation was specified");
        return nullptr;
    }

    if (itemPath[0] == "Variant Sets") {
        // Operation is to set a variant in a variant set.  Need both the
        // variant set and the variant as arguments to the operation.
        if (itemPath.size() != 3u) {
            TF_CODING_ERROR("Wrong number of arguments");
            return nullptr;
        }

        // At this point we know we have enough arguments to execute the
        // operation.
        return std::make_shared<SetVariantSelectionUndoableCommand>(
            fPrim, itemPath);
    } // Variant sets
    else if (itemPath[0] == "Toggle Visibility") {
        auto attributes = Ufe::Attributes::attributes(sceneItem());
        assert(attributes);
        auto visibility = std::dynamic_pointer_cast<Ufe::AttributeEnumString>(
            attributes->attribute(UsdGeomTokens->visibility));
        assert(visibility);
        auto current = visibility->get();
        return visibility->setCmd(
            current == UsdGeomTokens->invisible ? UsdGeomTokens->inherited : UsdGeomTokens->invisible);
    } // Visibility

    return nullptr;
}

} // namespace ufe
} // namespace MayaUsd
