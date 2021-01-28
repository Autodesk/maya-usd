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

#include <mayaUsd/base/api.h>
#include <mayaUsd/listeners/proxyShapeNotice.h>

#include <pxr/base/tf/hash.h>
#include <pxr/base/tf/notice.h>
#include <pxr/base/tf/weakBase.h>
#include <pxr/usd/usd/notice.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MCallbackIdArray.h>
#include <ufe/path.h>
#include <ufe/ufe.h> // For UFE_V2_FEATURES_AVAILABLE

#include <unordered_set>

PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {
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

#ifdef UFE_V2_FEATURES_AVAILABLE
    //! Call the stageEditTargetChanged() methods on stage observers.
    void stageEditTargetChanged(
        UsdNotice::StageEditTargetChanged const& notice,
        UsdStageWeakPtr const&                   sender);
#endif

private:
    // Notice listener method for proxy stage set
    void onStageSet(const MayaUsdProxyStageSetNotice& notice);

    // Notice listener method for proxy stage invalidate.
    void onStageInvalidate(const MayaUsdProxyStageInvalidateNotice& notice);

    // Array of Notice::Key for registered listener
#ifdef UFE_V2_FEATURES_AVAILABLE
    using NoticeKeys = std::array<TfNotice::Key, 2>;
#else
    using NoticeKeys = std::array<TfNotice::Key, 1>;
#endif

    // Map of per-stage listeners, indexed by stage.
    typedef TfHashMap<UsdStageWeakPtr, NoticeKeys, TfHash> StageListenerMap;
    StageListenerMap                                       fStageListeners;

    /*! \brief  Store invalidated ufe paths during dirty propagation.

       We need to delay notification till stage changes, but at that time it could be too costly to
       discover what changed in the stage map. Instead, we store all gateway notes that changed
       during dirty propagation and send invalidation from compute, when the new stage is set. This
       cache is only useful between onStageInvalidate and onStageSet notifications.
    */
    std::unordered_set<Ufe::Path> fInvalidStages;

    bool fBeforeNewCallback = false;

    MCallbackIdArray fCbIds;

}; // StagesSubject

#ifdef UFE_V2_FEATURES_AVAILABLE
//! \brief Guard to delay attribute changed notifications.
/*!
        Instantiating an object of this class allows the attribute changed
        notifications to be delayed until the guard expires.

    The guard collapses down notifications for a given UFE path, which is
        desirable to avoid duplicate notifications.  However, it is an error to
        have notifications for more than one attribute within a single guard.
 */
class MAYAUSD_CORE_PUBLIC AttributeChangedNotificationGuard
{
public:
    AttributeChangedNotificationGuard();
    ~AttributeChangedNotificationGuard();

    //@{
    //! Cannot be copied or assigned.
    AttributeChangedNotificationGuard(const AttributeChangedNotificationGuard&) = delete;
    const AttributeChangedNotificationGuard& operator&(const AttributeChangedNotificationGuard&)
        = delete;
    //@}
};
#endif

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
