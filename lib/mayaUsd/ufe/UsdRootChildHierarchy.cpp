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
#include "UsdRootChildHierarchy.h"

#include <ufe/runTimeMgr.h>

#include <cassert>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------
extern Ufe::Rtid  g_MayaRtid;
const char* const kNotGatewayNodePath = "Tail of path %s is not a gateway node.";

UsdRootChildHierarchy::UsdRootChildHierarchy(const UsdSceneItem::Ptr& item)
    : UsdHierarchy(item)
{
}

UsdRootChildHierarchy::~UsdRootChildHierarchy() { }

/*static*/
UsdRootChildHierarchy::Ptr UsdRootChildHierarchy::create(const UsdSceneItem::Ptr& item)
{
    return std::make_shared<UsdRootChildHierarchy>(item);
}

//------------------------------------------------------------------------------
// Ufe::Hierarchy overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr UsdRootChildHierarchy::parent() const
{
    // If we're a child of the root, our parent node in the path is a Maya
    // node.  Ask the Maya hierarchy interface to create a selection item
    // for that path.
    auto parentPath = path().pop();
#if !defined(NDEBUG)
    assert(parentPath.runTimeId() == g_MayaRtid);
#endif
    if (parentPath.runTimeId() != g_MayaRtid)
        TF_WARN(kNotGatewayNodePath, path().string().c_str());

    auto mayaHierarchyHandler = Ufe::RunTimeMgr::instance().hierarchyHandler(g_MayaRtid);
    return mayaHierarchyHandler->createItem(parentPath);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
