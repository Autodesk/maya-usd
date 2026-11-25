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
#include "MayaUsdUndoDuplicateCommand.h"

#include <mayaUsd/ufe/Utils.h>

#include <usdUfe/ufe/Utils.h>

#include <ufe/hierarchy.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(UsdUfe::UsdUndoDuplicateCommand, MayaUsdUndoDuplicateCommand);

MayaUsdUndoDuplicateCommand::MayaUsdUndoDuplicateCommand(const UsdUfe::UsdSceneItem::Ptr& srcItem)
    : UsdUfe::UsdUndoDuplicateCommand(srcItem)
{
}

UsdUfe::UsdUndoDuplicateCommand::Ptr
MayaUsdUndoDuplicateCommand::create(const UsdUfe::UsdSceneItem::Ptr& srcItem)
{
    return std::make_shared<MayaUsdUndoDuplicateCommand>(srcItem);
}

void MayaUsdUndoDuplicateCommand::execute()
{
    auto                 item = Ufe::Hierarchy::createItem(_ufeSrcPath);
    ReplicateExtrasToUSD extras;
    extras.initRecursive(item);

    UsdUfe::UsdUndoDuplicateCommand::execute();

    ReplicateExtrasToUSD::RenamedPaths renamed;
    renamed[UsdUfe::ufePathToPrim(_ufeSrcPath).GetPath()] = _usdDstPath;
    extras.finalize(MayaUsd::ufe::stagePath(_srcStage), renamed);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
