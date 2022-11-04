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
#pragma once

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdUndoDuplicateCommand.h>
#include <mayaUsd/undo/UsdUndoableItem.h>

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>

#include <ufe/path.h>
#include <ufe/selection.h>
#include <ufe/undoableCommand.h>
#include <ufe/value.h>

#include <map>
#include <unordered_map>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief UsdUndoDuplicateSelectionCommand
class MAYAUSD_CORE_PUBLIC UsdUndoDuplicateSelectionCommand : public Ufe::SelectionUndoableCommand
{
public:
    typedef std::shared_ptr<UsdUndoDuplicateSelectionCommand> Ptr;

    UsdUndoDuplicateSelectionCommand(
        const Ufe::Selection&       selection,
        const Ufe::ValueDictionary& duplicateOptions);
    ~UsdUndoDuplicateSelectionCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdUndoDuplicateSelectionCommand(const UsdUndoDuplicateSelectionCommand&) = delete;
    UsdUndoDuplicateSelectionCommand& operator=(const UsdUndoDuplicateSelectionCommand&) = delete;
    UsdUndoDuplicateSelectionCommand(UsdUndoDuplicateSelectionCommand&&) = delete;
    UsdUndoDuplicateSelectionCommand& operator=(UsdUndoDuplicateSelectionCommand&&) = delete;

    //! Create a UsdUndoDuplicateSelectionCommand from a USD prim and UFE path.
    static Ptr
    create(const Ufe::Selection& selection, const Ufe::ValueDictionary& duplicateOptions);

    void execute() override;
    void undo() override;
    void redo() override;

    Ufe::SceneItem::Ptr targetItem(const Ufe::Path& sourcePath) const override;

private:
    UsdUndoableItem _undoableItem;
    const bool      _copyExternalInputs;

    typedef std::unordered_map<Ufe::Path, UsdUndoDuplicateCommand::Ptr> TCommandMap;
    TCommandMap                                                         _perItemCommands;

    // Fixup data:
    typedef std::map<SdfPath, SdfPath>                        TDuplicatePathsMap;
    typedef std::unordered_map<Ufe::Path, TDuplicatePathsMap> TDuplicatesMap;
    TDuplicatesMap                                            _duplicatesMap;

    bool _updateSdfPathVector(
        SdfPathVector&                        pathVec,
        const TDuplicatePathsMap::value_type& duplicatePair,
        const TDuplicatePathsMap&             otherPairs,
        const bool                            keepExternal);
}; // UsdUndoDuplicateSelectionCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
