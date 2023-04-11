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

class MayaUsdProxyShapeListenerBase : public MPxNode
{
public:
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
