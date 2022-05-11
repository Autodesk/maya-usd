#ifndef MAYAUIINFOHANDLER_H
#define MAYAUIINFOHANDLER_H

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

#include <mayaUsd/base/api.h>

#include <ufe/uiInfoHandler.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Implementation of Ufe::UIInfoHandler interface for Maya objects.
class MAYAUSD_CORE_PUBLIC MayaUIInfoHandler : public Ufe::UIInfoHandler
{
public:
    typedef std::shared_ptr<MayaUIInfoHandler> Ptr;

    MayaUIInfoHandler();
    ~MayaUIInfoHandler() override;

    // Delete the copy/move constructors assignment operators.
    MayaUIInfoHandler(const MayaUIInfoHandler&) = delete;
    MayaUIInfoHandler& operator=(const MayaUIInfoHandler&) = delete;
    MayaUIInfoHandler(MayaUIInfoHandler&&) = delete;
    MayaUIInfoHandler& operator=(MayaUIInfoHandler&&) = delete;

    //! Create a MayaUIInfoHandler.
    static MayaUIInfoHandler::Ptr create();

    // Ufe::UIInfoHandler overrides
    bool treeViewCellInfo(const Ufe::SceneItem::Ptr& item, Ufe::CellInfo& info) const override;
    Ufe::UIInfoHandler::Icon treeViewIcon(const Ufe::SceneItem::Ptr& item) const override;
    std::string              treeViewTooltip(const Ufe::SceneItem::Ptr& item) const override;
    std::string              getLongRunTimeLabel() const override;
}; // MayaUIInfoHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUIINFOHANDLER_H
