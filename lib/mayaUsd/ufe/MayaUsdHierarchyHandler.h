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
#ifndef MAYAUSD_MAYAUSDHIERARCHYHANDLER_H
#define MAYAUSD_MAYAUSDHIERARCHYHANDLER_H

#include <mayaUsd/base/api.h>

#include <usdUfe/ufe/UsdHierarchyHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Maya USD run-time hierarchy handler.
/*!
        This hierarchy handler overrides the UsdUfe version in order
        to create the MayaUsd hierarchy classes.
 */
class MAYAUSD_CORE_PUBLIC MayaUsdHierarchyHandler : public UsdUfe::UsdHierarchyHandler
{
public:
    typedef std::shared_ptr<MayaUsdHierarchyHandler> Ptr;

    MayaUsdHierarchyHandler() = default;

    //! Create a MayaUsdHierarchyHandler.
    static MayaUsdHierarchyHandler::Ptr create();

    // UsdHierarchyHandler overrides
    Ufe::Hierarchy::Ptr         hierarchy(const Ufe::SceneItem::Ptr& item) const override;
    Ufe::Hierarchy::ChildFilter childFilter() const override;

#ifdef UFE_HIERARCHY_HAS_FIND
    //! Search all the USD stages of the scene in a single pass, restricted to
    //! searchRoot. See findStagePrims().
    Ufe::Selection find(
        Ufe::Rtid                               runTimeId,
        const Ufe::Path&                        searchRoot,
        const Ufe::Hierarchy::NameMatchFcn&     matches,
        const Ufe::Hierarchy::ChildFilter&      childFilter,
        const Ufe::Hierarchy::FindInterruptFcn& interrupt) const override;
#endif

    //! Search the USD stages of the scene with
    //! UsdUfe::UsdHierarchyHandler::findPrims(), returning the items that the
    //! Maya USD hierarchy shows as descendants of searchRoot:
    //! - only the prims that are ancestors or descendants of the primPath of
    //!   their proxy shape are searched,
    //! - a pulled prim (edited as Maya) and its descendants are not searched;
    //!   instead, its Maya object and that object's Maya descendants are,
    //!   when the Maya object exists.
    static Ufe::Selection findStagePrims(
        const Ufe::Path&                   searchRoot,
        const NameMatchFcn&                matches,
        const Ufe::Hierarchy::ChildFilter& childFilter,
        const std::function<bool()>&       interrupt = {});

}; // MayaUsdHierarchyHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_MAYAUSDHIERARCHYHANDLER_H
