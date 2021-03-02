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
#include <mayaUsd/ufe/UsdContextOps.h>

#include <ufe/contextOpsHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to create a UsdContextOps interface object.
class MAYAUSD_CORE_PUBLIC UsdContextOpsHandler : public Ufe::ContextOpsHandler
{
public:
    typedef std::shared_ptr<UsdContextOpsHandler> Ptr;

    UsdContextOpsHandler();
    ~UsdContextOpsHandler() override;

    // Delete the copy/move constructors assignment operators.
    UsdContextOpsHandler(const UsdContextOpsHandler&) = delete;
    UsdContextOpsHandler& operator=(const UsdContextOpsHandler&) = delete;
    UsdContextOpsHandler(UsdContextOpsHandler&&) = delete;
    UsdContextOpsHandler& operator=(UsdContextOpsHandler&&) = delete;

    //! Create a UsdContextOpsHandler.
    static UsdContextOpsHandler::Ptr create();

    // Ufe::ContextOpsHandler overrides
    Ufe::ContextOps::Ptr contextOps(const Ufe::SceneItem::Ptr& item) const override;
}; // UsdContextOpsHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
