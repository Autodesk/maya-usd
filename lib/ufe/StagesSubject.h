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
