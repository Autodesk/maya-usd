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
#include "mayaUsdEditForwardCommand.h"

#include <usdUfe/undo/UsdUndoBlock.h>

namespace MAYAUSD_NS_DEF {

MayaUsdEditForwardCommand::MayaUsdEditForwardCommand(Callbacks callbacks)
    : _callbacks(std::move(callbacks))
{
}

MayaUsdEditForwardCommand::Ptr MayaUsdEditForwardCommand::create(Callbacks callbacks)
{
    return std::make_shared<MayaUsdEditForwardCommand>(std::move(callbacks));
}

MayaUsdEditForwardCommand::Ptr
MayaUsdEditForwardCommand::create(const std::function<void()>& callback)
{
    return create(Callbacks { callback });
}

void MayaUsdEditForwardCommand::execute()
{
    UsdUfe::UsdUndoBlock undoBlock { &_undoableItem };
    for (const auto& cb : _callbacks) {
        if (cb) {
            cb();
        }
    }
}

} // namespace MAYAUSD_NS_DEF
