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

#if WANT_UFE_BUILD
#include <mayaUsd/ufe/Global.h>
#endif

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
#if WANT_UFE_BUILD
#include <mayaUsd/ufe/Utils.h>

#include <ufe/rtid.h>
#endif

#include <pxr/base/tf/refPtr.h>

PXR_NAMESPACE_OPEN_SCOPE

typedef Ufe::Path (*MayaHydraInterpretRprimPath)(const HdSceneIndexBaseRefPtr&, const SdfPath&);
typedef void (
    *MayaHydraHandleSelectionChanged)(const HdSceneIndexBaseRefPtr&, const Ufe::SelectionChanged&);

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
    static TfToken         dataSourceNodePathEntry("object");
    HdDataSourceBaseHandle dataSourceEntryPathHandle = inputArgs->Get(dataSourceNodePathEntry);

    // Retrieve the version integer. The Version integer combines major, minor, and patch number
    // like this major * 10000 + minor * 100 + patch
    using MayaHydraVersionDataSource = HdRetainedTypedSampledDataSource<int>;
    static TfToken         dataSourceVersionEntry("version");
    HdDataSourceBaseHandle dataSourceEntryVersionHandle = inputArgs->Get(dataSourceVersionEntry);
    int version = 100; // initial mayaHydra version where version data source had not been defined
    if (auto dataSourceEntryVersion
        = MayaHydraVersionDataSource::Cast(dataSourceEntryVersionHandle)) {
        version = dataSourceEntryVersion->GetTypedValue(0.0f);
    }

    if (auto dataSourceEntryNodePath = HdMObjectDataSource::Cast(dataSourceEntryPathHandle)) {
#if WANT_UFE_BUILD
        if (version >= 200) {
            // Retrieve interpret pick function from the scene index plugin, to be accessed by
            // mayaHydra interpretRprimPath
            using MayaHydraInterpretRprimPathDataSource
                = HdRetainedTypedSampledDataSource<MayaHydraInterpretRprimPath&>;
            static TfToken         dataSourceInterpretPickEntry("interpretRprimPath");
            HdDataSourceBaseHandle dataSourceEntryInterpretPickHandle
                = inputArgs->Get(dataSourceInterpretPickEntry);
            auto dataSourceEntryInterpretPick
                = MayaHydraInterpretRprimPathDataSource::Cast(dataSourceEntryInterpretPickHandle);
            MayaHydraInterpretRprimPath& interpretRprimPath
                = dataSourceEntryInterpretPick->GetTypedValue(0.0f);
            interpretRprimPath = (MayaHydraInterpretRprimPath)&MayaUsd::
                MayaUsdProxyShapeSceneIndex::InterpretRprimPath;
        } else {
            using HdRtidRefDataSource = HdRetainedTypedSampledDataSource<Ufe::Rtid&>;
            static TfToken         dataSourceRuntimeEntry("runtime");
            HdDataSourceBaseHandle dataSourceEntryRuntimeHandle
                = inputArgs->Get(dataSourceRuntimeEntry);
            auto dataSourceEntryRuntime = HdRtidRefDataSource::Cast(dataSourceEntryRuntimeHandle);
            if (TF_VERIFY(dataSourceEntryRuntime, "Error UFE runtime id data source not found")) {
                Ufe::Rtid& id = dataSourceEntryRuntime->GetTypedValue(0.0f);
                id = MayaUsd::ufe::getUsdRunTimeId();
                TF_VERIFY(id != 0, "Invalid UFE runtime id");
            }
        }
#endif
        MObject           dagNode = dataSourceEntryNodePath->GetTypedValue(0.0f);
        MStatus           status;
        MFnDependencyNode dependNodeFn(dagNode, &status);
        if (!TF_VERIFY(status, "Error getting MFnDependencyNode")) {
            return nullptr;
        }

        auto proxyShape = dynamic_cast<MayaUsdProxyShapeBase*>(dependNodeFn.userNode());
        if (TF_VERIFY(proxyShape, "Error getting MayaUsdProxyShapeBase")) {
            // Convert USD prims to rprims consumed by Hydra
            auto usdImagingStageSceneIndex = UsdImagingStageSceneIndex::New();
            // Flatten transforms, visibility, purpose, model, and material
            // bindings over hierarchies.
            auto flatteningSceneIndex = HdFlatteningSceneIndex::New(usdImagingStageSceneIndex);
            return MayaUsd::MayaUsdProxyShapeSceneIndex::New(
                proxyShape, flatteningSceneIndex, usdImagingStageSceneIndex);
        }
    }

    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE

namespace MAYAUSD_NS_DEF {

///////////////////////// MayaUsdProxyShapeSceneIndex

MayaUsdProxyShapeSceneIndex::MayaUsdProxyShapeSceneIndex(
    HdSceneIndexBaseRefPtr          inputSceneIndex,
    UsdImagingStageSceneIndexRefPtr usdImagingStageSceneIndex,
    MayaUsdProxyShapeBase*          proxyShape)
    : ParentClass(inputSceneIndex)
    , _usdImagingStageSceneIndex(usdImagingStageSceneIndex)
    , _proxyShape(proxyShape)
{
    TfWeakPtr<MayaUsdProxyShapeSceneIndex> ptr(this);
    TfNotice::Register(ptr, &MayaUsdProxyShapeSceneIndex::StageSet);
    TfNotice::Register(ptr, &MayaUsdProxyShapeSceneIndex::ObjectsChanged);
}

MayaUsdProxyShapeSceneIndex::~MayaUsdProxyShapeSceneIndex() { }

MayaUsdProxyShapeSceneIndexRefPtr MayaUsdProxyShapeSceneIndex::New(
    MayaUsdProxyShapeBase*                 proxyShape,
    const HdFlatteningSceneIndexRefPtr&    flatteningSceneIndex,
    const UsdImagingStageSceneIndexRefPtr& usdImagingStageSceneIndex)
{
    // Create the proxy shape scene index which populates the stage
    return TfCreateRefPtr(new MayaUsdProxyShapeSceneIndex(
        flatteningSceneIndex, usdImagingStageSceneIndex, proxyShape));
}

Ufe::Path MayaUsdProxyShapeSceneIndex::InterpretRprimPath(
    const HdSceneIndexBaseRefPtr& sceneIndex,
    const SdfPath&                path)
{
    if (MayaUsdProxyShapeSceneIndexRefPtr proxyShapeSceneIndex
        = TfStatic_cast<MayaUsdProxyShapeSceneIndexRefPtr>(sceneIndex)) {
        MStatus  status;
        MDagPath dagPath(
            MDagPath::getAPathTo(proxyShapeSceneIndex->_proxyShape->thisMObject(), &status));
        return Ufe::Path({ MayaUsd::ufe::dagPathToPathSegment(dagPath),
                           MayaUsd::ufe::usdPathToUfePathSegment(path) });
    }

    return Ufe::Path();
}

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
#if PXR_VERSION <= 2305
                // In most recent USD, Populate is called from within SetStage, so there is no need
                // to callsites to call it explicitly
                _usdImagingStageSceneIndex->Populate();
#endif
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

    return _GetInputSceneIndex()->GetPrim(primPath);
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
