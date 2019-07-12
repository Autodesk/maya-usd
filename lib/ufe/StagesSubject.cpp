// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "StagesSubject.h"
#include "Utils.h"
#include "UsdStageMap.h"
#include "ProxyShapeHandler.h"
#include "private/InPathChange.h"

#include "ufe/path.h"
#include "ufe/hierarchy.h"
#include "ufe/scene.h"
#include "ufe/sceneNotification.h"
#include "ufe/transform3d.h"

#include "maya/MSceneMessage.h"
#include "maya/MMessage.h"

#include <vector>

MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Global variables & macros
//------------------------------------------------------------------------------
extern UsdStageMap g_StageMap;
extern Ufe::Rtid g_USDRtid;

//------------------------------------------------------------------------------
// StagesSubject
//------------------------------------------------------------------------------

StagesSubject::StagesSubject()
{
	// Workaround to MAYA-65920: at startup, MSceneMessage.kAfterNew file
	// callback is incorrectly called by Maya before the
	// MSceneMessage.kBeforeNew file callback, which should be illegal.
	// Detect this and ignore illegal calls to after new file callbacks.
	// PPT, 19-Jan-16.
	fBeforeNewCallback = false;

	MStatus res;
	fCbIds.append(MSceneMessage::addCallback(
					MSceneMessage::kBeforeNew, beforeNewCallback, this, &res));
	CHECK_MSTATUS(res);
	fCbIds.append(MSceneMessage::addCallback(
					MSceneMessage::kBeforeOpen, beforeOpenCallback, this, &res));
	CHECK_MSTATUS(res);
	fCbIds.append(MSceneMessage::addCallback(
					MSceneMessage::kAfterOpen, afterOpenCallback, this, &res));
	CHECK_MSTATUS(res);
	fCbIds.append(MSceneMessage::addCallback(
					MSceneMessage::kAfterNew, afterNewCallback, this, &res));
	CHECK_MSTATUS(res);
}

StagesSubject::~StagesSubject()
{
	MMessage::removeCallbacks(fCbIds);
	fCbIds.clear();
}

/*static*/
StagesSubject::Ptr StagesSubject::create()
{
	return TfCreateWeakPtr(new StagesSubject);
}

bool StagesSubject::beforeNewCallback() const
{
	return fBeforeNewCallback;
}

void StagesSubject::beforeNewCallback(bool b)
{
	fBeforeNewCallback = b;
}

/*static*/
void StagesSubject::beforeNewCallback(void* clientData)
{
	StagesSubject* ss = static_cast<StagesSubject*>(clientData);
	ss->beforeNewCallback(true);
}

/*static*/
void StagesSubject::beforeOpenCallback(void* clientData)
{
	//StagesSubject* ss = static_cast<StagesSubject*>(clientData);
	StagesSubject::beforeNewCallback(clientData);
}

/*static*/
void StagesSubject::afterNewCallback(void* clientData)
{
	StagesSubject* ss = static_cast<StagesSubject*>(clientData);

	// Workaround to MAYA-65920: detect and avoid illegal callback sequence.
	if (!ss->beforeNewCallback())
		return;

	ss->beforeNewCallback(false);
	StagesSubject::afterOpenCallback(clientData);
}

/*static*/
void StagesSubject::afterOpenCallback(void* clientData)
{
	StagesSubject* ss = static_cast<StagesSubject*>(clientData);
	ss->afterOpen();
}

void StagesSubject::afterOpen()
{
	// Observe stage changes, for all stages.  Return listener object can
	// optionally be used to call Revoke() to remove observation, but must
	// keep reference to it, otherwise its reference count is immediately
	// decremented, falls to zero, and no observation occurs.

	// Ideally, we would observe the data model only if there are observers,
	// to minimize cost of observation.  However, since observation is
	// frequent, we won't implement this for now.  PPT, 22-Dec-2017.
	std::for_each(std::begin(fStageListeners), std::end(fStageListeners),
		[](StageListenerMap::value_type element) { TfNotice::Revoke(element.second); } );

	StagesSubject::Ptr me(this);
	for (auto stage : ProxyShapeHandler::getAllStages())
	{
		fStageListeners[stage] = TfNotice::Register(
			me, &StagesSubject::stageChanged, stage);
	}

	// Set up our stage to AL_usdmaya_ProxyShape UFE path (and reverse)
	// mapping.  We do this with the following steps:
	// - get all proxyShape nodes in the scene.
	// - get their AL Python wrapper
	// - get their Dag paths
	// - convert the Dag paths to UFE paths.
	g_StageMap.clear();
	auto proxyShapeNames = ProxyShapeHandler::getAllNames();
	for (const auto& psn : proxyShapeNames)
	{
		MDagPath dag = nameToDagPath(psn);
		Ufe::Path ufePath = dagPathToUfe(dag);
		auto stage = ProxyShapeHandler::dagPathToStage(psn);
		g_StageMap.addItem(ufePath, stage);
	}
}

void StagesSubject::stageChanged(UsdNotice::ObjectsChanged const& notice, UsdStageWeakPtr const& sender)
{
	// If the stage path has not been initialized yet, do nothing 
	if (stagePath(sender).empty())
		return;

	auto stage = notice.GetStage();
	for (const auto& changedPath : notice.GetResyncedPaths())
	{
		const std::string& usdPrimPathStr = changedPath.GetPrimPath().GetString();
		Ufe::Path ufePath = stagePath(sender) + Ufe::PathSegment(usdPrimPathStr, g_USDRtid, '/');
		auto prim = stage->GetPrimAtPath(changedPath);
		// Changed paths could be xformOps.
		// These are considered as invalid null prims
		if (prim.IsValid() && !InPathChange::inPathChange())
		{
			if (prim.IsActive())
			{
				auto notification = Ufe::ObjectAdd(Ufe::Hierarchy::createItem(ufePath));
				Ufe::Scene::notifyObjectAdd(notification);
			}
			else
			{
				auto notification = Ufe::ObjectPostDelete(Ufe::Hierarchy::createItem(ufePath));
				Ufe::Scene::notifyObjectDelete(notification);
			}
		}
	}

	for (const auto& changedPath : notice.GetChangedInfoOnlyPaths())
	{
		auto usdPrimPathStr = changedPath.GetPrimPath().GetString();
		auto ufePath = stagePath(sender) + Ufe::PathSegment(usdPrimPathStr, g_USDRtid, '/');
		// We need to determine if the change is a Transform3d change.
		// We must at least pick up xformOp:translate, xformOp:rotateXYZ, 
		// and xformOp:scale.
		static std::string xformOp(".xformOp:");
		if (changedPath.GetElementString().substr(0, xformOp.length()) == xformOp)
		{
			Ufe::Transform3d::notify(ufePath);
		}
	}
}

} // namespace ufe
} // namespace MayaUsd
