//
// Copyright 2021 Autodesk
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
#include "selectability.h"

PXR_NAMESPACE_OPEN_SCOPE

/*! \brief  The tokens used in the selectability metadata.
 */
TfToken Selectability::Metadata("maya_selectability");
TfToken Selectability::InheritToken("inherit");
TfToken Selectability::SelectableToken("selectable");
TfToken Selectability::UnselectableToken("unselectable");

/*! \brief  Compute the selectability of a prim, considering inheritence.
 */
bool Selectability::isSelectable(UsdPrim prim)
{
    while (true) {
        // The reason we treat invalid prim as selectable is two-fold:
        //
        // - We don't want to influence selectability of things that are not prim that are being
        //   tested by accident.
        // - We loop inheritence until we reach an invalid parent prim, and prim are selectable
        //   by default.
        if (!prim.IsValid())
            return true;

        const State state = getLocalState(prim);
        switch (state) {
        case kSelectable: return true;
        case kUnselectable: return false;
        default: break;
        }

        prim = prim.GetParent();
    }
}

/*! \brief  Retrieve the local selectability state of a prim, without any inheritence.
 */
Selectability::State Selectability::getLocalState(const UsdPrim& prim)
{
    TfToken selectability;
    if (!prim.GetMetadata(Metadata, &selectability))
        return kInherit;

    if (selectability == UnselectableToken) {
        return kUnselectable;
    } else if (selectability == SelectableToken) {
        return kSelectable;
    } else {
        // Invalid values will be treated as inherit.
        return kInherit;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
