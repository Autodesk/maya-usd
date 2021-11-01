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
#include "PulledObjectHierarchyHandler.h"

#include <mayaUsd/fileio/primUpdaterManager.h>
#include <mayaUsd/ufe/PulledObjectHierarchy.h>
#include <mayaUsd/utils/util.h>

#include <maya/MDagPath.h>
#include <ufe/path.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

PulledObjectHierarchyHandler::PulledObjectHierarchyHandler(
    const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler)
    : Ufe::HierarchyHandler()
    , _mayaHierarchyHandler(mayaHierarchyHandler)
{
}

/*static*/
PulledObjectHierarchyHandler::Ptr
PulledObjectHierarchyHandler::create(const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler)
{
    return std::make_shared<PulledObjectHierarchyHandler>(mayaHierarchyHandler);
}

//------------------------------------------------------------------------------
// Ufe::HierarchyHandler overrides
//------------------------------------------------------------------------------

Ufe::Hierarchy::Ptr PulledObjectHierarchyHandler::hierarchy(const Ufe::SceneItem::Ptr& item) const
{
    if (!item || item->path().empty())
        return nullptr;

    // Remove the "world" (but account for an item which is just "world").
    const auto& path = item->path().popHead();
    if (path.empty())
        return _mayaHierarchyHandler->hierarchy(item);

    auto      nbSegs = path.nbSegments();
    Ufe::Path ufePath;

    auto dagPath = PXR_NS::UsdMayaUtil::nameToDagPath(path.getSegments()[nbSegs - 1].string());
    if (PXR_NS::PrimUpdaterManager::readPullInformation(dagPath, ufePath)) {
        return PulledObjectHierarchy::create(_mayaHierarchyHandler, item, ufePath);
    } else {
        return _mayaHierarchyHandler->hierarchy(item);
    }
}

Ufe::SceneItem::Ptr PulledObjectHierarchyHandler::createItem(const Ufe::Path& path) const
{
    return _mayaHierarchyHandler->createItem(path);
}

Ufe::Hierarchy::ChildFilter PulledObjectHierarchyHandler::childFilter() const { return {}; }

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
