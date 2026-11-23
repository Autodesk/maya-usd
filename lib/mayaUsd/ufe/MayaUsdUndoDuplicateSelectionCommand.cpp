//
// Copyright 2026 Autodesk
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
#include "MayaUsdUndoDuplicateSelectionCommand.h"

#include <mayaUsd/ufe/MayaUsdUndoDuplicateCommand.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(
    UsdUfe::UsdUndoDuplicateInPlaceSelectionCommand,
    MayaUsdUndoDuplicateSelectionCommand);

MayaUsdUndoDuplicateSelectionCommand::MayaUsdUndoDuplicateSelectionCommand(
    const Ufe::Selection&       selection,
    const Ufe::ValueDictionary& duplicateOptions)
    : UsdUfe::UsdUndoDuplicateInPlaceSelectionCommand(selection, duplicateOptions)
{
}

MayaUsdUndoDuplicateSelectionCommand::Ptr MayaUsdUndoDuplicateSelectionCommand::create(
    const Ufe::Selection&       selection,
    const Ufe::ValueDictionary& duplicateOptions)
{
    auto retVal = std::make_shared<MayaUsdUndoDuplicateSelectionCommand>(selection, duplicateOptions);
    if (retVal->_sourceItems.empty()) {
        return {};
    }
    return retVal;
}

UsdUfe::UsdUndoDuplicateCommand::Ptr
MayaUsdUndoDuplicateSelectionCommand::createItemDuplicateCmd(const UsdUfe::UsdSceneItem::Ptr& srcItem)
{
    return MayaUsdUndoDuplicateCommand::create(srcItem);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
