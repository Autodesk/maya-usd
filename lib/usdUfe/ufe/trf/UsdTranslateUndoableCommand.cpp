//
// Copyright 2020 Autodesk
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
#include "UsdTranslateUndoableCommand.h"

#include "Utils.h"

namespace USDUFE_NS_DEF {

USDUFE_VERIFY_CLASS_SETUP(Ufe::TranslateUndoableCommand, UsdTranslateUndoableCommand);

PXR_NS::TfToken UsdTranslateUndoableCommand::xlate("xformOp:translate");

UsdTranslateUndoableCommand::UsdTranslateUndoableCommand(
    const Ufe::Path& path,
    double           x,
    double           y,
    double           z)
    : Ufe::TranslateUndoableCommand(path)
    , UsdTRSUndoableCommandBase(x, y, z)
{
}

/*static*/
UsdTranslateUndoableCommand::Ptr
UsdTranslateUndoableCommand::create(const Ufe::Path& path, double x, double y, double z)
{
    auto cmd = std::make_shared<MakeSharedEnabler<UsdTranslateUndoableCommand>>(path, x, y, z);
    cmd->initialize();
    return cmd;
}

void UsdTranslateUndoableCommand::undo() { undoImp(); }

void UsdTranslateUndoableCommand::redo() { redoImp(); }

void UsdTranslateUndoableCommand::addEmptyAttribute()
{
    performImp(0, 0, 0); // Add an empty translate
}

void UsdTranslateUndoableCommand::performImp(double x, double y, double z)
{
    translateOp(prim(), path(), x, y, z);
}

//------------------------------------------------------------------------------
// Ufe::TranslateUndoableCommand overrides
//------------------------------------------------------------------------------

bool UsdTranslateUndoableCommand::set(double x, double y, double z)
{
    perform(x, y, z);
    return true;
}

} // namespace USDUFE_NS_DEF
