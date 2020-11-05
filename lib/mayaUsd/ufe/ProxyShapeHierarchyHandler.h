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

//! \brief Maya run-time hierarchy handler with support for USD gateway node.
/*!
    This hierarchy handler is NOT a USD run-time hierarchy handler: it is a
    Maya run-time hierarchy handler.  It decorates the standard Maya run-time
    hierarchy handler and replaces it, providing special behavior only if the
    requested hierarchy interface is for the Maya to USD gateway node.  In that
    case, it returns a special ProxyShapeHierarchy interface object, which
    knows how to handle USD children of the Maya ProxyShapeHierarchy node.

    For all other Maya nodes, this hierarchy handler simply delegates the work
    to the standard Maya hierarchy handler it decorates, which returns a
    standard Maya hierarchy interface object.
 */
class MAYAUSD_CORE_PUBLIC ProxyShapeHierarchyHandler : public Ufe::HierarchyHandler
{
public:
    typedef std::shared_ptr<ProxyShapeHierarchyHandler> Ptr;

    ProxyShapeHierarchyHandler(const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler);
    ~ProxyShapeHierarchyHandler() override;

    // Delete the copy/move constructors assignment operators.
    ProxyShapeHierarchyHandler(const ProxyShapeHierarchyHandler&) = delete;
    ProxyShapeHierarchyHandler& operator=(const ProxyShapeHierarchyHandler&) = delete;
    ProxyShapeHierarchyHandler(ProxyShapeHierarchyHandler&&) = delete;
    ProxyShapeHierarchyHandler& operator=(ProxyShapeHierarchyHandler&&) = delete;

    //! Create a ProxyShapeHierarchyHandler from a UFE hierarchy handler.
    static ProxyShapeHierarchyHandler::Ptr
    create(const Ufe::HierarchyHandler::Ptr& mayaHierarchyHandler);

    // Ufe::HierarchyHandler overrides
    Ufe::Hierarchy::Ptr hierarchy(const Ufe::SceneItem::Ptr& item) const override;
    Ufe::SceneItem::Ptr createItem(const Ufe::Path& path) const override;
#if UFE_PREVIEW_VERSION_NUM >= 2022
    UFE_V2(Ufe::Hierarchy::ChildFilter childFilter() const override;)
#endif

private:
    Ufe::HierarchyHandler::Ptr fMayaHierarchyHandler;

}; // ProxyShapeHierarchyHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
