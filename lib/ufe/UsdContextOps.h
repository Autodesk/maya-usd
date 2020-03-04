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

#include "../base/api.h"

#include "UsdSceneItem.h"

#include <ufe/path.h>
#include <ufe/contextOps.h>

#include <pxr/usd/usd/prim.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Interface for scene item context operations.
/*!
    This class defines the interface that USD run-time implements to
    provide contextual operation support (example Outliner context menu).

    This class is not copy-able, nor move-able.

    \see UFE ContextOps class documentation for more details
*/
class MAYAUSD_CORE_PUBLIC UsdContextOps : public Ufe::ContextOps
{
public:
	typedef std::shared_ptr<UsdContextOps> Ptr;

	UsdContextOps();
	~UsdContextOps() override;

	// Delete the copy/move constructors assignment operators.
	UsdContextOps(const UsdContextOps&) = delete;
	UsdContextOps& operator=(const UsdContextOps&) = delete;
	UsdContextOps(UsdContextOps&&) = delete;
	UsdContextOps& operator=(UsdContextOps&&) = delete;

	//! Create a UsdContextOps.
	static UsdContextOps::Ptr create();

	void setItem(const UsdSceneItem::Ptr& item);
	const Ufe::Path& path() const;

	// Ufe::ContextOps overrides
	Ufe::SceneItem::Ptr sceneItem() const override;
    Items getItems(const ItemPath& itemPath) const override;
    Ufe::UndoableCommand::Ptr doOpCmd(const ItemPath& itemPath) override;

private:
	UsdSceneItem::Ptr fItem;
	UsdPrim fPrim;

}; // UsdContextOps

} // namespace ufe
} // namespace MayaUsd
