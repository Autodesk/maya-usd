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

    MayaUsdHierarchyHandler();
    ~MayaUsdHierarchyHandler() override;

    // Delete the copy/move constructors assignment operators.
    MayaUsdHierarchyHandler(const MayaUsdHierarchyHandler&) = delete;
    MayaUsdHierarchyHandler& operator=(const MayaUsdHierarchyHandler&) = delete;
    MayaUsdHierarchyHandler(MayaUsdHierarchyHandler&&) = delete;
    MayaUsdHierarchyHandler& operator=(MayaUsdHierarchyHandler&&) = delete;

    //! Create a MayaUsdHierarchyHandler.
    static MayaUsdHierarchyHandler::Ptr create();

    // UsdHierarchyHandler overrides
    Ufe::Hierarchy::Ptr hierarchy(const Ufe::SceneItem::Ptr& item) const override;
}; // MayaUsdHierarchyHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
