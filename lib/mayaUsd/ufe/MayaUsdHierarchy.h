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
#ifndef MAYAUSD_MAYAUSDHIERARCHY_H
#define MAYAUSD_MAYAUSDHIERARCHY_H

#include <mayaUsd/base/api.h>

#include <usdUfe/ufe/UsdHierarchy.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief USD run-time hierarchy interface
/*!
    This class overrides the base class UsdHierarchy in order to provide
    the children hook.
*/
class MAYAUSD_CORE_PUBLIC MayaUsdHierarchy : public UsdUfe::UsdHierarchy
{
public:
    typedef std::shared_ptr<MayaUsdHierarchy> Ptr;

    MayaUsdHierarchy(const UsdUfe::UsdSceneItem::Ptr& item);

    //! Create a MayaUsdHierarchy.
    static MayaUsdHierarchy::Ptr create(const UsdUfe::UsdSceneItem::Ptr& item);

    Ufe::SceneItemList children() const override;

protected:
    // UsdHierarchy overrides
    bool childrenHook(
        const PXR_NS::UsdPrim& child,
        Ufe::SceneItemList&    children,
        bool                   filterInactive) const override;

}; // MayaUsdHierarchy

//! Helper function to allow sharing code between this class MayaUsdHierarchy (which
//! derives from UsdHierarchy) and MayaUsdRootChildHierarchy (which derives from
//! UsdRootChildHierarchy). These two classes don't share a common base class
//! but they both override UsdHierarchy::childrenHook() with the same code.
bool mayaUsdHierarchyChildrenHook(
    const PXR_NS::SdfPath& proxyShapePrimPath,
    const PXR_NS::UsdPrim& child,
    Ufe::SceneItemList&    children,
    bool                   filterInactive,
    bool*                  itemCreated = nullptr);

//! Notify start/end of stage changes for hierarchy cache management.
void mayaUsdHierarchyStageChangedBegin();
void mayaUsdHierarchyStageChangedEnd();

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_MAYAUSDHIERARCHY_H
