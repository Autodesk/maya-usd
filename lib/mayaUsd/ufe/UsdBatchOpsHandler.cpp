//
// Copyright 2022 Autodesk
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
#include "UsdBatchOpsHandler.h"

#include <mayaUsd/ufe/UsdUndoDuplicateSelectionCommand.h>
#include <mayaUsd/ufe/Utils.h>

#include <ufe/pathString.h>

#ifdef UFE_BATCH_OPS_HAS_DUPLICATE_TO_TARGET
#include <usdUfe/ufe/UsdUndoDuplicateSelectionCommand.h>
#endif

namespace MAYAUSD_NS_DEF {
namespace ufe {

// Ensure that UsdBatchOpsHandler is properly setup.
#ifdef UFE_PREVIEW_CODE_WRAPPER_HANDLER_SUPPORT
MAYAUSD_VERIFY_CLASS_SETUP(Ufe::CodeWrapperHandler, UsdBatchOpsHandler);
#else
MAYAUSD_VERIFY_CLASS_SETUP(Ufe::BatchOpsHandler, UsdBatchOpsHandler);
#endif

/*static*/
UsdBatchOpsHandler::Ptr UsdBatchOpsHandler::create()
{
    return std::make_shared<UsdBatchOpsHandler>();
}

//------------------------------------------------------------------------------
// Ufe::BatchOpsHandler overrides
//------------------------------------------------------------------------------

Ufe::SelectionUndoableCommand::Ptr UsdBatchOpsHandler::duplicateSelectionCmd_(
    const Ufe::Selection&       selection,
    const Ufe::ValueDictionary& duplicateOptions)
{
#ifdef UFE_BATCH_OPS_HAS_DUPLICATE_TO_TARGET
    // Duplicating to a specific parent item.
    const auto itParent = duplicateOptions.find(kDstParentPath);
    if (itParent != duplicateOptions.end()) {
        if (!itParent->second.isType<std::string>()) {
            return nullptr;
        }
        const auto parentPath = Ufe::PathString::path(itParent->second.get<std::string>());
        const auto parentItem = downcast(Ufe::Hierarchy::createItem(parentPath));
        if (!parentItem) {
            return nullptr;
        }

        return UsdUfe::UsdUndoDuplicateSelectionCommand::create(selection, parentItem);
    }
#endif

    // Duplicating in place.
    return UsdUndoDuplicateSelectionCommand::create(selection, duplicateOptions);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
