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

#include <pxr/usd/usd/variantSets.h>

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
)
{
    Ufe::ContextOps::Items items;
    if (itemPath.empty()) {
        // Top-level items.  Just variant sets for now.
        if (fPrim.HasVariantSets()) {
            Ufe::ContextItem variantSetsOp("Variant Sets", "Variant Sets", true);
            items.push_back(variantSetsOp);
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
                    
                    items.push_back(Ufe::ContextItem(*i, *i, true));
                }
            }
            else {
                // Variants of a given variant set.  Second item in the path is
                // the variant set name.
                assert(itemPath.size() == 2u);

                UsdVariantSet varSet = varSets.GetVariantSet(itemPath[1]);
                auto current = varSet.GetVariantSelection();

                const auto varNames = varSet.GetVariantNames();
                for (const auto& vn : varNames) {
                    const bool isCurrent(vn == current);
                    items.push_back(Ufe::ContextItem(vn, vn, false, isCurrent));
                }
            } // Variants of a variant set
        } // Variant sets
    } // Top-level items

    return items;
}

bool UsdContextOps::doOp(const ItemPath& itemPath)
{
    return false;
}

} // namespace ufe
} // namespace MayaUsd
