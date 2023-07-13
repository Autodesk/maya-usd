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

#include <usdUfe/ufe/UsdContextOpsHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to create a MayaUsdContextOps interface object.
class MAYAUSD_CORE_PUBLIC MayaUsdContextOpsHandler : public UsdUfe::UsdContextOpsHandler
{
public:
    typedef std::shared_ptr<MayaUsdContextOpsHandler> Ptr;

    MayaUsdContextOpsHandler();
    ~MayaUsdContextOpsHandler() override;

    // Delete the copy/move constructors assignment operators.
    MayaUsdContextOpsHandler(const MayaUsdContextOpsHandler&) = delete;
    MayaUsdContextOpsHandler& operator=(const MayaUsdContextOpsHandler&) = delete;
    MayaUsdContextOpsHandler(MayaUsdContextOpsHandler&&) = delete;
    MayaUsdContextOpsHandler& operator=(MayaUsdContextOpsHandler&&) = delete;

    //! Create a MayaUsdContextOpsHandler.
    static MayaUsdContextOpsHandler::Ptr create();

    // UsdUfe::UsdContextOpsHandler overrides
    Ufe::ContextOps::Ptr contextOps(const Ufe::SceneItem::Ptr& item) const override;
}; // MayaUsdContextOpsHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
