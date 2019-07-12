// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "ProxyShapeHandler.h"

#include "../utils/query.h"

#include "maya/MGlobal.h"
#include "maya/MString.h"
#include "maya/MStringArray.h"

MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------
const std::string ProxyShapeHandler::fMayaUsdGatewayNodeType = "mayaUsdProxyShapeBase";

//------------------------------------------------------------------------------
// ProxyShapeHandler
//------------------------------------------------------------------------------

/*static*/
const std::string& ProxyShapeHandler::gatewayNodeType()
{
	return fMayaUsdGatewayNodeType;
}

/*static*/
std::vector<std::string> ProxyShapeHandler::getAllNames()
{
	std::vector<std::string> names;
	MString cmd;
	MStringArray result;
	cmd.format("ls -type ^1s -long", fMayaUsdGatewayNodeType.c_str());
	if (MS::kSuccess == MGlobal::executeCommand(cmd, result))
	{
		for (MString& name : result)
		{
			names.push_back(name.asChar());
		}
	}
	return names;
}

/*static*/
UsdStageWeakPtr ProxyShapeHandler::dagPathToStage(const std::string& dagPath)
{
	return UsdMayaQuery::GetPrim(dagPath).GetStage();
}

/*static*/
std::vector<UsdStageRefPtr> ProxyShapeHandler::getAllStages()
{
	// According to Pixar, the following should work:
	//   return UsdMayaStageCache::Get().GetAllStages();
	// but after a file open of a scene with one or more Pixar proxy shapes,
	// returns an empty list.  To be investigated, PPT, 28-Feb-2019.

	// When using an unmodified AL plugin, the following line crashes
	// Maya, so it requires the AL proxy shape inheritance from
	// MayaUsdProxyShapeBase.  PPT, 12-Apr-2019.
	std::vector<UsdStageRefPtr> stages;
	for (const auto& name : getAllNames())
	{
		UsdStageWeakPtr stage = dagPathToStage(name);
		if (stage)
		{
			stages.push_back(stage);
		}
	}
	return stages;
}

} // namespace ufe
} // namespace MayaUsd
