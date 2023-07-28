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

#include <usdUfe/base/api.h>

#include <pxr/base/tf/notice.h>
#include <pxr/base/tf/weakBase.h>
#include <pxr/usd/usd/notice.h>
#include <pxr/usd/usd/stage.h>

#include <ufe/path.h>
#include <ufe/sceneItem.h>

namespace USDUFE_NS_DEF {

//! \brief Subject class to observe USD stage changes.
/*!
        This USD observer translates USD notifications into UFE notifications.

        A client should derive their own version from this class and then
        when a gateway node is created connect that stage to USD notifications.
        Example:
            key1 = TfNotice::Register(me, &StagesSubject::stageChanged, stage);
            key2 = TfNotice::Register(me, &StagesSubject::stageEditTargetChanged, stage);

        When a new scene is created/opened in the DCC you should remove the
        TfNotice registration by calling:
            TfNotice::Revoke(key1);
            TfNotice::Revoke(key2);

        See the MayaUsd implmentation for more details:
        https://github.com/Autodesk/maya-usd/blob/dev/lib/mayaUsd/ufe/MayaStagesSubject.cpp
 */
class USDUFE_PUBLIC StagesSubject
    : public PXR_NS::TfRefBase
    , public PXR_NS::TfWeakBase
{
public:
    typedef PXR_NS::TfWeakPtr<StagesSubject> Ptr;
    typedef PXR_NS::TfRefPtr<StagesSubject>  RefPtr;

    //! Constructor
    StagesSubject();

    //! Destructor
    virtual ~StagesSubject();

    //! Create the StagesSubject.
    static StagesSubject::RefPtr create();

    // Delete the copy/move constructors assignment operators.
    StagesSubject(const StagesSubject&) = delete;
    StagesSubject& operator=(const StagesSubject&) = delete;
    StagesSubject(StagesSubject&&) = delete;
    StagesSubject& operator=(StagesSubject&&) = delete;

    // Ufe notification helpers - send notification trapping any exception.
    void sendObjectAdd(const Ufe::SceneItem::Ptr& sceneItem) const;
    void sendObjectPostDelete(const Ufe::SceneItem::Ptr& sceneItem) const;
    void sendObjectDestroyed(const Ufe::Path& ufePath) const;
    void sendSubtreeInvalidate(const Ufe::SceneItem::Ptr& sceneItem) const;

protected:
    //! Call the stageChanged() methods on stage observers.
    virtual void stageChanged(
        PXR_NS::UsdNotice::ObjectsChanged const& notice,
        PXR_NS::UsdStageWeakPtr const&           sender);

    //! Call the stageEditTargetChanged() methods on stage observers.
    virtual void stageEditTargetChanged(
        PXR_NS::UsdNotice::StageEditTargetChanged const& notice,
        PXR_NS::UsdStageWeakPtr const&                   sender);

}; // StagesSubject

//! \brief Guard to delay attribute changed notifications.
/*!
        Instantiating an object of this class allows the attribute changed
        notifications to be delayed until the guard expires.

        The guard collapses down notifications for a given UFE path, which is
        desirable to avoid duplicate notifications.  However, it is an error to
        have notifications for more than one attribute within a single guard.
 */
class USDUFE_PUBLIC AttributeChangedNotificationGuard
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

} // namespace USDUFE_NS_DEF
