// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#pragma once

#include "../base/api.h"

#include "pxr/pxr.h"
#include "pxr/usd/usd/prim.h"

#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

/*!
	Proxy shape abstraction, to support use of USD proxy shape with any plugin
	that has a proxy shape derived from MayaUsdProxyShapeBase.
 */
class MAYAUSD_CORE_PUBLIC ProxyShapeHandler
{
public:
	ProxyShapeHandler() = default;
	~ProxyShapeHandler() = default;

	// Delete the copy/move constructors assignment operators.
	ProxyShapeHandler(const ProxyShapeHandler&) = delete;
	ProxyShapeHandler& operator=(const ProxyShapeHandler&) = delete;
	ProxyShapeHandler(ProxyShapeHandler&&) = delete;
	ProxyShapeHandler& operator=(ProxyShapeHandler&&) = delete;

	//! \return Type of the Maya shape node at the root of a USD hierarchy.
	static const std::string& gatewayNodeType();

	static std::vector<std::string> getAllNames();

	static UsdStageWeakPtr dagPathToStage(const std::string& dagPath);

	static std::vector<UsdStageRefPtr> getAllStages();

private:
	static const std::string fMayaUsdGatewayNodeType;

}; // ProxyShapeHandler

} // namespace ufe
} // namespace MayaUsd
