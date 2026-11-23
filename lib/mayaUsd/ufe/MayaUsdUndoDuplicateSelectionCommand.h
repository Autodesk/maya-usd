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
#ifndef MAYAUSD_MAYAUSDUNDODUPLICATESELECTIONCOMMAND_H
#define MAYAUSD_MAYAUSDUNDODUPLICATESELECTIONCOMMAND_H

#include <mayaUsd/base/api.h>

#include <usdUfe/ufe/UsdUndoDuplicateInPlaceSelectionCommand.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Duplicates a selection in place, replicating the Maya extras of each item.
class MAYAUSD_CORE_PUBLIC MayaUsdUndoDuplicateSelectionCommand
    : public UsdUfe::UsdUndoDuplicateInPlaceSelectionCommand
{
public:
    typedef std::shared_ptr<MayaUsdUndoDuplicateSelectionCommand> Ptr;

    MayaUsdUndoDuplicateSelectionCommand(
        const Ufe::Selection&       selection,
        const Ufe::ValueDictionary& duplicateOptions);

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(MayaUsdUndoDuplicateSelectionCommand);

    //! Create a MayaUsdUndoDuplicateSelectionCommand from a selection and duplicate options.
    static Ptr
    create(const Ufe::Selection& selection, const Ufe::ValueDictionary& duplicateOptions);

protected:
    UsdUfe::UsdUndoDuplicateCommand::Ptr
    createItemDuplicateCmd(const UsdUfe::UsdSceneItem::Ptr& srcItem) override;
}; // MayaUsdUndoDuplicateSelectionCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_MAYAUSDUNDODUPLICATESELECTIONCOMMAND_H
