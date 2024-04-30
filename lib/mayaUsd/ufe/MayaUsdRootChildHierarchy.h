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

#include <usdUfe/ufe/UsdRootChildHierarchy.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief USD run-time hierarchy interface for children of the USD root prim.
/*!
    This class modifies its base class implementation to return the Maya USD
    gateway node as parent of USD prims that are children of the USD root prim.
 */
class MAYAUSD_CORE_PUBLIC MayaUsdRootChildHierarchy : public UsdRootChildHierarchy
{
public:
    typedef std::shared_ptr<MayaUsdRootChildHierarchy> Ptr;

    MayaUsdRootChildHierarchy(const UsdSceneItem::Ptr& item);

    //! Create a MayaUsdRootChildHierarchy.
    static MayaUsdRootChildHierarchy::Ptr create(const UsdSceneItem::Ptr& item);

protected:
    // UsdHierarchy overrides
    bool childrenHook(
        const PXR_NS::UsdPrim& child,
        Ufe::SceneItemList&    children,
        bool                   filterInactive) const override;

}; // MayaUsdRootChildHierarchy

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
