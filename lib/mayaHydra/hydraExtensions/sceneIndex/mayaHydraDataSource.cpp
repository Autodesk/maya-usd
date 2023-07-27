//
// Copyright 2023 Autodesk, Inc. All rights reserved.
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

#include "mayaHydraDataSource.h"

#include <mayaHydraLib/sceneIndex/mayaHydraDisplayStyleDataSource.h>
#include <mayaHydraLib/sceneIndex/mayaHydraPrimvarDataSource.h>
#include <mayaHydraLib/sceneIndex/mayaHydraCameraDataSource.h>
#include <mayaHydraLib/sceneIndex/mayaHydraLightDataSource.h>
#include <mayaHydraLib/sceneIndex/mayaHydraSceneIndex.h>
#include <mayaHydraLib/adapters/adapter.h>

#include <pxr/imaging/hd/retainedDataSource.h>
#include <pxr/imaging/hd/basisCurvesSchema.h>
#include <pxr/imaging/hd/basisCurvesTopologySchema.h>
#include <pxr/imaging/hd/cameraSchema.h>
#include <pxr/imaging/hd/legacyDisplayStyleSchema.h>
#include <pxr/imaging/hd/lightSchema.h>
#include <pxr/imaging/hd/materialBindingSchema.h>
#include <pxr/imaging/hd/materialConnectionSchema.h>
#include <pxr/imaging/hd/materialNetworkSchema.h>
#include <pxr/imaging/hd/materialNodeSchema.h>
#include <pxr/imaging/hd/materialSchema.h>
#include <pxr/imaging/hd/meshSchema.h>
#include <pxr/imaging/hd/meshTopologySchema.h>
#include <pxr/imaging/hd/primvarSchema.h>
#include <pxr/imaging/hd/primvarsSchema.h>
#include <pxr/imaging/hd/visibilitySchema.h>
#include <pxr/imaging/hd/volumeFieldSchema.h>
#include <pxr/imaging/hd/xformSchema.h>

PXR_NAMESPACE_OPEN_SCOPE


MayaHydraDataSource::MayaHydraDataSource(
    const SdfPath& id,
    TfToken type,
    MayaHydraSceneIndex* sceneIndex,
    MayaHydraAdapter* adapter)
    : _id(id)
    , _type(type)
    , _sceneIndex(sceneIndex)
    , _adapter(adapter)
{
}

TfTokenVector
MayaHydraDataSource::GetNames()
{
    TfTokenVector result;

    if (_type == HdPrimTypeTokens->mesh) {
        result.push_back(HdMeshSchemaTokens->mesh);
    }

    if (_type == HdPrimTypeTokens->basisCurves) {
        result.push_back(HdBasisCurvesSchemaTokens->basisCurves);
    }

    result.push_back(HdPrimvarsSchemaTokens->primvars);

    if (HdPrimTypeIsGprim(_type)) {
        result.push_back(HdMaterialBindingSchemaTokens->materialBinding);
        result.push_back(HdLegacyDisplayStyleSchemaTokens->displayStyle);
        result.push_back(HdVisibilitySchemaTokens->visibility);
        result.push_back(HdXformSchemaTokens->xform);
    }

    if (HdPrimTypeIsLight(_type)) {
        result.push_back(HdMaterialSchemaTokens->material);
        result.push_back(HdXformSchemaTokens->xform);
        result.push_back(HdLightSchemaTokens->light);
    }

    if (_type == HdPrimTypeTokens->material) {
        result.push_back(HdMaterialSchemaTokens->material);
    }

    if (_type == HdPrimTypeTokens->camera) {
        result.push_back(HdCameraSchemaTokens->camera);
        result.push_back(HdXformSchemaTokens->xform);
    }

    return result;
}

HdDataSourceBaseHandle
MayaHydraDataSource::Get(const TfToken& name)
{
    if (name == HdMeshSchemaTokens->mesh) {
        if (_type == HdPrimTypeTokens->mesh) {
            auto topology = _adapter->GetMeshTopology();
            return HdMeshSchema::Builder()
                .SetTopology(
                    HdMeshTopologySchema::Builder()
                    .SetFaceVertexCounts(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            topology.GetFaceVertexCounts()))
                    .SetFaceVertexIndices(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            topology.GetFaceVertexIndices()))
                    .SetOrientation(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            HdMeshTopologySchemaTokens->rightHanded))
                    .Build())
                .SetSubdivisionScheme(
                    HdRetainedTypedSampledDataSource<TfToken>::New(topology.GetScheme()))
                .SetDoubleSided(
                    HdRetainedTypedSampledDataSource<bool>::New(_adapter->GetDoubleSided()))
                .Build();
        }
    }
    else if (name == HdBasisCurvesSchemaTokens->basisCurves) {
        if (_type == HdPrimTypeTokens->basisCurves) {
            auto topology = _adapter->GetBasisCurvesTopology();
            return HdBasisCurvesSchema::Builder()
                .SetTopology(
                    HdBasisCurvesTopologySchema::Builder()
                    .SetCurveVertexCounts(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            topology.GetCurveVertexCounts()))
                    .SetCurveIndices(
                        HdRetainedTypedSampledDataSource<VtIntArray>::New(
                            topology.GetCurveIndices()))
                    .SetBasis(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            topology.GetCurveBasis()))
                    .SetType(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            topology.GetCurveType()))
                    .SetWrap(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            topology.GetCurveWrap()))
                    .Build())
                .Build();
        }
    }
    else if (name == HdPrimvarsSchemaTokens->primvars) {
        return _GetPrimvarsDataSource();
    }
    // TODO: Material
    //else if (name == HdMaterialBindingSchemaTokens->materialBinding) {
    //    return _GetMaterialBindingDataSource();
    //}
    else if (name == HdXformSchemaTokens->xform) {
        auto xform = _adapter->GetTransform();
        return HdXformSchema::Builder()
            .SetMatrix(
                HdRetainedTypedSampledDataSource<GfMatrix4d>::New(xform))
            .Build();
    }
    // TODO: Material
    //else if (name == HdMaterialSchemaTokens->material) {
    //    return _GetMaterialDataSource();
    //}
    else if (name == HdLegacyDisplayStyleSchemaTokens->displayStyle) {
        return MayaHydraDisplayStyleDataSource::New(_id, _type, _sceneIndex, _adapter);
    }
    else if (name == HdVisibilitySchemaTokens->visibility) {
        return _GetVisibilityDataSource();
    }
    else if (name == HdCameraSchemaTokens->camera) {
        return MayaHydraCameraDataSource::New(_id, _type, _adapter);
    }
    else if (name == HdLightSchemaTokens->light) {
        return MayaHydraLightDataSource::New(_id, _type, _adapter);
    }

    return nullptr;
}

HdDataSourceBaseHandle MayaHydraDataSource::_GetVisibilityDataSource()
{
    bool vis = _adapter->GetVisible();
    if (vis) {
        static const HdContainerDataSourceHandle visOn =
            HdVisibilitySchema::BuildRetained(
                HdRetainedTypedSampledDataSource<bool>::New(true));
        return visOn;
    }
    else {
        static const HdContainerDataSourceHandle visOff =
            HdVisibilitySchema::BuildRetained(
                HdRetainedTypedSampledDataSource<bool>::New(false));
        return visOff;
    }
}

HdDataSourceBaseHandle MayaHydraDataSource::_GetPrimvarsDataSource()
{
    if (_primvarsBuilt.load()) {
        return HdContainerDataSource::AtomicLoad(_primvars);
    }

    MayaHydraPrimvarsDataSourceHandle primvarsDs;

    for (size_t interpolation = HdInterpolationConstant;
        interpolation < HdInterpolationCount; ++interpolation) {

        HdPrimvarDescriptorVector v = _adapter->GetPrimvarDescriptors((HdInterpolation)interpolation);

        TfToken interpolationToken = _InterpolationAsToken(
            (HdInterpolation)interpolation);

        for (const auto& primvarDesc : v) {
            if (!primvarsDs) {
                primvarsDs = MayaHydraPrimvarsDataSource::New(_adapter);
            }
            primvarsDs->AddDesc(
                primvarDesc.name, interpolationToken, primvarDesc.role,
                primvarDesc.indexed);
        }
    }

    HdContainerDataSourceHandle ds = primvarsDs;
    HdContainerDataSource::AtomicStore(_primvars, ds);
    _primvarsBuilt.store(true);

    return primvarsDs;
}

TfToken MayaHydraDataSource::_InterpolationAsToken(HdInterpolation interpolation)
{
    switch (interpolation) {
    case HdInterpolationConstant:
        return HdPrimvarSchemaTokens->constant;
    case HdInterpolationUniform:
        return HdPrimvarSchemaTokens->uniform;
    case HdInterpolationVarying:
        return HdPrimvarSchemaTokens->varying;
    case HdInterpolationVertex:
        return HdPrimvarSchemaTokens->vertex;
    case HdInterpolationFaceVarying:
        return HdPrimvarSchemaTokens->faceVarying;
    case HdInterpolationInstance:
        return HdPrimvarSchemaTokens->instance;

    default:
        return HdPrimvarSchemaTokens->constant;
    }
}

HdDataSourceBaseHandle
MayaHydraDataSource::_GetMaterialBindingDataSource()
{
    SdfPath path = _sceneIndex->GetMaterialId(_id);
    if (path.IsEmpty()) {
        return nullptr;
    }
    HdDataSourceBaseHandle bindingPath =
        HdRetainedTypedSampledDataSource<SdfPath>::New(path);

    TfToken t = HdMaterialBindingSchemaTokens->allPurpose;
    HdContainerDataSourceHandle binding =
        HdMaterialBindingSchema::BuildRetained(1, &t, &bindingPath);
    return binding;
}

PXR_NAMESPACE_CLOSE_SCOPE
