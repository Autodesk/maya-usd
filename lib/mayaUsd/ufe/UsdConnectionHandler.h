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

#include <ufe/connectionHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

class MAYAUSD_CORE_PUBLIC UsdConnectionHandler : public Ufe::ConnectionHandler
{
public:
    typedef std::shared_ptr<UsdConnectionHandler> Ptr;

    //! Constructor.
    UsdConnectionHandler();
    //! Destructor.
    ~UsdConnectionHandler() override;

    //@{
    // Delete the copy/move constructors assignment operators.
    UsdConnectionHandler(const UsdConnectionHandler&) = delete;
    UsdConnectionHandler& operator=(const UsdConnectionHandler&) = delete;
    UsdConnectionHandler(UsdConnectionHandler&&) = delete;
    UsdConnectionHandler& operator=(UsdConnectionHandler&&) = delete;
    //@}

    static UsdConnectionHandler::Ptr create();

    Ufe::Connections::Ptr sourceConnections(const Ufe::SceneItem::Ptr& item) const override;

protected:
    bool createConnection(const Ufe::Attribute::Ptr& srcAttr, const Ufe::Attribute::Ptr& dstAttr) const override;
    bool deleteConnection(const Ufe::Attribute::Ptr& srcAttr, const Ufe::Attribute::Ptr& dstAttr) const override;

}; // UsdConnectionHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
