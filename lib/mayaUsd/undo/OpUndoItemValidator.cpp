//
// Copyright 2023 Autodesk
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

#include <mayaUsd/undo/OpUndoItemList.h>
#include <mayaUsd/undo/OpUndoItemValidator.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/stackTrace.h>
#include <pxr/base/tf/stringUtils.h>

#include <atomic>

namespace MAYAUSD_NS_DEF {

static std::atomic<int> validatorCount;

/* static */
void OpUndoItemValidator::increaseValidatorCount() { ++validatorCount; }

/* static */
void OpUndoItemValidator::decreaseValidatorCount() { --validatorCount; }

/* static */ bool OpUndoItemValidator::isNested() { return validatorCount > 1; }

/* static */
bool OpUndoItemValidator::validateEmpty()
{
#ifdef WANT_VALIDATE_UNDO_ITEM
    // Don't diagnose nested recorder and muting.
    if (isNested())
        return true;

    const OpUndoItemList& list = OpUndoItemList::instance();
    if (list.isEmpty())
        return true;

    // Note: TF macros require to be in the PXR namespace.
    PXR_NAMESPACE_USING_DIRECTIVE
    TF_CODING_ERROR("Undo item list not empty when starting to record or mute undo.");

    std::vector<std::string> names;
    for (const OpUndoItem::Ptr& item : list.getItems())
        names.emplace_back(item->getName());

    TF_CODING_ERROR("The items left behind are: %s", TfStringJoin(names).c_str());
    TF_CODING_ERROR(TfGetStackTrace().c_str());

    return false;
#else
    return true;
#endif
}

/* static */
bool OpUndoItemValidator::validateItem(const OpUndoItem& item)
{
#ifdef WANT_VALIDATE_UNDO_ITEM
    if (validatorCount > 0)
        return true;

    // Note: TF macros require to be in the PXR namespace.
    PXR_NAMESPACE_USING_DIRECTIVE
    TF_CODING_ERROR(
        "Undo item %s created without any undo recorder nor undo muting.", item.getName().c_str());
    TF_CODING_ERROR(TfGetStackTrace().c_str());

    return false;
#else
    (void)item;
    return true;
#endif
}

} // namespace MAYAUSD_NS_DEF
