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

#include "../base/api.h"

#include "UsdAttributes.h"

#include <ufe/attributesHandler.h>

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface to create the USD Attributes interface objects.
class UsdAttributesHandler : public Ufe::AttributesHandler
{
public:
	typedef std::shared_ptr<UsdAttributesHandler> Ptr;

	UsdAttributesHandler();
	~UsdAttributesHandler();

	// Delete the copy/move constructors assignment operators.
	UsdAttributesHandler(const UsdAttributesHandler&) = delete;
	UsdAttributesHandler& operator=(const UsdAttributesHandler&) = delete;
	UsdAttributesHandler(UsdAttributesHandler&&) = delete;
	UsdAttributesHandler& operator=(UsdAttributesHandler&&) = delete;

	//! Create a UsdAttributesHandler.
	static UsdAttributesHandler::Ptr create();

	// Ufe::AttributesHandler overrides
	Ufe::Attributes::Ptr attributes(const Ufe::SceneItem::Ptr& item) const override;

private:

}; // UsdAttributesHandler

} // namespace ufe
} // namespace MayaUsd
