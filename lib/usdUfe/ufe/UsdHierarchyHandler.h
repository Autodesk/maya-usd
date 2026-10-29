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
#ifndef USDUFE_USDHIERARCHYHANDLER_H
#define USDUFE_USDHIERARCHYHANDLER_H

#include <usdUfe/base/api.h>

#include <pxr/usd/usd/prim.h>

#include <ufe/hierarchyHandler.h>
#include <ufe/path.h>
#include <ufe/selection.h>

#include <functional>
#include <string>
#include <vector>

namespace USDUFE_NS_DEF {

//! \brief USD run-time hierarchy handler.
/*!
        This hierarchy handler is the standard USD run-time hierarchy handler.  Its
        only special behavior is to return a UsdRootChildHierarchy interface object
        if it is asked for a hierarchy interface for a child of the USD root prim.
        These prims are special because we define their parent to be the DCC USD
        gateway node, which the UsdRootChildHierarchy interface implements.
 */
class USDUFE_PUBLIC UsdHierarchyHandler : public Ufe::HierarchyHandler
{
public:
    typedef std::shared_ptr<UsdHierarchyHandler> Ptr;

    UsdHierarchyHandler() = default;

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdHierarchyHandler);

    //! Create a UsdHierarchyHandler.
    static UsdHierarchyHandler::Ptr create();

    // UsdHierarchyHandler overrides
    Ufe::Hierarchy::Ptr         hierarchy(const Ufe::SceneItem::Ptr& item) const override;
    Ufe::SceneItem::Ptr         createItem(const Ufe::Path& path) const override;
    Ufe::Hierarchy::ChildFilter childFilter() const override;

    //! Called by findPrims() for each traversed prim, before it is matched.
    //! Returns true if the hook handled the prim, in which case findPrims()
    //! neither matches the prim nor traverses its descendants. The hook can
    //! append items to found in place of the prim.
    using FindPrimHook = std::function<bool(const PXR_NS::UsdPrim& prim, Ufe::Selection& found)>;

    //! Returns the FindPrimHook to use for the stage of the argument gateway
    //! path, or an empty hook.
    using FindPrimHookFactory = std::function<FindPrimHook(const Ufe::Path& stagePath)>;

    //! Returns whether a prim name matches. Same type as
    //! Ufe::Hierarchy::NameMatchFcn, which older UFE versions do not have.
    using NameMatchFcn = std::function<bool(const std::string& name)>;

    //! Find the prims whose name satisfies matches by traversing the USD
    //! stages directly, in a single pass over all the argument stages.
    //!
    //! The traversed prims are those that UsdHierarchy::filteredChildren()
    //! returns for childFilter, or UsdHierarchy::children() if childFilter is
    //! empty, including instance proxies. A prim that is filtered out is not
    //! traversed, nor are its descendants.
    //!
    //! If searchRoot is a USD path, only the subtree rooted at that prim
    //! (inclusive, if it passes childFilter) is searched and stagesPaths is
    //! ignored. A point instance has no children, so only itself is matched.
    //! Otherwise, the stages whose path is searchRoot or a descendant
    //! of it are searched, or all of them if searchRoot is empty.
    //! \param stagesPaths UFE paths of the gateway nodes of the stages to search.
    //! \param searchRoot Path under which to search.
    //! \param matches Called with the name of every traversed prim. If empty,
    //!        nothing is found.
    //! \param childFilter Child filter driving which prims are traversed.
    //! \param interrupt If not empty, polled periodically during the traversal.
    //!        When it returns true, the search stops and the prims found so
    //!        far are returned.
    //! \param hookFactory If not empty, called once per searched stage to get
    //!        the FindPrimHook for that stage.
    //! \return Selection of the matching prims, in no particular order.
    static Ufe::Selection findPrims(
        const std::vector<Ufe::Path>&      stagesPaths,
        const Ufe::Path&                   searchRoot,
        const NameMatchFcn&                matches,
        const Ufe::Hierarchy::ChildFilter& childFilter,
        const std::function<bool()>&       interrupt = {},
        const FindPrimHookFactory&         hookFactory = {});
}; // UsdHierarchyHandler

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDHIERARCHYHANDLER_H
