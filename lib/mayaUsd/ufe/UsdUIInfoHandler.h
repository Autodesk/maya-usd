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

#include <ufe/uiInfoHandler.h>

#include <mayaUsd/base/api.h>


namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to create a UsdUIInfoHandler interface object.
class MAYAUSD_CORE_PUBLIC UsdUIInfoHandler : public Ufe::UIInfoHandler
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
#if UFE_PREVIEW_VERSION_NUM >= 2024
	Ufe::UIInfoHandler::Icon treeViewIcon(const Ufe::SceneItem::Ptr& item) const override;
	std::string treeViewTooltip(const Ufe::SceneItem::Ptr& item) const override;
#else
	std::string treeViewIcon(const Ufe::SceneItem::Ptr& item) const override;
#endif
	std::string getLongRunTimeLabel() const override;
}; // UsdUIInfoHandler

} // namespace ufe
} // namespace MayaUsd

#endif // USDUIINFOHANDLER_H
