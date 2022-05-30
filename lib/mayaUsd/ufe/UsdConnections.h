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

#include <ufe/connections.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

class MAYAUSD_CORE_PUBLIC UsdConnections : public Ufe::Connections
{
public:
    typedef std::shared_ptr<UsdConnections> Ptr;

    UsdConnections(const Ufe::SceneItem::Ptr& item);
    ~UsdConnections() override;

    //@{
    // Delete the copy/move constructors assignment operators.
    UsdConnections(const UsdConnections&) = delete;
    UsdConnections& operator=(const UsdConnections&) = delete;
    UsdConnections(UsdConnections&&) = delete;
    UsdConnections& operator=(UsdConnections&&) = delete;
    //@}

    static UsdConnections::Ptr create(const Ufe::SceneItem::Ptr& item);

    std::vector<Ufe::Connection::Ptr> allSourceConnections() const override;

}; // UsdConnections

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

