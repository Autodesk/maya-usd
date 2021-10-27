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

#include <ufe/hierarchyHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Maya run-time hierarchy handler with support for pulled Maya objects.
/*!
    TO DOC
 */
class MAYAUSD_CORE_PUBLIC PulledObjectHierarchyHandler : public Ufe::HierarchyHandler
{
public:
    typedef std::shared_ptr<PulledObjectHierarchyHandler> Ptr;

    PulledObjectHierarchyHandler(const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler);

    // Delete the copy/move constructors assignment operators.
    PulledObjectHierarchyHandler(const PulledObjectHierarchyHandler&) = delete;
    PulledObjectHierarchyHandler& operator=(const PulledObjectHierarchyHandler&) = delete;
    PulledObjectHierarchyHandler(PulledObjectHierarchyHandler&&) = delete;
    PulledObjectHierarchyHandler& operator=(PulledObjectHierarchyHandler&&) = delete;

    //! Create a PulledObjectHierarchyHandler from a UFE hierarchy handler.
    static PulledObjectHierarchyHandler::Ptr
    create(const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler);

    // Ufe::HierarchyHandler overrides
    Ufe::Hierarchy::Ptr         hierarchy(const Ufe::SceneItem::Ptr& item) const override;
    Ufe::SceneItem::Ptr         createItem(const Ufe::Path& path) const override;
    Ufe::Hierarchy::ChildFilter childFilter() const override;

private:
    Ufe::HierarchyHandler::Ptr _mayaHierarchyHandler;

}; // PulledObjectHierarchyHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
