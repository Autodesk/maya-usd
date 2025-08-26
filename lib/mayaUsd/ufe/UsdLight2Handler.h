//
// Copyright 2025 Autodesk
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
#ifndef MAYAUSD_USDLIGHT2HANDLER_H
#define MAYAUSD_USDLIGHT2HANDLER_H

#include <mayaUsd/base/api.h>

#include <ufe/light2Handler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to create a UsdLight2Handler interface object.
class MAYAUSD_CORE_PUBLIC UsdLight2Handler : public Ufe::Light2Handler
{
public:
    typedef std::shared_ptr<UsdLight2Handler> Ptr;

    UsdLight2Handler() = default;

    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdLight2Handler);

    //! Create a UsdLight2Handler.
    static UsdLight2Handler::Ptr create();

    // Ufe::LightHandler overrides
    Ufe::Light2::Ptr light(const Ufe::SceneItem::Ptr& item) const override;

}; // UsdLight2Handler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_USDLIGHT2HANDLER_H
