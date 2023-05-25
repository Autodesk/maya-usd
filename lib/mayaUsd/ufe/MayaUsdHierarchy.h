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
#pragma once

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

    MayaUsdHierarchy(const UsdSceneItem::Ptr& item);
    ~MayaUsdHierarchy() override;

    // Delete the copy/move constructors assignment operators.
    MayaUsdHierarchy(const MayaUsdHierarchy&) = delete;
    MayaUsdHierarchy& operator=(const MayaUsdHierarchy&) = delete;
    MayaUsdHierarchy(MayaUsdHierarchy&&) = delete;
    MayaUsdHierarchy& operator=(MayaUsdHierarchy&&) = delete;

    //! Create a MayaUsdHierarchy.
    static MayaUsdHierarchy::Ptr create(const UsdSceneItem::Ptr& item);

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
    const PXR_NS::UsdPrim& child,
    Ufe::SceneItemList&    children,
    bool                   filterInactive);

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
