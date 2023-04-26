//
// Copyright 2023 Autodesk
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
#ifndef PXRUSDMAYA_PROXY_SHAPE_LISTENER_BASE_H
#define PXRUSDMAYA_PROXY_SHAPE_LISTENER_BASE_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/listeners/stageNoticeListener.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/notice.h>

#include <maya/MPlug.h>
#include <maya/MPxNode.h>
#include <maya/MStatus.h>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
#define MAYAUSD_PROXY_SHAPE_LISTENER_BASE_TOKENS \
    ((MayaTypeName, "mayaUsdProxyShapeListenerBase"))
// clang-format on

TF_DECLARE_PUBLIC_TOKENS(
    MayaUsdProxyShapeListenerBaseTokens,
    MAYAUSD_CORE_PUBLIC,
    MAYAUSD_PROXY_SHAPE_LISTENER_BASE_TOKENS);

/// \class MayaUsdProxyShapeListenerBase
/// \brief This class allows listening to a Maya USD proxy for USD stage notifications.
///
/// Goals:
///
///  1 - Allow a client to know when a stage is changed
///  2 - Filter out changes that do not affect the rendering of the stage
///  3 - C++ clients of this code do not need to link with USD
///  4 - The class should not affect the stage in any negative way
///
/// Implementation:
///
/// We add a new Maya node that can connect to the "outStageCacheId" of the proxy shape and provide
/// update counters that will increment if the view needs to be refreshed or when the data needs to
/// be resynced. Using an external node prevents affecting the USD stage and requires only Maya APIs
/// for C++ clients.
///
/// The "updateId" counter will increment every time a scene refresh is required due to a value
/// changing (equivalent to a Hydra "change"). The "resyncId" counter will increment every time a
/// scene reparse is needed due to major topological changes (equivalent to a Hydra "resync").
///
/// Usage:
///
/// Clients wanting to listen on a proxyShape for USD changes need to instantiate a
/// "mayaUsdProxyShapeListener" node and connect its "stageCacheId" input to the "outStageCacheId"
/// output of the proxy shape. The new node will start listening on the proxy shape as soon as its
/// "outStageCacheId" gets pulled. So, once the proxy to listener connection is done, you can
/// either:
///
///  - connect your Maya node to the listener "outStageCacheId" and one of the two counters to get
///  dirtied/evaluated via regular Maya process
///
///  - Use MNodeMessage.addNodeDirtyPlugCallback() or MNodeMessage.addAttributeChangedCallback() to
///  have your C++ code receive notifications when the stage has changed and needs to be redrawn.
///  When processing the scene for updates, you need to fetch the latest USD cache ID from the
///  listener as this will allow the listener to start listening anew whenever the proxy starts
///  handling a new stage. An implementation of this workflow can be found in
///  test/testUtils/mayaUtils.py, look for TestProxyShapeUpdateHandler and its uses in unit tests.
///

class MayaUsdProxyShapeListenerBase : public MPxNode
{
public:
    MAYAUSD_CORE_PUBLIC
    ~MayaUsdProxyShapeListenerBase();

    MAYAUSD_CORE_PUBLIC
    static const MTypeId typeId;
    MAYAUSD_CORE_PUBLIC
    static const MString typeName;

    // Change counter attributes
    MAYAUSD_CORE_PUBLIC
    static MObject updateCounterAttr;
    MAYAUSD_CORE_PUBLIC
    static MObject resyncCounterAttr;

    // Input attributes
    MAYAUSD_CORE_PUBLIC
    static MObject stageCacheIdAttr;

    // Output attributes
    MAYAUSD_CORE_PUBLIC
    static MObject outStageCacheIdAttr;

    MAYAUSD_CORE_PUBLIC
    static void* creator();

    MAYAUSD_CORE_PUBLIC
    static MStatus initialize();

    MAYAUSD_CORE_PUBLIC
    void postConstructor() override;
    MAYAUSD_CORE_PUBLIC
    MStatus compute(const MPlug& plug, MDataBlock& dataBlock) override;

    MAYAUSD_CORE_PUBLIC
    MStatus connectionMade(const MPlug&, const MPlug&, bool asSrc) override;
    MAYAUSD_CORE_PUBLIC
    MStatus connectionBroken(const MPlug&, const MPlug&, bool asSrc) override;
    MAYAUSD_CORE_PUBLIC
    MStatus setDependentsDirty(const MPlug& plug, MPlugArray& plugArray) override;

private:
    UsdMayaStageNoticeListener _stageNoticeListener;
    void _OnStageContentsChanged(const UsdNotice::StageContentsChanged& notice);
    void _OnStageObjectsChanged(const UsdNotice::ObjectsChanged& notice);

    int _lastKnownStageCacheId { -1 };

    void _IncrementCounter(MObject attribute);
    void _ReInit();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
