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

#include "private/UfeNotifGuard.h"

#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/ProxyShapeHandler.h>
#include <mayaUsd/ufe/UfeVersionCompat.h>
#ifdef UFE_V2_FEATURES_AVAILABLE
#include <mayaUsd/ufe/UsdCamera.h>
#endif
#include <mayaUsd/ufe/UsdStageMap.h>
#include <mayaUsd/ufe/Utils.h>
#ifdef UFE_V2_FEATURES_AVAILABLE
#include <mayaUsd/undo/UsdUndoManager.h>
#endif

#include <pxr/pxr.h>
#include <pxr/usd/sdf/schema.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/xformOp.h>

#include <maya/MMessage.h>
#include <maya/MSceneMessage.h>
#include <ufe/hierarchy.h>
#include <ufe/path.h>
#include <ufe/scene.h>
#include <ufe/sceneNotification.h>
#include <ufe/transform3d.h>

#include <atomic>
#include <limits>
#include <vector>

#ifdef UFE_V2_FEATURES_AVAILABLE
#include <ufe/attributes.h>
#include <ufe/object3d.h>
#include <ufe/object3dNotification.h>

#include <unordered_map>
#endif

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Prevent re-entrant stage set.
std::atomic_bool stageSetGuardCount { false };

bool isTransformChange(const TfToken& nameToken)
{
    return nameToken == UsdGeomTokens->xformOpOrder || UsdGeomXformOp::IsXformOp(nameToken);
}

// Prevent exception from the notifications from escaping and breaking USD/Maya.
// USD does not wrap its notification in try/catch, so we need to do it ourselves.
template <class RECEIVER, class NOTIFICATION>
void notifyWithoutExceptions(const NOTIFICATION& notif)
{
    try {
        RECEIVER::notify(notif);
    } catch (const std::exception& ex) {
        TF_WARN("Caught error during notification: %s", ex.what());
    }
}

#ifdef UFE_V2_FEATURES_AVAILABLE
// The attribute change notification guard is not meant to be nested, but
// use a counter nonetheless to provide consistent behavior in such cases.
std::atomic_int attributeChangedNotificationGuardCount { 0 };

enum class AttributeChangeType
{
    kAdded,
    kValueChanged,
    kConnectionChanged,
    kRemoved
};

struct AttributeNotification
{
    Ufe::Path           _path;
    TfToken             _token;
    AttributeChangeType _type;

    bool operator==(const AttributeNotification& other)
    {
        // Only collapse multiple value changes. Collapsing added/removed notifications needs to be
        // done safely so the observer ends up in the right state.
        return other._type == _type && other._token == _token && other._path == _path
            && _type == AttributeChangeType::kValueChanged;
    }
};

// Keep an array of the pending attribute notifications. Using a vector
// for two main reasons:
// 1) Order of notifs must be maintained.
// 2) Allow notif with same path, but different token. At worst the check is linear
//    in the size of the vector (which is the same as an unordered_multimap).
std::vector<AttributeNotification> pendingAttributeChangedNotifications;

bool inAttributeChangedNotificationGuard()
{
    return attributeChangedNotificationGuardCount.load() > 0;
}

#if (UFE_PREVIEW_VERSION_NUM >= 4024)
#define UFE_V4_24(...) __VA_ARGS__
#else
#define UFE_V4_24(...)
#endif

void sendAttributeChanged(
    const Ufe::Path&    ufePath,
    const TfToken&      changedToken,
    AttributeChangeType UFE_V4_24(changeType))
{
#if (UFE_PREVIEW_VERSION_NUM >= 4024)
    switch (changeType) {
    case AttributeChangeType::kValueChanged: {
        notifyWithoutExceptions<Ufe::Attributes>(
            Ufe::AttributeValueChanged(ufePath, changedToken.GetString()));

        if (MayaUsd::ufe::UsdCamera::isCameraToken(changedToken)) {
            notifyWithoutExceptions<Ufe::Camera>(ufePath);
        }
    } break;
    case AttributeChangeType::kAdded: {
        notifyWithoutExceptions<Ufe::Attributes>(
            Ufe::AttributeAdded(ufePath, changedToken.GetString()));
    } break;
    case AttributeChangeType::kRemoved: {
        notifyWithoutExceptions<Ufe::Attributes>(
            Ufe::AttributeRemoved(ufePath, changedToken.GetString()));
    } break;
    case AttributeChangeType::kConnectionChanged: {
        notifyWithoutExceptions<Ufe::Attributes>(
            Ufe::AttributeConnectionChanged(ufePath, changedToken.GetString()));
    } break;
    }
#else
    notifyWithoutExceptions<Ufe::Attributes>(
        Ufe::AttributeValueChanged(ufePath, changedToken.GetString()));

    if (MayaUsd::ufe::UsdCamera::isCameraToken(changedToken)) {
        notifyWithoutExceptions<Ufe::Camera>(ufePath);
    }
#endif
}

void valueChanged(const Ufe::Path& ufePath, const TfToken& changedToken)
{
    if (inAttributeChangedNotificationGuard()) {
        // Don't add pending notif if one already exists with same path/token.
        auto p = AttributeNotification { ufePath, changedToken, AttributeChangeType::kValueChanged };
        if (std::find(
                pendingAttributeChangedNotifications.begin(),
                pendingAttributeChangedNotifications.end(),
                p)
            == pendingAttributeChangedNotifications.end()) {
            pendingAttributeChangedNotifications.emplace_back(p);
        }
    } else {
        sendAttributeChanged(ufePath, changedToken, AttributeChangeType::kValueChanged);
    }
}

#if (UFE_PREVIEW_VERSION_NUM >= 4024)
void attributeChanged(
    const Ufe::Path&    ufePath,
    const TfToken&      changedToken,
    AttributeChangeType changeType)
{
    if (inAttributeChangedNotificationGuard()) {
        // Don't add pending notif if one already exists with same path/token.
        auto p = AttributeNotification { ufePath, changedToken, changeType };
        if (std::find(
                pendingAttributeChangedNotifications.begin(),
                pendingAttributeChangedNotifications.end(),
                p)
            == pendingAttributeChangedNotifications.end()) {
            pendingAttributeChangedNotifications.emplace_back(p);
        }
    } else {
        sendAttributeChanged(ufePath, changedToken, changeType);
    }
}
#endif

void processAttributeChanges(
    const Ufe::Path&                                ufePath,
    const SdfPath&                                  changedPath,
    const std::vector<const SdfChangeList::Entry*>& UFE_V4_24(entries))
{
#if (UFE_PREVIEW_VERSION_NUM >= 4024)
    bool sendValueChanged = false;
    bool sendAdded = false;
    bool sendRemoved = false;
    bool sendConnectionChanged = false;
    for (const auto& entry : entries) {
        if (entry->flags.didAddProperty || entry->flags.didAddPropertyWithOnlyRequiredFields) {
            sendAdded = true;
        } else if (
            entry->flags.didRemoveProperty
            || entry->flags.didRemovePropertyWithOnlyRequiredFields) {
            sendRemoved = true;
        } else if (entry->flags.didChangeAttributeConnection) {
            sendConnectionChanged = true;
        } else {
            sendValueChanged = true;
        }
    }
    if (sendAdded) {
        attributeChanged(ufePath, changedPath.GetNameToken(), AttributeChangeType::kAdded);
    }
    if (sendValueChanged) {
        valueChanged(ufePath, changedPath.GetNameToken());
    }
    if (sendConnectionChanged) {
        attributeChanged(
            ufePath, changedPath.GetNameToken(), AttributeChangeType::kConnectionChanged);
    }
    if (sendRemoved) {
        attributeChanged(ufePath, changedPath.GetNameToken(), AttributeChangeType::kRemoved);
    }
#else
    valueChanged(ufePath, changedPath.GetNameToken());
#endif
};

#endif

void sendObjectAdd(const Ufe::SceneItem::Ptr& sceneItem)
{
    try {
#ifdef UFE_V2_FEATURES_AVAILABLE
        Ufe::Scene::instance().notify(Ufe::ObjectAdd(sceneItem));
#else
        auto notification = Ufe::ObjectAdd(sceneItem);
        Ufe::Scene::notifyObjectAdd(notification);
#endif
    } catch (const std::exception& ex) {
        TF_WARN("Caught error during notification: %s", ex.what());
    }
}

void sendObjectPostDelete(const Ufe::SceneItem::Ptr& sceneItem)
{
    try {
#ifdef UFE_V2_FEATURES_AVAILABLE
        Ufe::Scene::instance().notify(Ufe::ObjectPostDelete(sceneItem));
#else
        auto notification = Ufe::ObjectPostDelete(sceneItem);
        Ufe::Scene::notifyObjectDelete(notification);
#endif
    } catch (const std::exception& ex) {
        TF_WARN("Caught error during notification: %s", ex.what());
    }
}

void sendObjectDestroyed(const Ufe::Path& ufePath)
{
    try {
#ifdef UFE_V2_FEATURES_AVAILABLE
        Ufe::Scene::instance().notify(Ufe::ObjectDestroyed(ufePath));
#else
        // Unfortunately in Ufe v1 there was no object destroyed notif
        // and the only delete notifs we have both take a scene item
        // which we cannot create for the input Ufe path since that
        // path is no longer valid (it has already been destroyed).
        // So the only choice we have is to subtree invalidate the
        // parent which will remove the destroyed item (keeping all
        // the valid children).
        auto sceneItem = Ufe::Hierarchy::createItem(ufePath.pop());
        if (TF_VERIFY(sceneItem)) {
            sendObjectPostDelete(sceneItem);
            sendObjectAdd(sceneItem);
        }
#endif
    } catch (const std::exception& ex) {
        TF_WARN("Caught error during notification: %s", ex.what());
    }
}

void sendSubtreeInvalidate(const Ufe::SceneItem::Ptr& sceneItem)
{
    try {
#ifdef UFE_V2_FEATURES_AVAILABLE
        Ufe::Scene::instance().notify(Ufe::SubtreeInvalidate(sceneItem));
#else
        // In Ufe v1 there was no subtree invalidate notif. So we mimic it by sending
        // delete/add notifs.
        sendObjectPostDelete(sceneItem);
        sendObjectAdd(sceneItem);
#endif
    } catch (const std::exception& ex) {
        TF_WARN("Caught error during notification: %s", ex.what());
    }
}

} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Global variables & macros
//------------------------------------------------------------------------------
extern UsdStageMap g_StageMap;
extern Ufe::Rtid   g_USDRtid;

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
    fCbIds.append(
        MSceneMessage::addCallback(MSceneMessage::kBeforeNew, beforeNewCallback, this, &res));
    CHECK_MSTATUS(res);
    fCbIds.append(
        MSceneMessage::addCallback(MSceneMessage::kBeforeOpen, beforeOpenCallback, this, &res));
    CHECK_MSTATUS(res);
    fCbIds.append(
        MSceneMessage::addCallback(MSceneMessage::kAfterOpen, afterOpenCallback, this, &res));
    CHECK_MSTATUS(res);
    fCbIds.append(
        MSceneMessage::addCallback(MSceneMessage::kAfterNew, afterNewCallback, this, &res));
    CHECK_MSTATUS(res);

    TfWeakPtr<StagesSubject> me(this);
    TfNotice::Register(me, &StagesSubject::onStageSet);
    TfNotice::Register(me, &StagesSubject::onStageInvalidate);
}

StagesSubject::~StagesSubject()
{
    MMessage::removeCallbacks(fCbIds);
    fCbIds.clear();
}

/*static*/
StagesSubject::Ptr StagesSubject::create() { return TfCreateWeakPtr(new StagesSubject); }

bool StagesSubject::beforeNewCallback() const { return fBeforeNewCallback; }

void StagesSubject::beforeNewCallback(bool b)
{
    fBeforeNewCallback = b;
    fInvalidStages.clear();
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
    // StagesSubject* ss = static_cast<StagesSubject*>(clientData);
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
    std::for_each(
        std::begin(fStageListeners),
        std::end(fStageListeners),
        [](StageListenerMap::value_type element) {
            for (auto& noticeKey : element.second) {
                TfNotice::Revoke(noticeKey);
            }
        });
    fStageListeners.clear();

    // Set up our stage to proxy shape UFE path (and reverse)
    // mapping.  We do this with the following steps:
    // - get all proxyShape nodes in the scene.
    // - get their Dag paths.
    // - convert the Dag paths to UFE paths.
    // - get their stage.
    g_StageMap.setDirty();
}

void StagesSubject::stageChanged(
    UsdNotice::ObjectsChanged const& notice,
    UsdStageWeakPtr const&           sender)
{
    // If the stage path has not been initialized yet, do nothing
    if (stagePath(sender).empty())
        return;

    auto stage = notice.GetStage();
    auto resyncPaths = notice.GetResyncedPaths();
    for (auto it = resyncPaths.begin(), end = resyncPaths.end(); it != end; ++it) {
        const auto& changedPath = *it;
        if (changedPath.IsPrimPropertyPath()) {
            // Special case to detect when an xformop is added or removed from a prim.
            // We need to send some notifs so Maya can update (such as on undo
            // to move the transform manipulator back to original position).
            const TfToken nameToken = changedPath.GetNameToken();
            auto          usdPrimPathStr = changedPath.GetPrimPath().GetString();
            auto ufePath = stagePath(sender) + Ufe::PathSegment(usdPrimPathStr, g_USDRtid, '/');
            if (isTransformChange(nameToken)) {
                if (!InTransform3dChange::inTransform3dChange()) {
                    notifyWithoutExceptions<Ufe::Transform3d>(ufePath);
                }
            }
            UFE_V2(processAttributeChanges(ufePath, changedPath, it.base()->second);)
            // No further processing for this prim property path is required.
            continue;
        }

        // Relational attributes will not be caught by the IsPrimPropertyPath()
        // and we don't care about them.
        if (changedPath.IsPropertyPath())
            continue;

        // Assume proxy shapes (and thus stages) cannot be instanced.  We can
        // therefore map the stage to a single UFE path.  Lifting this
        // restriction would mean sending one add or delete notification for
        // each Maya Dag path instancing the proxy shape / stage.
        Ufe::Path ufePath;
        UsdPrim   prim;
        if (changedPath == SdfPath::AbsoluteRootPath()) {
            ufePath = stagePath(sender);
            prim = stage->GetPseudoRoot();
        } else {
            const std::string& usdPrimPathStr = changedPath.GetPrimPath().GetString();
            ufePath = stagePath(sender) + Ufe::PathSegment(usdPrimPathStr, g_USDRtid, '/');
            prim = stage->GetPrimAtPath(changedPath);
        }

        if (prim.IsValid() && !InPathChange::inPathChange()) {
            auto sceneItem = Ufe::Hierarchy::createItem(ufePath);

            // AL LayerCommands.addSubLayer test will cause Maya to crash
            // if we don't filter invalid sceneItems. This patch is provided
            // to prevent crashes, but more investigation will have to be
            // done to understand why ufePath in case of sub layer
            // creation causes Ufe::Hierarchy::createItem to fail.
            if (!sceneItem)
                continue;

#ifndef MAYA_ENABLE_NEW_PRIM_DELETE
            // Special case when we know the operation came from either
            // the add or delete of our UFE/USD implementation.
            if (InAddOrDeleteOperation::inAddOrDeleteOperation()) {
                if (prim.IsActive()) {
                    sendObjectAdd(sceneItem);
                } else {
                    sendObjectPostDelete(sceneItem);
                }
            } else {
#endif
                // Use the entry flags in the USD notice to know what operation was performed and
                // thus what Ufe notif to send.
                const std::vector<const SdfChangeList::Entry*>& entries = it.base()->second;
                bool                                            sentNotif { false };
                for (const auto& entry : entries) {
                    if (entry->flags.didAddInertPrim || entry->flags.didAddNonInertPrim) {
                        sendObjectAdd(sceneItem);
                        sentNotif = true;
                        break;
                    } else if (
                        entry->flags.didRemoveInertPrim || entry->flags.didRemoveNonInertPrim) {
                        sendObjectPostDelete(sceneItem);
                        sentNotif = true;
                        break;
                    }

                    // Special case for "active" metadata.
                    if (entry->HasInfoChange(SdfFieldKeys->Active)) {
                        if (prim.IsActive()) {
                            sendObjectAdd(sceneItem);
                        } else {
                            sendObjectPostDelete(sceneItem);
                        }
                        sentNotif = true;
                        break;
                    }
                }

                if (!sentNotif) {
                    // According to USD docs for GetResyncedPaths():
                    // - Resyncs imply entire subtree invalidation of all descendant prims and
                    // properties. So we send the UFE subtree invalidate notif.
                    sendSubtreeInvalidate(sceneItem);
                }
#ifndef MAYA_ENABLE_NEW_PRIM_DELETE
            }
#endif
        } else if (!prim.IsValid() && !InPathChange::inPathChange()) {
            Ufe::SceneItem::Ptr sceneItem = Ufe::Hierarchy::createItem(ufePath);
            if (!sceneItem || InAddOrDeleteOperation::inAddOrDeleteOperation()) {
                sendObjectDestroyed(ufePath);
            } else {
                sendSubtreeInvalidate(sceneItem);
            }
        }
    }

    auto changedInfoOnlyPaths = notice.GetChangedInfoOnlyPaths();
    for (auto it = changedInfoOnlyPaths.begin(), end = changedInfoOnlyPaths.end(); it != end;
         ++it) {
        const auto& changedPath = *it;
        auto        usdPrimPathStr = changedPath.GetPrimPath().GetString();
        auto        ufePath = stagePath(sender) + Ufe::PathSegment(usdPrimPathStr, g_USDRtid, '/');

#ifdef UFE_V2_FEATURES_AVAILABLE
        bool sendValueChangedFallback = true;

        // isPrimPropertyPath() does not consider relational attributes
        // isPropertyPath() does consider relational attributes
        // isRelationalAttributePath() considers only relational attributes
        if (changedPath.IsPrimPropertyPath()) {
            processAttributeChanges(ufePath, changedPath, it.base()->second);
            sendValueChangedFallback = false;
        }

        // Send a special message when visibility has changed.
        if (changedPath.GetNameToken() == UsdGeomTokens->visibility) {
            Ufe::VisibilityChanged vis(ufePath);
            notifyWithoutExceptions<Ufe::Object3d>(vis);
            sendValueChangedFallback = false;
        }
#endif

        if (!InTransform3dChange::inTransform3dChange()) {
            // Is the change a Transform3d change?
            const UsdPrim prim = stage->GetPrimAtPath(changedPath.GetPrimPath());
            const TfToken nameToken = changedPath.GetNameToken();
            if (isTransformChange(nameToken)) {
                notifyWithoutExceptions<Ufe::Transform3d>(ufePath);
                UFE_V2(sendValueChangedFallback = false;)
            } else if (prim && prim.IsA<UsdGeomPointInstancer>()) {
                // If the prim at the changed path is a PointInstancer, check
                // whether the modified path is one of the attributes authored
                // by point instance manipulation.
                if (nameToken == UsdGeomTokens->orientations
                    || nameToken == UsdGeomTokens->positions
                    || nameToken == UsdGeomTokens->scales) {
                    // This USD change represents a Transform3d change to a
                    // PointInstancer prim.
                    // Unfortunately though, there is no way for us to know
                    // which point instance indices were actually affected by
                    // this change. As a result, we must assume that they *all*
                    // may have been affected, so we construct UFE paths for
                    // every instance and issue a notification for each one.
                    const UsdGeomPointInstancer pointInstancer(prim);

#if PXR_VERSION >= 2011
                    const size_t numInstances
                        = bool(pointInstancer) ? pointInstancer.GetInstanceCount() : 0u;
#else
                    VtIntArray protoIndices;
                    if (pointInstancer) {
                        const UsdAttribute protoIndicesAttr = pointInstancer.GetProtoIndicesAttr();
                        if (protoIndicesAttr) {
                            protoIndicesAttr.Get(&protoIndices);
                        }
                    }
                    const size_t numInstances = protoIndices.size();
#endif

                    // The PointInstancer schema can theoretically support as
                    // as many instances as can be addressed by size_t, but
                    // Hydra currently only represents the instanceIndex of
                    // instances using int. We clamp the number of instance
                    // indices to the largest possible int to ensure that we
                    // don't overflow.
                    const int numIndices
                        = (numInstances <= static_cast<size_t>(std::numeric_limits<int>::max()))
                        ? static_cast<int>(numInstances)
                        : std::numeric_limits<int>::max();

                    for (int instanceIndex = 0; instanceIndex < numIndices; ++instanceIndex) {
                        const Ufe::Path instanceUfePath = stagePath(sender)
                            + usdPathToUfePathSegment(changedPath.GetPrimPath(), instanceIndex);
                        notifyWithoutExceptions<Ufe::Transform3d>(instanceUfePath);
                    }
                    UFE_V2(sendValueChangedFallback = false;)
                }
            }
        }

#ifdef UFE_V2_FEATURES_AVAILABLE
        if (sendValueChangedFallback) {

            // check to see if there is an entry which Ufe should notify about.
            const std::vector<const SdfChangeList::Entry*>& entries = it.base()->second;
            for (const auto& entry : entries) {
                // Adding an inert prim means we created a primSpec for an ancestor of
                // a prim which has a real change to it.
                if (entry->flags.didAddInertPrim || entry->flags.didRemoveInertPrim)
                    continue;

                valueChanged(ufePath, changedPath.GetNameToken());
                // just send one notification
                break;
            }
        }
#endif
    }

#ifdef UFE_V2_FEATURES_AVAILABLE
    // Special case when we are notified, but no paths given.
    if (notice.GetResyncedPaths().empty() && notice.GetChangedInfoOnlyPaths().empty()) {
        auto                       ufePath = stagePath(sender);
        Ufe::AttributeValueChanged vc(ufePath, "/");
        notifyWithoutExceptions<Ufe::Attributes>(vc);
    }
#endif
}

#ifdef UFE_V2_FEATURES_AVAILABLE
void StagesSubject::stageEditTargetChanged(
    UsdNotice::StageEditTargetChanged const& notice,
    UsdStageWeakPtr const&                   sender)
{
    // Track the edit target layer's state
    UsdUndoManager::instance().trackLayerStates(notice.GetStage()->GetEditTarget().GetLayer());
}
#endif

void StagesSubject::onStageSet(const MayaUsdProxyStageSetNotice& notice)
{
#ifdef UFE_V2_FEATURES_AVAILABLE
    auto noticeStage = notice.GetStage();
    // Check if stage received from notice is valid. We could have cases where a ProxyShape has an
    // invalid stage.
    if (noticeStage) {
        // Track the edit target layer's state
        UsdUndoManager::instance().trackLayerStates(noticeStage->GetEditTarget().GetLayer());
    }
#endif

    // Handle re-entrant MayaUsdProxyShapeBase::compute; allow update only on first compute call.
    if (MayaUsdProxyShapeBase::in_compute > 1)
        return;

    // Handle re-entrant onStageSet
    bool expectedState = false;
    if (stageSetGuardCount.compare_exchange_strong(expectedState, true)) {
        // We should have no listeners and stage map is dirty.
        TF_VERIFY(g_StageMap.isDirty());
        TF_VERIFY(fStageListeners.empty());

        StagesSubject::Ptr me(this);
        for (auto stage : ProxyShapeHandler::getAllStages()) {

            NoticeKeys noticeKeys;

            noticeKeys[0] = TfNotice::Register(me, &StagesSubject::stageChanged, stage);
            UFE_V2(noticeKeys[1]
                   = TfNotice::Register(me, &StagesSubject::stageEditTargetChanged, stage);)
            fStageListeners[stage] = noticeKeys;
        }

        // Now we can send the notifications about stage change.
        for (auto& path : fInvalidStages) {
            Ufe::SceneItem::Ptr sceneItem = Ufe::Hierarchy::createItem(path);
            if (sceneItem) {
                sendSubtreeInvalidate(sceneItem);
            }
        }

        fInvalidStages.clear();

        stageSetGuardCount = false;
    }
}

void StagesSubject::onStageInvalidate(const MayaUsdProxyStageInvalidateNotice& notice)
{
    afterOpen();

    auto p = notice.GetProxyShape().ufePath();
    if (!p.empty()) {
        // We can't send notification to clients from dirty propagation.
        // Delay it till the new stage is actually set during compute.
        fInvalidStages.insert(p);
    }
}

#ifdef UFE_V2_FEATURES_AVAILABLE
AttributeChangedNotificationGuard::AttributeChangedNotificationGuard()
{
    if (inAttributeChangedNotificationGuard()) {
        TF_CODING_ERROR("Attribute changed notification guard cannot be nested.");
    }

    if (attributeChangedNotificationGuardCount.load() == 0
        && !pendingAttributeChangedNotifications.empty()) {
        TF_CODING_ERROR("Stale pending attribute changed notifications.");
    }

    ++attributeChangedNotificationGuardCount;
}

AttributeChangedNotificationGuard::~AttributeChangedNotificationGuard()
{
    if (--attributeChangedNotificationGuardCount < 0) {
        TF_CODING_ERROR("Corrupt attribute changed notification guard.");
    }

    if (attributeChangedNotificationGuardCount.load() > 0) {
        return;
    }

    for (const auto& notificationInfo : pendingAttributeChangedNotifications) {
        sendAttributeChanged(
            notificationInfo._path, notificationInfo._token, notificationInfo._type);
    }

    pendingAttributeChangedNotifications.clear();
}
#endif

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
