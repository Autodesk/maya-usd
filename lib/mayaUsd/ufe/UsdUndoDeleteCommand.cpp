//
// Copyright 2019 Autodesk
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
#include "UsdUndoDeleteCommand.h"

#include "private/UfeNotifGuard.h"

#if UFE_PREVIEW_VERSION_NUM >= 2029
#include <mayaUsd/undo/UsdUndoBlock.h>
#endif

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUndoDeleteCommand::UsdUndoDeleteCommand(const UsdPrim& prim)
    : Ufe::UndoableCommand()
#if UFE_PREVIEW_VERSION_NUM >= 2029
    , _prim(prim)
#else
    , fPrim(prim)
#endif
{
}

UsdUndoDeleteCommand::~UsdUndoDeleteCommand() { }

UsdUndoDeleteCommand::Ptr UsdUndoDeleteCommand::create(const UsdPrim& prim)
{
    return std::make_shared<UsdUndoDeleteCommand>(prim);
}

#if UFE_PREVIEW_VERSION_NUM >= 2029
void UsdUndoDeleteCommand::execute()
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;

    UsdUndoBlock undoBlock(&_undoableItem);
    _prim.SetActive(false);
}

void UsdUndoDeleteCommand::undo()
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;

    _undoableItem.undo();
}

void UsdUndoDeleteCommand::redo()
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;

    _undoableItem.redo();
}
#else
void UsdUndoDeleteCommand::perform(bool state)
{
    MayaUsd::ufe::InAddOrDeleteOperation ad;
    fPrim.SetActive(state);
}

void UsdUndoDeleteCommand::undo() { perform(true); }

void UsdUndoDeleteCommand::redo() { perform(false); }
#endif

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
