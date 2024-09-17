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
#ifndef USDUFE_USDATTRIBUTESHANDLER_H
#define USDUFE_USDATTRIBUTESHANDLER_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UsdAttributes.h>

#include <ufe/attributesHandler.h>

namespace USDUFE_NS_DEF {

//! \brief Interface to create the USD Attributes interface objects.
class USDUFE_PUBLIC UsdAttributesHandler : public Ufe::AttributesHandler
{
public:
    typedef std::shared_ptr<UsdAttributesHandler> Ptr;

    UsdAttributesHandler() = default;

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(UsdAttributesHandler);

    //! Create a UsdAttributesHandler.
    static UsdAttributesHandler::Ptr create();

    // Ufe::AttributesHandler overrides
    Ufe::Attributes::Ptr attributes(const Ufe::SceneItem::Ptr& item) const override;

private:
}; // UsdAttributesHandler

} // namespace USDUFE_NS_DEF

#endif // USDUFE_USDATTRIBUTESHANDLER_H
