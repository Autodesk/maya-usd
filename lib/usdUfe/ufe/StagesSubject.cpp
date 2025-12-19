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

#include <usdUfe/base/tokens.h>
#include <usdUfe/ufe/Global.h>
#include <usdUfe/ufe/UfeNotifGuard.h>
#include <usdUfe/ufe/UfeVersionCompat.h>
#include <usdUfe/ufe/UsdCamera.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/undo/UsdUndoManager.h>

#include <pxr/base/tf/envSetting.h>
#include <pxr/base/tf/getenv.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/xformOp.h>

#include <ufe/attributes.h>
#include <ufe/globalSelection.h>
#include <ufe/hierarchy.h>
#include <ufe/object3d.h>
#include <ufe/object3dNotification.h>
#include <ufe/observableSelection.h>
#include <ufe/path.h>
#include <ufe/scene.h>
#include <ufe/sceneNotification.h>
#include <ufe/transform3d.h>

#include <regex>
#include <vector>

namespace {

TF_DEFINE_ENV_SETTING(
    MAYAUSD_IGNORE_ROOT_PROTOTYPES_ON_STAGE_CHANGED,
    true,
    "Ignores handling prototype prims at root on stage changed callback.");

bool isTransformChange(const TfToken& nameToken)
{
    return nameToken == UsdGeomTokens->xformOpOrder || UsdGeomXformOp::IsXformOp(nameToken);
}

// Prevent exception from the notifications from escaping and breaking USD/DCC.
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

// The attribute change notification guard is not meant to be nested, but
// use a counter nonetheless to provide consistent behavior in such cases.
std::atomic_int attributeChangedNotificationGuardCount { 0 };

enum class AttributeChangeType
{
    kAdded,
    kValueChanged,
    kConnectionChanged,
    kRemoved,
    kMetadataChanged
};

struct AttributeNotification
{
    AttributeNotification(const Ufe::Path& path, const TfToken& token, AttributeChangeType type)
        : _path(path)
        , _token(token)
        , _type(type)
    {
    }

    virtual ~AttributeNotification() { }

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

struct AttributeMetadataNotification : public AttributeNotification
{
    AttributeMetadataNotification(
        const Ufe::Path&             path,
        const TfToken&               token,
        AttributeChangeType          type,
        const std::set<std::string>& metadataKeys)
        : AttributeNotification(path, token, type)
        , _metadataKeys(metadataKeys)
    {
    }

    ~AttributeMetadataNotification() override { }

    std::set<std::string> _metadataKeys;

    bool operator==(const AttributeMetadataNotification& other)
    {
        // Only collapse multiple value changes. Collapsing added/removed notifications needs to be
        // done safely so the observer ends up in the right state.
        return other._type == _type && other._token == _token && other._path == _path
            && _type == AttributeChangeType::kMetadataChanged;
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

void sendAttributeChanged(
    const Ufe::Path&    ufePath,
    const TfToken&      changedToken,
    AttributeChangeType UFE_V4(changeType))
{
#ifdef UFE_V4_FEATURES_AVAILABLE
    switch (changeType) {
    case AttributeChangeType::kValueChanged: {
        notifyWithoutExceptions<Ufe::Attributes>(
            Ufe::AttributeValueChanged(ufePath, changedToken.GetString()));

        if (UsdUfe::UsdCamera::isCameraToken(changedToken)) {
            notifyWithoutExceptions<Ufe::Camera>(ufePath);
        }
    } break;
    case AttributeChangeType::kAdded: {
        if (UsdUfe::InSetAttribute::inSetAttribute()) {
            notifyWithoutExceptions<Ufe::Attributes>(
                Ufe::AttributeValueChanged(ufePath, changedToken.GetString()));

            if (UsdUfe::UsdCamera::isCameraToken(changedToken)) {
                notifyWithoutExceptions<Ufe::Camera>(ufePath);
            }
        } else {
            // Special case when Redo-ing visibility change, the notice.GetChangedInfoOnlyPaths()
            // does not contain the change, hence handling visibility notification in re-sync path.
            if (changedToken == UsdGeomTokens->visibility) {
                Ufe::VisibilityChanged vis(ufePath);
                notifyWithoutExceptions<Ufe::Object3d>(vis);
            }
            notifyWithoutExceptions<Ufe::Attributes>(
                Ufe::AttributeAdded(ufePath, changedToken.GetString()));
        }
    } break;
    case AttributeChangeType::kRemoved: {
        if (UsdUfe::InSetAttribute::inSetAttribute()) {
            notifyWithoutExceptions<Ufe::Attributes>(
                Ufe::AttributeValueChanged(ufePath, changedToken.GetString()));

            if (UsdUfe::UsdCamera::isCameraToken(changedToken)) {
                notifyWithoutExceptions<Ufe::Camera>(ufePath);
            }
        } else {
            // Special case when Undoing a visibility change, the notice.GetChangedInfoOnlyPaths()
            // does not contain the change, hence handling visibility notification in re-sync path.
            if (changedToken == UsdGeomTokens->visibility) {
                Ufe::VisibilityChanged vis(ufePath);
                notifyWithoutExceptions<Ufe::Object3d>(vis);
            }
            notifyWithoutExceptions<Ufe::Attributes>(
                Ufe::AttributeRemoved(ufePath, changedToken.GetString()));
        }
    } break;
    case AttributeChangeType::kConnectionChanged: {
        notifyWithoutExceptions<Ufe::Attributes>(
            Ufe::AttributeConnectionChanged(ufePath, changedToken.GetString()));
    } break;
    case AttributeChangeType::kMetadataChanged: {
        // Do nothing (needed to fix build error)
    } break;
    }
#else
    notifyWithoutExceptions<Ufe::Attributes>(
        Ufe::AttributeValueChanged(ufePath, changedToken.GetString()));

    if (UsdUfe::UsdCamera::isCameraToken(changedToken)) {
        notifyWithoutExceptions<Ufe::Camera>(ufePath);
    }
#endif
}

#ifdef UFE_V4_FEATURES_AVAILABLE
void sendAttributeMetadataChanged(
    const Ufe::Path&             ufePath,
    const TfToken&               changedToken,
    AttributeChangeType          UFE_V4(changeType),
    const std::set<std::string>& metadataKeys)
{
    if (changeType == AttributeChangeType::kMetadataChanged) {
        notifyWithoutExceptions<Ufe::Attributes>(
            Ufe::AttributeMetadataChanged(ufePath, changedToken.GetString(), metadataKeys));
    }
}
#endif

void valueChanged(const Ufe::Path& ufePath, const TfToken& changedToken)
{
    if (inAttributeChangedNotificationGuard()) {
        // Don't add pending notif if one already exists with same path/token.
        auto p
            = AttributeNotification { ufePath, changedToken, AttributeChangeType::kValueChanged };
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

#ifdef UFE_V4_FEATURES_AVAILABLE
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

#ifdef UFE_V4_FEATURES_AVAILABLE
void attributeMetadataChanged(
    const Ufe::Path&             ufePath,
    const TfToken&               changedToken,
    AttributeChangeType          changeType,
    const std::set<std::string>& metadataKeys)
{
    if (inAttributeChangedNotificationGuard()) {
        // Don't add pending notif if one already exists with same path/token.
        auto p = AttributeMetadataNotification(ufePath, changedToken, changeType, metadataKeys);
        auto p2 = std::find(
            pendingAttributeChangedNotifications.begin(),
            pendingAttributeChangedNotifications.end(),
            p);
        if (p2 == pendingAttributeChangedNotifications.end()) {
            pendingAttributeChangedNotifications.emplace_back(p);
        } else {
            auto p2AttributeMetadataNotification
                = dynamic_cast<AttributeMetadataNotification*>(&(*p2));
            if (p2AttributeMetadataNotification) {
                p2AttributeMetadataNotification->_metadataKeys.insert(
                    metadataKeys.begin(), metadataKeys.end());
            }
        }
    } else {
        sendAttributeMetadataChanged(ufePath, changedToken, changeType, metadataKeys);
    }
}

std::vector<std::string> getMetadataKeys(const std::string& strMetadata)
{
    std::vector<std::string> metadataKeys;
    const std::regex         rgx("(')([^ ]*)(':)");
    std::string              metadataKey;

    for (std::sregex_iterator it(strMetadata.begin(), strMetadata.end(), rgx), it_end; it != it_end;
         ++it) {
        if (it->empty()) {
            continue;
        }
        metadataKey = std::string((*it)[0]);
        metadataKey = metadataKey.substr(metadataKey.find('\'') + 1, metadataKey.rfind('\'') - 1);
        metadataKeys.push_back(metadataKey);
    }

    return metadataKeys;
}
#endif

void processAttributeChanges(
    const Ufe::Path&                                ufePath,
    const SdfPath&                                  changedPath,
    const std::vector<const SdfChangeList::Entry*>& UFE_V4(entries))
{
#ifdef UFE_V4_FEATURES_AVAILABLE
    bool                  sendValueChanged = true; // Default notification to send.
    bool                  sendAdded = false;
    bool                  sendRemoved = false;
    bool                  sendConnectionChanged = false;
    bool                  sendMetadataChanged = false;
    std::set<std::string> metadataKeys;
    for (const auto& entry : entries) {
        // We can have multiple flags merged into a single entry:
        if (entry->flags.didAddProperty || entry->flags.didAddPropertyWithOnlyRequiredFields) {
            sendAdded = true;
            sendValueChanged = false;
        }
        if (entry->flags.didRemoveProperty
            || entry->flags.didRemovePropertyWithOnlyRequiredFields) {
            sendRemoved = true;
            sendValueChanged = false;
        }
        if (entry->flags.didChangeAttributeConnection) {
            sendConnectionChanged = true;
            sendValueChanged = false;
        }
        for (const auto& infoChanged : entry->infoChanged) {
            if (infoChanged.first == UsdShadeTokens->sdrMetadata) {
                sendMetadataChanged = true;
                std::stringstream newMetadataStream;
                newMetadataStream << infoChanged.second.second;
                const std::string strMetadata = newMetadataStream.str();
                // e.g. strMetadata string format:
                // "'uifolder':,'uisoftmin':0.0, 'uihide':1, 'uiorder':0"
                if (!strMetadata.empty()) {
                    // Find the modified key which is between a pair of single quotes.
                    for (const auto& newMetadataKey : getMetadataKeys(strMetadata)) {
                        metadataKeys.insert(newMetadataKey);
                    }
                }
            } else if (infoChanged.first == SdfFieldKeys->AllowedTokens) {
                sendMetadataChanged = true;
                metadataKeys.insert(UsdUfe::MetadataTokens->UIEnumLabels);
            } else if (infoChanged.first == SdfFieldKeys->Documentation) {
                sendMetadataChanged = true;
                metadataKeys.insert(UsdUfe::MetadataTokens->UIDoc);
            } else if (infoChanged.first == SdfFieldKeys->DisplayGroup) {
                sendMetadataChanged = true;
                metadataKeys.insert(UsdUfe::MetadataTokens->UIFolder);
            } else if (infoChanged.first == SdfFieldKeys->DisplayName) {
                sendMetadataChanged = true;
                metadataKeys.insert(UsdUfe::MetadataTokens->UIName);
            }
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
    if (sendMetadataChanged) {
        attributeMetadataChanged(
            ufePath,
            changedPath.GetNameToken(),
            AttributeChangeType::kMetadataChanged,
            metadataKeys);
    }
#else
    valueChanged(ufePath, changedPath.GetNameToken());
#endif
};

} // namespace

namespace USDUFE_NS_DEF {

// Ensure that StagesSubject is properly setup.
USDUFE_VERIFY_CLASS_SETUP(PXR_NS::TfRefBase, StagesSubject);
USDUFE_VERIFY_CLASS_BASE(PXR_NS::TfWeakBase, StagesSubject);

USDUFE_VERIFY_CLASS_NOT_MOVE_OR_COPY(AttributeChangedNotificationGuard);

//------------------------------------------------------------------------------
// StagesSubject
//------------------------------------------------------------------------------

StagesSubject::StagesSubject() { }

StagesSubject::~StagesSubject() { }

/*static*/
StagesSubject::RefPtr StagesSubject::create() { return TfCreateRefPtr(new StagesSubject); }

PXR_NS::TfNotice::Key StagesSubject::registerStage(const PXR_NS::UsdStageRefPtr& stage)
{
    auto me = PXR_NS::TfCreateWeakPtr(this);
    return TfNotice::Register(me, &StagesSubject::stageChanged, stage);
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
            // We need to send some notifications so DCC can update (such as on undo
            // to move the transform manipulator back to original position).
            const TfToken& nameToken = changedPath.GetNameToken();
            auto           usdPrimPathStr = changedPath.GetPrimPath().GetString();
            auto           ufePath
                = stagePath(sender) + Ufe::PathSegment(usdPrimPathStr, getUsdRunTimeId(), '/');
            if (isTransformChange(nameToken)) {
                if (!UsdUfe::InTransform3dChange::inTransform3dChange()) {
                    notifyWithoutExceptions<Ufe::Transform3d>(ufePath);
                }
            }

            processAttributeChanges(ufePath, changedPath, it.base()->second);

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
        // each DCC path instancing the proxy shape / stage.
        Ufe::Path ufePath;
        UsdPrim   prim;
        if (changedPath == SdfPath::AbsoluteRootPath()) {
            ufePath = stagePath(sender);
            prim = stage->GetPseudoRoot();
        } else {
            const std::string& usdPrimPathStr = changedPath.GetPrimPath().GetString();
            ufePath = stagePath(sender)
                + Ufe::PathSegment(usdPrimPathStr, UsdUfe::getUsdRunTimeId(), '/');
            prim = stage->GetPrimAtPath(changedPath);
        }

        // Check the path string to see if we are dealing with a prototype,
        // this should work for both valid and invalid prims.
        if (TfGetEnvSetting(MAYAUSD_IGNORE_ROOT_PROTOTYPES_ON_STAGE_CHANGED)
            && changedPath.GetString().compare(0, 13, "/__Prototype_") == 0) {
            continue;
        }

        if (prim.IsValid() && !InPathChange::inPathChange()) {
            auto sceneItem = Ufe::Hierarchy::createItem(ufePath);

            // AL LayerCommands.addSubLayer test will cause crash
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
                    }
                    // Note : Do not send ObjectDelete notifs when didRemoveInertPrim or
                    // didRemoveNonInertPrim are set. Indeed, we can get these if prim
                    // specs are removed from some layers, but it does not mean that the prim is no
                    // longer in the composed stage. If the prim was actually gone, we would either
                    // get an invalid prim (in which case we would not even get here, and would send
                    // a object destroyed" notif in the else below), or we would fall into the
                    // "HasInfoChange : Active" case below. However, let the fallback
                    // SubtreeInvalidate notif be sent, as it is sometimes required (for example
                    // when unmarking a prim as instanceable - we get entries with the inert prim
                    // removed, as its instanced version is removed, but it is still there as a
                    // regular prim and needs to be invalidated.

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

                // If we are not in an add or delete operation, and a prim is
                // removed, we need to cleanup the selection list in order to
                // prevent stale items from being kept in the global selection set.
                if (!InAddOrDeleteOperation::inAddOrDeleteOperation()) {
                    auto       parentPath = changedPath.GetParentPath();
                    const auto parentUfePath = parentPath == SdfPath::AbsoluteRootPath()
                        ? stagePath(sender)
                        : stagePath(sender)
                            + Ufe::PathSegment(
                                parentPath.GetString(), UsdUfe::getUsdRunTimeId(), '/');

                    // Filter the global selection, removing items below our parent prim.
                    auto globalSn = Ufe::GlobalSelection::get();
                    if (!globalSn->empty()) {
                        bool itemRemoved = false;
                        auto newSel
                            = UsdUfe::removeDescendants(*globalSn, parentUfePath, &itemRemoved);
                        if (itemRemoved) {
                            globalSn->replaceWith(newSel);
                        }
                    }
                }
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
        auto        ufePath
            = stagePath(sender) + Ufe::PathSegment(usdPrimPathStr, UsdUfe::getUsdRunTimeId(), '/');

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

        if (!UsdUfe::InTransform3dChange::inTransform3dChange()) {
            // Is the change a Transform3d change?
            const UsdPrim prim = stage->GetPrimAtPath(changedPath.GetPrimPath());
            const TfToken nameToken = changedPath.GetNameToken();
            if (isTransformChange(nameToken)) {
                notifyWithoutExceptions<Ufe::Transform3d>(ufePath);
                sendValueChangedFallback = false;
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
                    const size_t                numInstances
                        = bool(pointInstancer) ? pointInstancer.GetInstanceCount() : 0u;

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
                    sendValueChangedFallback = false;
                }
            }
        }

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
    }

    // Special case when we are notified, but no paths given.
    if (resyncPaths.empty() && changedInfoOnlyPaths.empty()) {
        auto                       ufePath = stagePath(sender);
        Ufe::AttributeValueChanged vc(ufePath, "/");
        notifyWithoutExceptions<Ufe::Attributes>(vc);
    }
}

void StagesSubject::stageEditTargetChanged(
    UsdNotice::StageEditTargetChanged const& notice,
    UsdStageWeakPtr const&                   sender)
{
    // Track the edit target layer's state
    UsdUndoManager::instance().trackLayerStates(notice.GetStage()->GetEditTarget().GetLayer());
}

void StagesSubject::sendObjectAdd(const Ufe::SceneItem::Ptr& sceneItem) const
{
    try {
        Ufe::Scene::instance().notify(Ufe::ObjectAdd(sceneItem));
    } catch (const std::exception& ex) {
        TF_WARN("Caught error during notification: %s", ex.what());
    }
}

void StagesSubject::sendObjectPostDelete(const Ufe::SceneItem::Ptr& sceneItem) const
{
    try {
        Ufe::Scene::instance().notify(Ufe::ObjectPostDelete(sceneItem));
    } catch (const std::exception& ex) {
        TF_WARN("Caught error during notification: %s", ex.what());
    }
}

void StagesSubject::sendObjectDestroyed(const Ufe::Path& ufePath) const
{
    try {
        Ufe::Scene::instance().notify(Ufe::ObjectDestroyed(ufePath));
    } catch (const std::exception& ex) {
        TF_WARN("Caught error during notification: %s", ex.what());
    }
}

void StagesSubject::sendSubtreeInvalidate(const Ufe::SceneItem::Ptr& sceneItem) const
{
    try {
        Ufe::Scene::instance().notify(Ufe::SubtreeInvalidate(sceneItem));
    } catch (const std::exception& ex) {
        TF_WARN("Caught error during notification: %s", ex.what());
    }
}

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
        if (notificationInfo._type == AttributeChangeType::kMetadataChanged) {
#ifdef UFE_V4_FEATURES_AVAILABLE
            if (const auto metadataNotificationInfo
                = dynamic_cast<const AttributeMetadataNotification*>(&notificationInfo)) {
                sendAttributeMetadataChanged(
                    metadataNotificationInfo->_path,
                    metadataNotificationInfo->_token,
                    metadataNotificationInfo->_type,
                    metadataNotificationInfo->_metadataKeys);
            }
#endif
        } else {
            sendAttributeChanged(
                notificationInfo._path, notificationInfo._token, notificationInfo._type);
        }
    }

    pendingAttributeChangedNotifications.clear();
}

} // namespace USDUFE_NS_DEF
