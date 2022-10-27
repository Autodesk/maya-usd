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
#pragma once

#include <mayaUsd/ufe/UsdUndoableCommand.h>

#include <ufe/baseUndoableCommands.h>
#include <ufe/types.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Class for Ufe::Transform3d::setMatrixCmd() implementation.
//
// This class provides the implementation for Ufe::Transform3d::setMatrixCmd()
// derived classes, with undo / redo support.
//
class MAYAUSD_CORE_PUBLIC UsdSetMatrix4dUndoableCommand
    : public UsdBaseUndoableCommand<Ufe::SetMatrix4dUndoableCommand>
{
public:
    UsdSetMatrix4dUndoableCommand(const Ufe::Path& path, const Ufe::Matrix4d& newM);

    ~UsdSetMatrix4dUndoableCommand() override;

    // No-op: Maya does not set matrices through interactive manipulation.
    bool set(const Ufe::Matrix4d&) override;

protected:
    void executeUndoBlock() override;

private:
    Ufe::Vector3d _newT;
    Ufe::Vector3d _newR;
    Ufe::Vector3d _newS;
};

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
