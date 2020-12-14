//
// Copyright 2020 Autodesk
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

#include <ufe/contextOpsHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Maya run-time context ops handler with support for USD gateway node.
/*!
    This context ops handler is NOT a USD run-time context ops handler: it is a
    Maya run-time context ops handler.  It decorates the standard Maya run-time
    context ops handler and replaces it, providing special behavior only if the
    requested context ops interface is for the Maya to USD gateway node.

    For all other Maya nodes, this context ops handler simply delegates the work
    to the standard Maya context ops handler it decorates, which returns a
    standard Maya context ops interface object.
 */
class MAYAUSD_CORE_PUBLIC ProxyShapeContextOpsHandler : public Ufe::ContextOpsHandler
{
public:
    typedef std::shared_ptr<ProxyShapeContextOpsHandler> Ptr;

    ProxyShapeContextOpsHandler(const Ufe::ContextOpsHandler::Ptr& mayaContextOpsHandler);
    ~ProxyShapeContextOpsHandler() override;

    // Delete the copy/move constructors assignment operators.
    ProxyShapeContextOpsHandler(const ProxyShapeContextOpsHandler&) = delete;
    ProxyShapeContextOpsHandler& operator=(const ProxyShapeContextOpsHandler&) = delete;
    ProxyShapeContextOpsHandler(ProxyShapeContextOpsHandler&&) = delete;
    ProxyShapeContextOpsHandler& operator=(ProxyShapeContextOpsHandler&&) = delete;

    //! Create a ProxyShapeContextOpsHandler from a UFE hierarchy handler.
    static ProxyShapeContextOpsHandler::Ptr
    create(const Ufe::ContextOpsHandler::Ptr& mayaContextOpsHandler);

    // Ufe::ContextOpsHandler overrides
    Ufe::ContextOps::Ptr contextOps(const Ufe::SceneItem::Ptr& item) const override;

private:
    Ufe::ContextOpsHandler::Ptr _mayaContextOpsHandler;

}; // ProxyShapeContextOpsHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
