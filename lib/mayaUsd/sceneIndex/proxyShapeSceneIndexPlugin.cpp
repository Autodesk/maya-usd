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

#include <mayaUsd/nodes/proxyShapeStageExtraData.h>
#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/Utils.h>

#include <usdUfe/ufe/Utils.h>

#include <pxr/imaging/hd/retainedDataSource.h>

#if USD_IMAGING_API_VERSION >= 20 // For USD 23.11+
#include <pxr/usdImaging/usdImaging/sceneIndices.h>
#else

#include <pxr/imaging/hd/flatteningSceneIndex.h>
#if defined(HD_API_VERSION) && HD_API_VERSION >= 51
#include <pxr/imaging/hd/materialBindingsSchema.h>
#else
#include <pxr/imaging/hd/materialBindingSchema.h>
#endif // HD_API_VERSION >= 51

#include <pxr/imaging/hd/purposeSchema.h>
#include <pxr/imaging/hd/retainedDataSource.h>
#include <pxr/imaging/hd/sceneIndexPlugin.h>
#include <pxr/imaging/hd/sceneIndexPluginRegistry.h>
#include <pxr/usd/usd/primFlags.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#if PXR_VERSION >= 2302
#include <pxr/usdImaging/usdImaging/drawModeSceneIndex.h> //For USD 2302 and later
#include <pxr/usdImaging/usdImaging/niPrototypePropagatingSceneIndex.h>
#include <pxr/usdImaging/usdImaging/piPrototypePropagatingSceneIndex.h>
#else
#include <pxr/imaging/hd/instancedBySceneIndex.h>
#include <pxr/usdImaging/usdImagingGL/drawModeSceneIndex.h> //For USD 2211
#endif                                                      // PXR_VERSION >= 2302

#if defined(HD_API_VERSION) && HD_API_VERSION >= 54
#include <pxr/imaging/hd/flattenedMaterialBindingsDataSourceProvider.h>
#include <pxr/imaging/hdsi/legacyDisplayStyleOverrideSceneIndex.h>
#include <pxr/imaging/hdsi/primTypePruningSceneIndex.h>
#include <pxr/usdImaging/usdImaging/extentResolvingSceneIndex.h>
#include <pxr/usdImaging/usdImaging/flattenedDataSourceProviders.h>
#include <pxr/usdImaging/usdImaging/renderSettingsFlatteningSceneIndex.h>
#include <pxr/usdImaging/usdImaging/rootOverridesSceneIndex.h>
#include <pxr/usdImaging/usdImaging/selectionSceneIndex.h>
#include <pxr/usdImaging/usdImaging/unloadedDrawModeSceneIndex.h>
#endif // HD_API_VERSION >= 54

#endif // USD_IMAGING_API_VERSION >= 20

#include <maya/MEventMessage.h>
#include <maya/MObject.h>
#include <maya/MObjectHandle.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>
#include <ufe/rtid.h>

PXR_NAMESPACE_OPEN_SCOPE

typedef Ufe::Path (*MayaHydraInterpretRprimPath)(const HdSceneIndexBaseRefPtr&, const SdfPath&);

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
        MObject           dagNode = dataSourceEntryNodePath->GetTypedValue(0.0f);
        MStatus           status;
        MFnDependencyNode dependNodeFn(dagNode, &status);
        if (!TF_VERIFY(status, "Error getting MFnDependencyNode")) {
            return nullptr;
        }

        auto proxyShape = dynamic_cast<MayaUsdProxyShapeBase*>(dependNodeFn.userNode());
        if (TF_VERIFY(proxyShape, "Error getting MayaUsdProxyShapeBase")) {
#if USD_IMAGING_API_VERSION >= 20 // For USD 23.11+
            UsdImagingCreateSceneIndicesInfo createInfo;
            UsdImagingSceneIndices sceneIndices = UsdImagingCreateSceneIndices(createInfo);

            return MayaUsd::MayaUsdProxyShapeSceneIndex::New(
                proxyShape, sceneIndices.finalSceneIndex, sceneIndices.stageSceneIndex);
#else

// We already have PXR_VERSION >= 2211
#if PXR_VERSION < 2302 // So for 2211
            auto                   usdImagingStageSceneIndex = UsdImagingStageSceneIndex::New();
            HdSceneIndexBaseRefPtr sceneIndex = UsdImagingGLDrawModeSceneIndex::New(
                HdFlatteningSceneIndex::New(
                    HdInstancedBySceneIndex::New(usdImagingStageSceneIndex)),
                /* inputArgs = */ nullptr);
#else
            // For PXR_VERSION >=  2302 so for USD 22.11+
#if defined(HD_API_VERSION) && HD_API_VERSION >= 54 // For USD 23.08+
            using namespace HdMakeDataSourceContainingFlattenedDataSourceProvider;
#endif
            //  Used for the HdFlatteningSceneIndex to flatten material bindings
            static const HdContainerDataSourceHandle flatteningInputArgs
                = HdRetainedContainerDataSource::New(
#if defined(HD_API_VERSION) && HD_API_VERSION >= 51
                    HdMaterialBindingsSchema::GetSchemaToken(),
#else
                    HdMaterialBindingSchemaTokens->materialBinding,
#endif
#if defined(HD_API_VERSION) && HD_API_VERSION >= 54 // For USD 23.08+
                    Make<HdFlattenedMaterialBindingsDataSourceProvider>());
#else
                    HdRetainedTypedSampledDataSource<bool>::New(true));
#endif

#if PXR_VERSION < 2308
            // Convert USD prims to rprims consumed by Hydra
            auto usdImagingStageSceneIndex = UsdImagingStageSceneIndex::New();

            // Flatten transforms, visibility, purpose, model, and material
            // bindings over hierarchies.
            // Do it the same way as USDView does with a SceneIndices chain
            HdSceneIndexBaseRefPtr sceneIndex
                = UsdImagingPiPrototypePropagatingSceneIndex::New(usdImagingStageSceneIndex);

            sceneIndex = UsdImagingNiPrototypePropagatingSceneIndex::New(sceneIndex);

            // The native prototype propagating scene index does a lot of
            // the flattening before inserting copies of the the prototypes
            // into the scene index. However, the resolved material for a prim
            // coming from a USD prototype can depend on the prim ancestors of
            // a corresponding instance. So we need to do one final resolve here.
            sceneIndex = HdFlatteningSceneIndex::New(sceneIndex, flatteningInputArgs);
            sceneIndex = UsdImagingDrawModeSceneIndex::New(sceneIndex, /* inputArgs = */ nullptr);
#else
            // #For USD 23.08 and later, HD_API_VERSION=54 in USD 23.08
            static const bool _displayUnloadedPrimsWithBounds = true;

            HdContainerDataSourceHandle const stageInputArgs = HdRetainedContainerDataSource::New(
                UsdImagingStageSceneIndexTokens->includeUnloadedPrims,
                HdRetainedTypedSampledDataSource<bool>::New(_displayUnloadedPrimsWithBounds));

            // Create the scene index graph.
            auto usdImagingStageSceneIndex = UsdImagingStageSceneIndex::New(stageInputArgs);
            HdSceneIndexBaseRefPtr sceneIndex = usdImagingStageSceneIndex;

            static HdContainerDataSourceHandle const materialPruningInputArgs
                = HdRetainedContainerDataSource::New(
                    HdsiPrimTypePruningSceneIndexTokens->primTypes,
                    HdRetainedTypedSampledDataSource<TfTokenVector>::New(
                        { HdPrimTypeTokens->material }),
                    HdsiPrimTypePruningSceneIndexTokens->bindingToken,
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        HdMaterialBindingsSchema::GetSchemaToken()));

            // Prune scene materials prior to flattening inherited
            // materials bindings and resolving material bindings
            sceneIndex = HdsiPrimTypePruningSceneIndex::New(sceneIndex, materialPruningInputArgs);

            static HdContainerDataSourceHandle const lightPruningInputArgs
                = HdRetainedContainerDataSource::New(
                    HdsiPrimTypePruningSceneIndexTokens->primTypes,
                    HdRetainedTypedSampledDataSource<TfTokenVector>::New(HdLightPrimTypeTokens()),
                    HdsiPrimTypePruningSceneIndexTokens->doNotPruneNonPrimPaths,
                    HdRetainedTypedSampledDataSource<bool>::New(false));

            sceneIndex = HdsiPrimTypePruningSceneIndex::New(sceneIndex, lightPruningInputArgs);

            // Use extentsHint for default_/geometry purpose
            HdContainerDataSourceHandle const extentInputArgs = HdRetainedContainerDataSource::New(
                UsdGeomTokens->purpose,
                HdRetainedTypedSampledDataSource<TfToken>::New(UsdGeomTokens->default_));

            sceneIndex = UsdImagingExtentResolvingSceneIndex::New(sceneIndex, extentInputArgs);

            if (_displayUnloadedPrimsWithBounds) {
                sceneIndex = UsdImagingUnloadedDrawModeSceneIndex::New(sceneIndex);
            }

            sceneIndex = UsdImagingRootOverridesSceneIndex::New(sceneIndex);

            sceneIndex = UsdImagingPiPrototypePropagatingSceneIndex::New(sceneIndex);

            sceneIndex = UsdImagingNiPrototypePropagatingSceneIndex::New(sceneIndex);

            sceneIndex = UsdImagingSelectionSceneIndex::New(sceneIndex);

            sceneIndex = UsdImagingRenderSettingsFlatteningSceneIndex::New(sceneIndex);

            sceneIndex
                = HdFlatteningSceneIndex::New(sceneIndex, UsdImagingFlattenedDataSourceProviders());

            sceneIndex = UsdImagingDrawModeSceneIndex::New(
                sceneIndex,
                /* inputArgs = */ nullptr);

            sceneIndex = HdsiLegacyDisplayStyleOverrideSceneIndex::New(sceneIndex);

#endif // End of #if PXR_VERSION < 2308 block
#endif // end of #if PXR_VERSION < 2302 block

            return MayaUsd::MayaUsdProxyShapeSceneIndex::New(
                proxyShape, sceneIndex, usdImagingStageSceneIndex);
#endif // End of #else clause of if USD_IMAGING_API_VERSION >= 20 block
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

    _timeChangeCallbackId = MEventMessage::addEventCallback("timeChanged", onTimeChanged, this);
}

MayaUsdProxyShapeSceneIndex::~MayaUsdProxyShapeSceneIndex()
{
    MMessage::removeCallback(_timeChangeCallbackId);
}

MayaUsdProxyShapeSceneIndexRefPtr MayaUsdProxyShapeSceneIndex::New(
    MayaUsdProxyShapeBase*                 proxyShape,
    const HdSceneIndexBaseRefPtr&          sceneIndexChainLastElement,
    const UsdImagingStageSceneIndexRefPtr& usdImagingStageSceneIndex)
{
    // Create the proxy shape scene index which populates the stage
    return TfCreateRefPtr(new MayaUsdProxyShapeSceneIndex(
        sceneIndexChainLastElement, usdImagingStageSceneIndex, proxyShape));
}

void MayaUsdProxyShapeSceneIndex::onTimeChanged(void* data)
{
    auto* instance = reinterpret_cast<MayaUsdProxyShapeSceneIndex*>(data);
    if (!TF_VERIFY(instance)) {
        return;
    }
    instance->UpdateTime();
}

bool MayaUsdProxyShapeSceneIndex::isProxyShapeValid()
{
    return MayaUsdProxyShapeStageExtraData::containsProxyShape(*_proxyShape);
}

void MayaUsdProxyShapeSceneIndex::UpdateTime()
{
    if (_usdImagingStageSceneIndex && _proxyShape && isProxyShapeValid()) {
        _usdImagingStageSceneIndex->SetTime(_proxyShape->getTime());
    }
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
        return Ufe::Path(
            { MayaUsd::ufe::dagPathToPathSegment(dagPath), UsdUfe::usdPathToUfePathSegment(path) });
    }

    return Ufe::Path();
}

void MayaUsdProxyShapeSceneIndex::StageSet(const MayaUsdProxyStageSetNotice& notice) { Populate(); }

void MayaUsdProxyShapeSceneIndex::ObjectsChanged(
    const MayaUsdProxyStageObjectsChangedNotice& notice)
{
    _usdImagingStageSceneIndex->ApplyPendingUpdates();
}

void MayaUsdProxyShapeSceneIndex::Populate()
{
    if (!_populated) {
        auto stage = _proxyShape->getUsdStage();
        if (TF_VERIFY(stage, "Unable to fetch a valid USDStage.")) {
            _usdImagingStageSceneIndex->SetStage(stage);
            // Check whether the pseudo-root has children
            if (!stage->GetPseudoRoot().GetChildren().empty())
            // MAYA-126641: Upon first call to MayaUsdProxyShapeBase::getUsdStage, the stage may be
            // empty. Do not mark the scene index as _populated, until stage is full. This is taken
            // care of inside MayaUsdProxyShapeSceneIndex::_StageSet callback.
            {
#if PXR_VERSION < 2305
                // In most recent USD, Populate is called from within SetStage, so there is no need
                // to callsites to call it explicitly
                _usdImagingStageSceneIndex->Populate();
#endif
                _populated = true;
            }
            // Set the initial time
            UpdateTime();
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
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
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

#endif // PXR_VERSION >= 2211
