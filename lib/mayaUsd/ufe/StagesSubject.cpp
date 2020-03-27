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

#include "StagesSubject.h"
#include "Utils.h"
#include "UsdStageMap.h"
#include "ProxyShapeHandler.h"
#include "private/InPathChange.h"

#ifdef UFE_V2_FEATURES_AVAILABLE
#include <ufe/attributes.h>
#endif
#include <ufe/hierarchy.h>
#include <ufe/path.h>
#include <ufe/scene.h>
#include <ufe/sceneNotification.h>
#include <ufe/transform3d.h>
#ifdef UFE_V2_FEATURES_AVAILABLE
#include <ufe/object3d.h>
#if UFE_PREVIEW_VERSION_NUM >= 2010
#include <ufe/object3dNotification.h>
#endif
#endif

#include <maya/MSceneMessage.h>
#include <maya/MMessage.h>

#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/xformOp.h>

#include <vector>

#ifdef UFE_V2_FEATURES_AVAILABLE
#include <unordered_map>

namespace {

// The attribute change notification guard is not meant to be nested, but
// use a counter nonetheless to provide consistent behavior in such cases.
int attributeChangedNotificationGuardCount = 0;

bool inAttributeChangedNotificationGuard()
{
    return attributeChangedNotificationGuardCount > 0;
}

std::unordered_map<Ufe::Path, std::string> pendingAttributeChangedNotifications;

}
#endif

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

	TfWeakPtr<StagesSubject> me(this);
	TfNotice::Register(me, &StagesSubject::onStageSet);
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

	// Set up our stage to proxy shape UFE path (and reverse)
	// mapping.  We do this with the following steps:
	// - get all proxyShape nodes in the scene.
	// - get their Dag paths.
	// - convert the Dag paths to UFE paths.
	// - get their stage.
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
		// Assume proxy shapes (and thus stages) cannot be instanced.  We can
		// therefore map the stage to a single UFE path.  Lifting this
		// restriction would mean sending one add or delete notification for
		// each Maya Dag path instancing the proxy shape / stage.
		Ufe::Path ufePath = stagePath(sender) + Ufe::PathSegment(usdPrimPathStr, g_USDRtid, '/');
		auto prim = stage->GetPrimAtPath(changedPath);
		// Changed paths could be xformOps.
		// These are considered as invalid null prims
		if (prim.IsValid() && !InPathChange::inPathChange())
		{
			auto sceneItem = Ufe::Hierarchy::createItem(ufePath);

			// AL LayerCommands.addSubLayer test will cause Maya to crash
			// if we don't filter invalid sceneItems. This patch is provided
			// to prevent crashes, but more investigation will have to be
			// done to understand why ufePath in case of sub layer
			// creation causes Ufe::Hierarchy::createItem to fail.
			if (!sceneItem)
				continue;

			if (prim.IsActive())
			{
				auto notification = Ufe::ObjectAdd(sceneItem);
				Ufe::Scene::notifyObjectAdd(notification);
			}
			else
			{
				auto notification = Ufe::ObjectPostDelete(sceneItem);
				Ufe::Scene::notifyObjectDelete(notification);
			}
		}
	}

	for (const auto& changedPath : notice.GetChangedInfoOnlyPaths())
	{
		auto usdPrimPathStr = changedPath.GetPrimPath().GetString();
		auto ufePath = stagePath(sender) + Ufe::PathSegment(usdPrimPathStr, g_USDRtid, '/');

#ifdef UFE_V2_FEATURES_AVAILABLE
		// isPrimPropertyPath() does not consider relational attributes
		// isPropertyPath() does consider relational attributes
		// isRelationalAttributePath() considers only relational attributes
		if (changedPath.IsPrimPropertyPath()) {
			if (inAttributeChangedNotificationGuard()) {
				pendingAttributeChangedNotifications[ufePath] =
					changedPath.GetName();
			}
			else {
				Ufe::Attributes::notify(ufePath, changedPath.GetName());
			}
		}

#if UFE_PREVIEW_VERSION_NUM >= 2010
		// Send a special message when visibility has changed.
		if (changedPath.GetNameToken() == UsdGeomTokens->visibility)
		{
			Ufe::VisibilityChanged vis(ufePath);
			Ufe::Object3d::notify(vis);
		}
#endif
#endif

		// We need to determine if the change is a Transform3d change.
		// We must at least pick up xformOp:translate, xformOp:rotateXYZ, 
		// and xformOp:scale.
		if(UsdGeomXformOp::IsXformOp(changedPath.GetNameToken()))
		{
			Ufe::Transform3d::notify(ufePath);
		}
	}
}

void StagesSubject::onStageSet(const UsdMayaProxyStageSetNotice& notice)
{
	afterOpen();
}

#ifdef UFE_V2_FEATURES_AVAILABLE
AttributeChangedNotificationGuard::AttributeChangedNotificationGuard()
{
	if (inAttributeChangedNotificationGuard()) {
		TF_CODING_ERROR("Attribute changed notification guard cannot be nested.");
	}

	if (attributeChangedNotificationGuardCount == 0 &&
		!pendingAttributeChangedNotifications.empty()) {
		TF_CODING_ERROR("Stale pending attribute changed notifications.");
	}

	++attributeChangedNotificationGuardCount;

}

AttributeChangedNotificationGuard::~AttributeChangedNotificationGuard()
{
	if (--attributeChangedNotificationGuardCount < 0) {
		TF_CODING_ERROR("Corrupt attribute changed notification guard.");
	}

	if (attributeChangedNotificationGuardCount > 0 ) {
		return;
	}

	for (const auto& notificationInfo : pendingAttributeChangedNotifications) {
		Ufe::Attributes::notify(notificationInfo.first, notificationInfo.second);
	}

	pendingAttributeChangedNotifications.clear();
}
#endif

} // namespace ufe
} // namespace MayaUsd
