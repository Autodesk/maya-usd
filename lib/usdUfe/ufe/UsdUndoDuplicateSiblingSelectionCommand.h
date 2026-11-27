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
#ifndef USDUFE_USDUNDODUPLICATESIBLINGSELECTIONCOMMAND_H
#define USDUFE_USDUNDODUPLICATESIBLINGSELECTIONCOMMAND_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UfeVersionCompat.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/UsdUndoDuplicateCommand.h>
#include <usdUfe/undo/UsdUndoableItem.h>

#include <ufe/path.h>
#include <ufe/selection.h>
#include <ufe/undoableCommand.h>
#include <ufe/value.h>

#include <map>
#include <unordered_map>

namespace USDUFE_NS_DEF {

//! \brief Duplicate a selection, each item next to itself under its own parent.
class USDUFE_PUBLIC UsdUndoDuplicateSiblingSelectionCommand : public Ufe::SelectionUndoableCommand
{
public:
    typedef std::shared_ptr<UsdUndoDuplicateSiblingSelectionCommand> Ptr;

    UsdUndoDuplicateSiblingSelectionCommand(
        const Ufe::Selection&       selection,
        const Ufe::ValueDictionary& duplicateOptions);

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdUndoDuplicateSiblingSelectionCommand);

    //! Create a UsdUndoDuplicateSiblingSelectionCommand from a selection and duplicate options.
    static Ptr
    create(const Ufe::Selection& selection, const Ufe::ValueDictionary& duplicateOptions);

    void execute() override;
    void undo() override;
    void redo() override;
    UFE_V4(std::string commandString() const override { return "DuplicateSelection"; })

    Ufe::SceneItem::Ptr targetItem(const Ufe::Path& sourcePath) const override;

private:
    // Transient list of items to duplicate. Needed by execute.
    std::vector<UsdSceneItem::Ptr> _sourceItems;

    UsdUndoableItem _undoableItem;
    const bool      _copyExternalInputs;

    using CommandMap = std::unordered_map<Ufe::Path, UsdUndoDuplicateCommand::Ptr>;
    CommandMap _perItemCommands;

    // Fixup data:
    using DuplicatePathsMap = std::map<PXR_NS::SdfPath, PXR_NS::SdfPath>;
    using DuplicatesMap = std::unordered_map<Ufe::Path, DuplicatePathsMap>;
    DuplicatesMap _duplicatesMap;

    bool updateSdfPathVector(
        PXR_NS::SdfPathVector&               pathVec,
        const DuplicatePathsMap::value_type& duplicatePair,
        const DuplicatePathsMap&             otherPairs,
        const bool                           keepExternal);
}; // UsdUndoDuplicateSiblingSelectionCommand

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDUNDODUPLICATESIBLINGSELECTIONCOMMAND_H
