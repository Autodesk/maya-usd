// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#pragma once

#include "../base/api.h"
#include "../listeners/proxyShapeNotice.h"

#include "pxr/base/tf/weakBase.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/notice.h"
#include "pxr/usd/usd/notice.h"

#include "maya/MCallbackIdArray.h"

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Subject class to observe Maya scene.
/*!
	This class observes Maya file open, to register a USD observer on each
	stage the Maya scene contains.  This USD observer translates USD
	notifications into UFE notifications.
 */
class MAYAUSD_CORE_PUBLIC StagesSubject : public TfWeakBase
{
public:
	typedef TfWeakPtr<StagesSubject> Ptr;

	//! Constructor
	StagesSubject();

	//! Destructor
	~StagesSubject();

	//! Create the StagesSubject.
	static StagesSubject::Ptr create();

	// Delete the copy/move constructors assignment operators.
	StagesSubject(const StagesSubject&) = delete;
	StagesSubject& operator=(const StagesSubject&) = delete;
	StagesSubject(StagesSubject&&) = delete;
	StagesSubject& operator=(StagesSubject&&) = delete;

	bool beforeNewCallback() const;
	void beforeNewCallback(bool b);

	void afterOpen();

private:
	// Maya scene message callbacks
	static void beforeNewCallback(void* clientData);
	static void beforeOpenCallback(void* clientData);
	static void afterNewCallback(void* clientData);
	static void afterOpenCallback(void* clientData);

	//! Call the stageChanged() methods on stage observers.
	void stageChanged(UsdNotice::ObjectsChanged const& notice, UsdStageWeakPtr const& sender);

private:
	// Notice listener method for proxy stage set
	void onStageSet(const UsdMayaProxyStageSetNotice& notice);

	// Map of per-stage listeners, indexed by stage.
	typedef TfHashMap<UsdStageWeakPtr, TfNotice::Key, TfHash> StageListenerMap;
	StageListenerMap fStageListeners;

	bool fBeforeNewCallback = false;

	MCallbackIdArray fCbIds;

}; // StagesSubject

} // namespace ufe
} // namespace MayaUsd
