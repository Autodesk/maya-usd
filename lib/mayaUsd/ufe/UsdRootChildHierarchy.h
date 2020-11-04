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
#pragma once

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdHierarchy.h>

// PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief USD run-time hierarchy interface for children of the USD root prim.
/*!
    This class modifies its base class implementation to return the Maya USD
    gateway node as parent of USD prims that are children of the USD root prim.
 */
class MAYAUSD_CORE_PUBLIC UsdRootChildHierarchy : public UsdHierarchy
{
public:
    typedef std::shared_ptr<UsdRootChildHierarchy> Ptr;

    UsdRootChildHierarchy(const UsdSceneItem::Ptr& item);
    ~UsdRootChildHierarchy() override;

    // Delete the copy/move constructors assignment operators.
    UsdRootChildHierarchy(const UsdRootChildHierarchy&) = delete;
    UsdRootChildHierarchy& operator=(const UsdRootChildHierarchy&) = delete;
    UsdRootChildHierarchy(UsdRootChildHierarchy&&) = delete;
    UsdRootChildHierarchy& operator=(UsdRootChildHierarchy&&) = delete;

    //! Create a UsdRootChildHierarchy.
    static UsdRootChildHierarchy::Ptr create(const UsdSceneItem::Ptr& item);

    // Ufe::Hierarchy overrides
    Ufe::SceneItem::Ptr parent() const override;

}; // UsdRootChildHierarchy

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
