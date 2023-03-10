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

#include "proxyShapeSceneIndexPlugin.h"

#if PXR_VERSION >= 2211

#include <pxr/imaging/hd/flatteningSceneIndex.h>
#include <pxr/imaging/hd/retainedDataSource.h>
#include <pxr/imaging/hd/sceneIndexPlugin.h>
#include <pxr/imaging/hd/sceneIndexPluginRegistry.h>
#include <pxr/usd/usd/primFlags.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <maya/MObject.h>
#include <maya/MObjectHandle.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>
#include <ufe/rtid.h>


PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    HdSceneIndexPluginRegistry::
        Define<MayaUsdProxyShapeMayaNodeSceneIndexPlugin, HdSceneIndexPlugin>();
}

///////////////////////// MayaUsdProxyShapeMayaNodeSceneIndexPlugin

MayaUsdProxyShapeMayaNodeSceneIndexPlugin::MayaUsdProxyShapeMayaNodeSceneIndexPlugin() { }

HdSceneIndexBaseRefPtr MayaUsdProxyShapeMayaNodeSceneIndexPlugin::_AppendSceneIndex(
    const HdSceneIndexBaseRefPtr&      inputScene,
    const HdContainerDataSourceHandle& inputArgs)
{
    using HdMObjectDataSource = HdRetainedTypedSampledDataSource<MObject>;
    using HdRtidRefDataSource = HdRetainedTypedSampledDataSource<Ufe::Rtid&>;

    static TfToken dataSourceNodePathEntry("object");
    static TfToken dataSourceRuntimeEntry("runtime");

    HdDataSourceBaseHandle dataSourceEntryPathHandle = inputArgs->Get(dataSourceNodePathEntry);
    HdDataSourceBaseHandle dataSourceEntryRuntimeHandle = inputArgs->Get(dataSourceRuntimeEntry);
    if (auto dataSourceEntryNodePath = HdMObjectDataSource::Cast(dataSourceEntryPathHandle)) {
        auto dataSourceEntryRuntime = HdRtidRefDataSource::Cast(dataSourceEntryRuntimeHandle);
        if (TF_VERIFY(dataSourceEntryRuntime, "Error UFE runtime id data source not found")) {
            Ufe::Rtid& id = dataSourceEntryRuntime->GetTypedValue(0.0f);
            id = MayaUsd::ufe::getUsdRunTimeId();
            TF_VERIFY(id != 0, "Invalid UFE runtime id");
        }
        MObject           dagNode = dataSourceEntryNodePath->GetTypedValue(0.0f);
        MStatus           status;
        MFnDependencyNode dependNodeFn(dagNode, &status);
        if (!TF_VERIFY(status, "Error getting MFnDependencyNode")) {
            return nullptr;
        }

        auto proxyShape = dynamic_cast<MayaUsdProxyShapeBase*>(dependNodeFn.userNode());
        if (TF_VERIFY(proxyShape, "Error getting MayaUsdProxyShapeBase")) {
            auto psSceneIndex = MayaUsd::MayaUsdProxyShapeSceneIndex::New(proxyShape);
            // Flatten transforms, visibility, purpose, model, and material
            // bindings over hierarchies.
            return HdFlatteningSceneIndex::New(psSceneIndex);
        }
    }

    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE

namespace MAYAUSD_NS_DEF {

///////////////////////// MayaUsdProxyShapeSceneIndex

MayaUsdProxyShapeSceneIndex::MayaUsdProxyShapeSceneIndex(
    UsdImagingStageSceneIndexRefPtr inputSceneIndex,
    MayaUsdProxyShapeBase*          proxyShape)
    : ParentClass(inputSceneIndex)
    , _usdImagingStageSceneIndex(inputSceneIndex)
    , _proxyShape(proxyShape)
{
    TfWeakPtr<MayaUsdProxyShapeSceneIndex> ptr(this);
    TfNotice::Register(ptr, &MayaUsdProxyShapeSceneIndex::StageSet);
    TfNotice::Register(ptr, &MayaUsdProxyShapeSceneIndex::ObjectsChanged);
}

MayaUsdProxyShapeSceneIndex::~MayaUsdProxyShapeSceneIndex() { }

void MayaUsdProxyShapeSceneIndex::StageSet(const MayaUsdProxyStageSetNotice& notice) { Populate(); }

void MayaUsdProxyShapeSceneIndex::ObjectsChanged(
    const MayaUsdProxyStageObjectsChangedNotice& notice)
{
    // MAYA-126804 TODO: This should be enough to trigger dirty notifications being send
    // when a matrix is changed. However this is not working due missing
    // UsdImagingMeshAdapter::Invalidate implementation. This is not part of USD v22.11 however has
    // been fixed in later versions.
    _usdImagingStageSceneIndex->ApplyPendingUpdates();
}

void MayaUsdProxyShapeSceneIndex::Populate()
{
    if (!_populated) {
        if (auto stage = _proxyShape->getUsdStage()) {
            // Check whether the pseudo-root has children
            if (!stage->GetPseudoRoot().GetChildren().empty())
            // MAYA-126641: Upon first call to MayaUsdProxyShapeBase::getUsdStage, the stage may be
            // empty. Do not mark the scene index as _populated, until stage is full. This is taken
            // care of inside MayaUsdProxyShapeSceneIndex::_StageSet callback.
            {
                _usdImagingStageSceneIndex->SetStage(stage);
                _usdImagingStageSceneIndex->Populate();
                _populated = true;
            }
        }
    }
}

// satisfying HdSceneIndexBase
HdSceneIndexPrim MayaUsdProxyShapeSceneIndex::GetPrim(const SdfPath& primPath) const
{
    // MAYA-126790 TODO: properly resolve missing PrimsAdded notification issue
    // https://github.com/PixarAnimationStudios/USD/blob/dev/pxr/imaging/hd/sceneIndex.cpp#L38
    // Pixar has discussed adding a missing overridable virtual function when an observer is
    // registered For now GetPrim called with magic string populates the scene index
    static SdfPath maya126790Workaround("maya126790Workaround");
    if (primPath == maya126790Workaround) {
        auto nonConstThis = const_cast<MayaUsdProxyShapeSceneIndex*>(this);
        nonConstThis->Populate();
        return HdSceneIndexPrim();
    }

    return _usdImagingStageSceneIndex->GetPrim(primPath);
}

SdfPathVector MayaUsdProxyShapeSceneIndex::GetChildPrimPaths(const SdfPath& primPath) const
{
    return _usdImagingStageSceneIndex->GetChildPrimPaths(primPath);
}

void MayaUsdProxyShapeSceneIndex::_PrimsAdded(
    const HdSceneIndexBase&                       sender,
    const HdSceneIndexObserver::AddedPrimEntries& entries)
{
    _SendPrimsAdded(entries);
}

void MayaUsdProxyShapeSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase&                         sender,
    const HdSceneIndexObserver::RemovedPrimEntries& entries)
{
    _SendPrimsRemoved(entries);
}

void MayaUsdProxyShapeSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase&                         sender,
    const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    _SendPrimsDirtied(entries);
}
} // namespace MAYAUSD_NS_DEF

#endif
