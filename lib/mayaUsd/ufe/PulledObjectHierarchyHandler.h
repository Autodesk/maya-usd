//
// Copyright 2021 Autodesk
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
    Pulled Maya objects are a sub-hierarchy of USD objects that are being
    edited as Maya data.  In Maya form, the sub-hierarchy is still rooted to
    its USD parent through pull information on the Maya root of the pulled
    sub-hierarchy.

    The PulledObjectHierarchyHandler wraps its argument Maya hierarchy handler,
    and calls it for scene item creation.  For hierarchy interface creation,
    the PulledObjectHierarchyHandler will check the Maya Dag path if there is
    pull information associated with it, which will be the case for the root of
    the pulled sub-hierarchy.  If so, it will create a PulledObjectHierarchy
    interface.  If not, it will delegate to the Maya hierarchy handler, which
    will create a normal Maya hierarchy interface.
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
