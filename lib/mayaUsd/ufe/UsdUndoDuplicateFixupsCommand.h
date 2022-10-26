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
#include <mayaUsd/undo/UsdUndoableItem.h>

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>

#include <ufe/path.h>
#include <ufe/undoableCommand.h>

#include <map>
#include <unordered_map>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief UsdUndoDuplicateFixupsCommand
class MAYAUSD_CORE_PUBLIC UsdUndoDuplicateFixupsCommand : public Ufe::UndoableCommand
{
public:
    typedef std::shared_ptr<UsdUndoDuplicateFixupsCommand> Ptr;

    UsdUndoDuplicateFixupsCommand();
    ~UsdUndoDuplicateFixupsCommand() override;

    // Delete the copy/move constructors assignment operators.
    UsdUndoDuplicateFixupsCommand(const UsdUndoDuplicateFixupsCommand&) = delete;
    UsdUndoDuplicateFixupsCommand& operator=(const UsdUndoDuplicateFixupsCommand&) = delete;
    UsdUndoDuplicateFixupsCommand(UsdUndoDuplicateFixupsCommand&&) = delete;
    UsdUndoDuplicateFixupsCommand& operator=(UsdUndoDuplicateFixupsCommand&&) = delete;

    //! Create a UsdUndoDuplicateFixupsCommand from a USD prim and UFE path.
    static UsdUndoDuplicateFixupsCommand::Ptr create();

    //! Add a duplication pair to fixup.
    void trackDuplicates(const UsdPrim& srcPrim, const UsdPrim& dstPrim);

    void execute() override;
    void undo() override;
    void redo() override;

private:
    UsdUndoableItem _undoableItem;

    typedef std::map<SdfPath, SdfPath>                        TDuplicatePathsMap;
    typedef std::unordered_map<Ufe::Path, TDuplicatePathsMap> TDuplicatesMap;
    TDuplicatesMap                                            _duplicatesMap;

    bool duplicateUndo();
    bool duplicateRedo();

    bool _updateSdfPathVector(
        SdfPathVector&                        pathVec,
        const TDuplicatePathsMap::value_type& duplicatePair,
        const TDuplicatePathsMap&             otherPairs);
}; // UsdUndoDuplicateFixupsCommand

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
