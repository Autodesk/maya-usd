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

#include <ufe/hierarchyHandler.h>

#include <mayaUsd/base/api.h>
#include <mayaUsd/ufe/UsdHierarchy.h>
#include <mayaUsd/ufe/UsdRootChildHierarchy.h>

//PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief USD run-time hierarchy handler.
/*!
	This hierarchy handler is the standard USD run-time hierarchy handler.  Its
	only special behavior is to return a UsdRootChildHierarchy interface object
	if it is asked for a hierarchy interface for a child of the USD root prim.
	These prims are special because we define their parent to be the Maya USD
	gateway node, which the UsdRootChildHierarchy interface implements.
 */
class MAYAUSD_CORE_PUBLIC UsdHierarchyHandler : public Ufe::HierarchyHandler
{
public:
	typedef std::shared_ptr<UsdHierarchyHandler> Ptr;

	UsdHierarchyHandler();
	~UsdHierarchyHandler() override;

	// Delete the copy/move constructors assignment operators.
	UsdHierarchyHandler(const UsdHierarchyHandler&) = delete;
	UsdHierarchyHandler& operator=(const UsdHierarchyHandler&) = delete;
	UsdHierarchyHandler(UsdHierarchyHandler&&) = delete;
	UsdHierarchyHandler& operator=(UsdHierarchyHandler&&) = delete;

	//! Create a UsdHierarchyHandler.
	static UsdHierarchyHandler::Ptr create();

	// UsdHierarchyHandler overrides
	Ufe::Hierarchy::Ptr hierarchy(const Ufe::SceneItem::Ptr& item) const override;
	Ufe::SceneItem::Ptr createItem(const Ufe::Path& path) const override;
#if UFE_PREVIEW_VERSION_NUM >= 2022
    UFE_V2(Ufe::Hierarchy::ChildFilter childFilter() const override;)
#endif
}; // UsdHierarchyHandler

} // namespace ufe
} // namespace MayaUsd
