#ifndef USDUIINFOHANDLER_H
#define USDUIINFOHANDLER_H

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

#include <usdUfe/base/api.h>

#include <ufe/uiInfoHandler.h>

#include <array>
#include <map>
#include <string>

namespace USDUFE_NS_DEF {

//! \brief Implementation of Ufe::UIInfoHandler interface for USD objects.
class USDUFE_PUBLIC UsdUIInfoHandler : public Ufe::UIInfoHandler
{
public:
    typedef std::shared_ptr<UsdUIInfoHandler> Ptr;

    UsdUIInfoHandler();
    ~UsdUIInfoHandler() override;

    // Delete the copy/move constructors assignment operators.
    UsdUIInfoHandler(const UsdUIInfoHandler&) = delete;
    UsdUIInfoHandler& operator=(const UsdUIInfoHandler&) = delete;
    UsdUIInfoHandler(UsdUIInfoHandler&&) = delete;
    UsdUIInfoHandler& operator=(UsdUIInfoHandler&&) = delete;

    //! Create a UsdUIInfoHandler.
    static UsdUIInfoHandler::Ptr create();

    // Ufe::UIInfoHandler overrides
    bool treeViewCellInfo(const Ufe::SceneItem::Ptr& item, Ufe::CellInfo& info) const override;
    Ufe::UIInfoHandler::Icon treeViewIcon(const Ufe::SceneItem::Ptr& item) const override;
    std::string              treeViewTooltip(const Ufe::SceneItem::Ptr& item) const override;
    std::string              getLongRunTimeLabel() const override;

    // Helpers

    // Called from treeViewIcon() method to find the correct icon based on node type.
    // Get the map (node type -> icon filename) of the supported icon types.
    // Can be overridden by derived class to add to the map.
    typedef std::map<std::string, std::string> SupportedTypesMap;
    virtual SupportedTypesMap                  getSupportedIconTypes() const;

protected:
    // Derived classes can set this color to override the default invisible color.
    std::array<double, 3> fInvisibleColor;
}; // UsdUIInfoHandler

} // namespace USDUFE_NS_DEF

#endif // USDUIINFOHANDLER_H
