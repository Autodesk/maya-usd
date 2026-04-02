//
// Copyright 2021 Autodesk
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

#include "UsdTransform3dUndoableCommands.h"

#include <usdUfe/base/tokens.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/utils/editRouterContext.h>

#include <ufe/transform3d.h>

namespace USDUFE_NS_DEF {

USDUFE_VERIFY_CLASS_SETUP(Ufe::SetMatrix4dUndoableCommand, UsdSetMatrix4dUndoableCommand);

UsdSetMatrix4dUndoableCommand::UsdSetMatrix4dUndoableCommand(
    const Ufe::Path&     path,
    const Ufe::Matrix4d& newM)
    : Ufe::SetMatrix4dUndoableCommand(path)
{
    extractTRS(newM, &_newT, &_newR, &_newS);
}

bool UsdSetMatrix4dUndoableCommand::set(const Ufe::Matrix4d&)
{
    TF_WARN("Illegal call to UsdSetMatrix4dUndoableCommand::set()");
    return true;
}

void UsdSetMatrix4dUndoableCommand::execute()
{
    OperationEditRouterContext editContext(
        EditRoutingTokens->RouteTransform, ufePathToPrim(path()));

    // transform3d() and editTransform3d() are equivalent for a normal Maya
    // transform stack, but not for a fallback Maya transform stack, and
    // both can be edited by this command.

    auto t3d = Ufe::Transform3d::editTransform3d(sceneItem());
    if (!TF_VERIFY(t3d)) {
        return;
    }
    _compositeCmd = Ufe::CompositeUndoableCommand::create(
        { t3d->translateCmd(_newT.x(), _newT.y(), _newT.z()),
          t3d->rotateCmd(_newR.x(), _newR.y(), _newR.z()),
          t3d->scaleCmd(_newS.x(), _newS.y(), _newS.z()) });
    _compositeCmd->execute();
}

void UsdSetMatrix4dUndoableCommand::undo()
{
    if (_compositeCmd) {
        _compositeCmd->undo();
    }
}

void UsdSetMatrix4dUndoableCommand::redo()
{
    if (_compositeCmd) {
        _compositeCmd->redo();
    }
}

} // namespace USDUFE_NS_DEF
