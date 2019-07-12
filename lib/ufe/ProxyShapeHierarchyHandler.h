// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#pragma once

#include "../base/api.h"

#include "ufe/hierarchyHandler.h"
#include "ufe/ProxyShapeHierarchy.h"

//PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Maya run-time hierarchy handler with support for USD gateway node.
/*!
    This hierarchy handler is NOT a USD run-time hierarchy handler: it is a
    Maya run-time hierarchy handler.  It decorates the standard Maya run-time
    hierarchy handler and replaces it, providing special behavior only if the
    requested hierarchy interface is for the Maya to USD gateway node.  In that
    case, it returns a special ProxyShapeHierarchy interface object, which
    knows how to handle USD children of the Maya ProxyShapeHierarchy node.

    For all other Maya nodes, this hierarchy handler simply delegates the work
    to the standard Maya hierarchy handler it decorates, which returns a
    standard Maya hierarchy interface object.
 */
class MAYAUSD_CORE_PUBLIC ProxyShapeHierarchyHandler : public Ufe::HierarchyHandler
{
public:
	typedef std::shared_ptr<ProxyShapeHierarchyHandler> Ptr;

	ProxyShapeHierarchyHandler(Ufe::HierarchyHandler::Ptr mayaHierarchyHandler);
	~ProxyShapeHierarchyHandler() override;

	// Delete the copy/move constructors assignment operators.
	ProxyShapeHierarchyHandler(const ProxyShapeHierarchyHandler&) = delete;
	ProxyShapeHierarchyHandler& operator=(const ProxyShapeHierarchyHandler&) = delete;
	ProxyShapeHierarchyHandler(ProxyShapeHierarchyHandler&&) = delete;
	ProxyShapeHierarchyHandler& operator=(ProxyShapeHierarchyHandler&&) = delete;

	//! Create a ProxyShapeHierarchyHandler from a UFE hierarchy handler.
	static ProxyShapeHierarchyHandler::Ptr create(Ufe::HierarchyHandler::Ptr mayaHierarchyHandler);

	// Ufe::HierarchyHandler overrides
	Ufe::Hierarchy::Ptr hierarchy(const Ufe::SceneItem::Ptr& item) const override;
	Ufe::SceneItem::Ptr createItem(const Ufe::Path& path) const override;

private:
	Ufe::HierarchyHandler::Ptr fMayaHierarchyHandler;
	ProxyShapeHierarchy::Ptr fProxyShapeHierarchy;

}; // ProxyShapeHierarchyHandler

} // namespace ufe
} // namespace MayaUsd
