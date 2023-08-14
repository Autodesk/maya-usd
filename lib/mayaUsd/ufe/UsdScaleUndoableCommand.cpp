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
#include "UsdScaleUndoableCommand.h"

#include "private/Utils.h"

namespace MAYAUSD_NS_DEF {
namespace ufe {

// Ensure that UsdScaleUndoableCommand is properly setup.
MAYAUSD_VERIFY_CLASS_SETUP(UsdTRSUndoableCommandBase<PXR_NS::GfVec3f>, UsdScaleUndoableCommand);
static_assert(std::is_base_of<Ufe::ScaleUndoableCommand, UsdScaleUndoableCommand>::value);

PXR_NS::TfToken UsdScaleUndoableCommand::scaleTok("xformOp:scale");

UsdScaleUndoableCommand::UsdScaleUndoableCommand(
    const Ufe::Path& path,
    double           x,
    double           y,
    double           z)
    : Ufe::ScaleUndoableCommand(path)
    , UsdTRSUndoableCommandBase(x, y, z)
{
}

/*static*/
UsdScaleUndoableCommand::Ptr
UsdScaleUndoableCommand::create(const Ufe::Path& path, double x, double y, double z)
{
    auto cmd = std::make_shared<MakeSharedEnabler<UsdScaleUndoableCommand>>(path, x, y, z);
    cmd->initialize();
    return cmd;
}

void UsdScaleUndoableCommand::undo() { undoImp(); }

void UsdScaleUndoableCommand::redo() { redoImp(); }

void UsdScaleUndoableCommand::addEmptyAttribute()
{
    performImp(1, 1, 1); // Add a neutral scale
}

void UsdScaleUndoableCommand::performImp(double x, double y, double z)
{
    scaleOp(prim(), path(), x, y, z);
}

//------------------------------------------------------------------------------
// Ufe::ScaleUndoableCommand overrides
//------------------------------------------------------------------------------

bool UsdScaleUndoableCommand::set(double x, double y, double z)
{
    perform(x, y, z);
    return true;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
