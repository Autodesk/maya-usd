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

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdBatchOpsHandler::UsdBatchOpsHandler()
    : Ufe::BatchOpsHandler()
{
}

UsdBatchOpsHandler::~UsdBatchOpsHandler() { }

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
    return UsdUndoDuplicateSelectionCommand::create(selection, duplicateOptions);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
