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
#ifndef MAYAUSD_USDCONNECTIONHANDLER_H
#define MAYAUSD_USDCONNECTIONHANDLER_H

#include <mayaUsd/base/api.h>

#include <ufe/connectionHandler.h>

#include <memory>

namespace MAYAUSD_NS_DEF {
namespace ufe {

class MAYAUSD_CORE_PUBLIC UsdConnectionHandler : public Ufe::ConnectionHandler
{
public:
    typedef std::shared_ptr<UsdConnectionHandler> Ptr;

    //! Constructor.
    UsdConnectionHandler() = default;

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdConnectionHandler);

    static UsdConnectionHandler::Ptr create();

    Ufe::Connections::Ptr sourceConnections(const Ufe::SceneItem::Ptr& item) const override;

#ifdef UFE_V4_FEATURES_AVAILABLE
    std::shared_ptr<Ufe::ConnectionResultUndoableCommand> createConnectionCmd(
        const Ufe::Attribute::Ptr& srcAttr,
        const Ufe::Attribute::Ptr& dstAttr) const override;

    std::shared_ptr<Ufe::UndoableCommand> deleteConnectionCmd(
        const Ufe::Attribute::Ptr& srcAttr,
        const Ufe::Attribute::Ptr& dstAttr) const override;

private:
    Ufe::Connection
    createConnection(const Ufe::Attribute::Ptr& srcAttr, const Ufe::Attribute::Ptr& dstAttr) const;
    void
    deleteConnection(const Ufe::Attribute::Ptr& srcAttr, const Ufe::Attribute::Ptr& dstAttr) const;
#else
protected:
    bool createConnection(const Ufe::Attribute::Ptr& srcAttr, const Ufe::Attribute::Ptr& dstAttr)
        const override;
    bool deleteConnection(const Ufe::Attribute::Ptr& srcAttr, const Ufe::Attribute::Ptr& dstAttr)
        const override;
#endif
}; // UsdConnectionHandler
} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_USDCONNECTIONHANDLER_H
