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
#pragma once

#include <ufe/transform3dHandler.h>

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdTransform3d.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to create a UsdTransform3d interface object.
class MAYAUSD_CORE_PUBLIC UsdTransform3dHandler : public Ufe::Transform3dHandler
{
public:
	typedef std::shared_ptr<UsdTransform3dHandler> Ptr;

	UsdTransform3dHandler();
	~UsdTransform3dHandler();

	// Delete the copy/move constructors assignment operators.
	UsdTransform3dHandler(const UsdTransform3dHandler&) = delete;
	UsdTransform3dHandler& operator=(const UsdTransform3dHandler&) = delete;
	UsdTransform3dHandler(UsdTransform3dHandler&&) = delete;
	UsdTransform3dHandler& operator=(UsdTransform3dHandler&&) = delete;

	//! Create a UsdTransform3dHandler.
	static UsdTransform3dHandler::Ptr create();

	// Ufe::Transform3dHandler overrides
	Ufe::Transform3d::Ptr transform3d(const Ufe::SceneItem::Ptr& item) const override;

}; // UsdTransform3dHandler

} // namespace ufe
} // namespace MayaUsd
