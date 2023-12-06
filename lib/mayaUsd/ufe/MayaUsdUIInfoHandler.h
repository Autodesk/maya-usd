#ifndef MAYAUSDUIINFOHANDLER_H
#define MAYAUSDUIINFOHANDLER_H

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

#include <mayaUsd/base/api.h>

#include <usdUfe/ufe/UsdUIInfoHandler.h>

#include <maya/MEventMessage.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Implementation of Ufe::UIInfoHandler interface for USD objects.
class MAYAUSD_CORE_PUBLIC MayaUsdUIInfoHandler : public UsdUfe::UsdUIInfoHandler
{
public:
    typedef UsdUfe::UsdUIInfoHandler              Parent;
    typedef std::shared_ptr<MayaUsdUIInfoHandler> Ptr;

    MayaUsdUIInfoHandler();
    ~MayaUsdUIInfoHandler() override;

    // Delete the copy/move constructors assignment operators.
    MayaUsdUIInfoHandler(const MayaUsdUIInfoHandler&) = delete;
    MayaUsdUIInfoHandler& operator=(const MayaUsdUIInfoHandler&) = delete;
    MayaUsdUIInfoHandler(MayaUsdUIInfoHandler&&) = delete;
    MayaUsdUIInfoHandler& operator=(MayaUsdUIInfoHandler&&) = delete;

    //! Create a MayaUsdUIInfoHandler.
    static MayaUsdUIInfoHandler::Ptr create();

    UsdUfe::UsdUIInfoHandler::SupportedTypesMap getSupportedIconTypes() const override;
    Ufe::UIInfoHandler::Icon treeViewIcon(const Ufe::SceneItem::Ptr& item) const override;

private:
    void updateInvisibleColor();

    // Note: the on-color-changed callback function is declared taking a void pointer
    //       to be compatible with MMessage callback API.
    static void onColorChanged(void*);

    MCallbackId fColorChangedCallbackId = 0;
}; // MayaUsdUIInfoHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSDUIINFOHANDLER_H
