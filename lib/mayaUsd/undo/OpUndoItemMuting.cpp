//
// Copyright 2023 Autodesk
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

#include <mayaUsd/undo/OpUndoItemMuting.h>

namespace MAYAUSD_NS_DEF {

OpUndoItemMuting::OpUndoItemMuting(bool forcedMuting)
    : _preservedUndoInfo()
    , _forcedMuting(forcedMuting)
{
    if (!_forcedMuting && OpUndoItemValidator::isNested())
        return;

    _preservedUndoInfo = std::move(OpUndoItemList::instance());
    OpUndoItemList::instance().clear();
}

OpUndoItemMuting::~OpUndoItemMuting()
{
    if (!_forcedMuting && OpUndoItemValidator::isNested())
        return;

    OpUndoItemList::instance() = std::move(_preservedUndoInfo);
}

} // namespace MAYAUSD_NS_DEF
