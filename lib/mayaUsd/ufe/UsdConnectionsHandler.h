//
// Copyright 2022 Autodesk
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

#include <ufe/connectionsHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

class MAYAUSD_CORE_PUBLIC UsdConnectionsHandler : public Ufe::ConnectionsHandler
{
public:
    typedef std::shared_ptr<UsdConnectionsHandler> Ptr;

    //! Constructor.
    UsdConnectionsHandler();
    //! Destructor.
    ~UsdConnectionsHandler() override;

    //@{
    // Delete the copy/move constructors assignment operators.
    UsdConnectionsHandler(const UsdConnectionsHandler&) = delete;
    UsdConnectionsHandler& operator=(const UsdConnectionsHandler&) = delete;
    UsdConnectionsHandler(UsdConnectionsHandler&&) = delete;
    UsdConnectionsHandler& operator=(UsdConnectionsHandler&&) = delete;
    //@}

    static UsdConnectionsHandler::Ptr create();

    Ufe::Connections::Ptr sourceConnections(const Ufe::SceneItem::Ptr& item) const override;

    bool connect(const Ufe::Attribute::Ptr& srcAttr, const Ufe::Attribute::Ptr& dstAttr) const override;
    bool disconnect(const Ufe::Attribute::Ptr& srcAttr, const Ufe::Attribute::Ptr& dstAttr) const override;

}; // UsdConnectionsHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
