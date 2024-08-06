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
#ifndef USDUFE_USDROOTCHILDHIERARCHY_H
#define USDUFE_USDROOTCHILDHIERARCHY_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UsdHierarchy.h>

namespace USDUFE_NS_DEF {

//! \brief USD run-time hierarchy interface for children of the USD root prim.
/*!
    This class modifies its base class implementation to return the DCC USD
    gateway node as parent of USD prims that are children of the USD root prim.
 */
class USDUFE_PUBLIC UsdRootChildHierarchy : public UsdHierarchy
{
public:
    typedef std::shared_ptr<UsdRootChildHierarchy> Ptr;

    UsdRootChildHierarchy(const UsdSceneItem::Ptr& item);

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdRootChildHierarchy);

    //! Create a UsdRootChildHierarchy.
    static UsdRootChildHierarchy::Ptr create(const UsdSceneItem::Ptr& item);

    // Ufe::Hierarchy overrides
    Ufe::SceneItem::Ptr parent() const override;

}; // UsdRootChildHierarchy

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDROOTCHILDHIERARCHY_H
