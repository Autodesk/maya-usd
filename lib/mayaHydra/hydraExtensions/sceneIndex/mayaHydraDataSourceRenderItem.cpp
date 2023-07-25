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

#include "mayaHydraDataSourceRenderItem.h"

#include <mayaHydraLib/adapters/renderItemAdapter.h>

#include <pxr/base/tf/denseHashMap.h>
#include <pxr/imaging/hd/retainedDataSource.h>
#include "pxr/imaging/hd/basisCurvesSchema.h"
#include "pxr/imaging/hd/basisCurvesTopologySchema.h"
#include "pxr/imaging/hd/cameraSchema.h"
#include "pxr/imaging/hd/categoriesSchema.h"
#include "pxr/imaging/hd/coordSysBindingSchema.h"
#include "pxr/imaging/hd/extComputationInputComputationSchema.h"
#include "pxr/imaging/hd/extComputationOutputSchema.h"
#include "pxr/imaging/hd/extComputationPrimvarSchema.h"
#include "pxr/imaging/hd/extComputationPrimvarsSchema.h"
#include "pxr/imaging/hd/extComputationSchema.h"
#include "pxr/imaging/hd/extentSchema.h"
#include "pxr/imaging/hd/geomSubsetSchema.h"
#include "pxr/imaging/hd/geomSubsetsSchema.h"
#include "pxr/imaging/hd/instanceCategoriesSchema.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/instanceSchema.h"
#include "pxr/imaging/hd/legacyDisplayStyleSchema.h"
#include "pxr/imaging/hd/lightSchema.h"
#include "pxr/imaging/hd/materialBindingSchema.h"
#include "pxr/imaging/hd/materialConnectionSchema.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/meshSchema.h"
#include "pxr/imaging/hd/meshTopologySchema.h"
#include "pxr/imaging/hd/primvarSchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/purposeSchema.h"
#include "pxr/imaging/hd/renderBufferSchema.h"
#include "pxr/imaging/hd/renderSettingsSchema.h"
#include "pxr/imaging/hd/sampleFilterSchema.h"
#include "pxr/imaging/hd/displayFilterSchema.h"
#include "pxr/imaging/hd/subdivisionTagsSchema.h"
#include "pxr/imaging/hd/visibilitySchema.h"
#include "pxr/imaging/hd/volumeFieldBindingSchema.h"
#include "pxr/imaging/hd/volumeFieldSchema.h"
#include "pxr/imaging/hd/xformSchema.h"


PXR_NAMESPACE_OPEN_SCOPE

// ----------------------------------------------------------------------------

class MayaHydraDataSourceRenderItemPrimvarValue : public HdSampledDataSource
{
public:
    HD_DECLARE_DATASOURCE(MayaHydraDataSourceRenderItemPrimvarValue);

    MayaHydraDataSourceRenderItemPrimvarValue(
        const TfToken& primvarName,
        MayaHydraRenderItemAdapter* riAdapter)
        : _primvarName(primvarName)
        , _riAdapter(riAdapter)
    {
    }

    VtValue GetValue(Time shutterOffset) override
    {
        return _riAdapter->Get(_primvarName);
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime, Time endTime,
        std::vector<Time>* outSampleTimes) override
    {
        return false;
    }

private:
    TfToken _primvarName;
    MayaHydraRenderItemAdapter* _riAdapter;
};


// ----------------------------------------------------------------------------

class MayaHydraDataSourceRenderItemPrimvars : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(MayaHydraDataSourceRenderItemPrimvars);

    MayaHydraDataSourceRenderItemPrimvars(
        MayaHydraRenderItemAdapter* riAdapter)
        : _riAdapter(riAdapter)
    {
    }

    void AddDesc(
        const TfToken& name,
        const TfToken& interpolation,
        const TfToken& role, bool indexed)
    {
        _entries[name] = { interpolation, role, indexed };
    }

    TfTokenVector GetNames() override
    {
        TfTokenVector result;
        result.reserve(_entries.size());
        for (const auto& pair : _entries) {
            result.push_back(pair.first);
        }
        return result;
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        _EntryMap::const_iterator it = _entries.find(name);
        if (it == _entries.end()) {
            return nullptr;
        }

        // Need to handle indexed case?
        assert(!(*it).second.indexed);
        return HdPrimvarSchema::Builder()
            .SetPrimvarValue(MayaHydraDataSourceRenderItemPrimvarValue::New(
                name, _riAdapter))
            .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
                (*it).second.interpolation))
            .SetRole(HdPrimvarSchema::BuildRoleDataSource(
                (*it).second.role))
            .Build();
    }

private:
    struct _Entry
    {
        TfToken interpolation;
        TfToken role;
        bool indexed;
    };

    using _EntryMap = TfDenseHashMap<TfToken, _Entry,
        TfToken::HashFunctor, std::equal_to<TfToken>, 32>;

    _EntryMap _entries;
    MayaHydraRenderItemAdapter* _riAdapter;
};

HD_DECLARE_DATASOURCE_HANDLES(MayaHydraDataSourceRenderItemPrimvars);

// ----------------------------------------------------------------------------

MayaHydraDataSourceRenderItem::MayaHydraDataSourceRenderItem(
    const SdfPath& id,
    TfToken type,
    MayaHydraRenderItemAdapter* riAdapter)
    : _id(id)
    , _type(type)
    , _riAdapter(riAdapter)
{
}

TfTokenVector
MayaHydraDataSourceRenderItem::GetNames()
{
    TfTokenVector result;

    if (_type == HdPrimTypeTokens->mesh) {
        result.push_back(HdMeshSchemaTokens->mesh);
    }
    else if (_type == HdPrimTypeTokens->basisCurves) {
        result.push_back(HdBasisCurvesSchemaTokens->basisCurves);
    }

    result.push_back(HdPrimvarsSchemaTokens->primvars);
    result.push_back(HdExtComputationPrimvarsSchemaTokens->extComputationPrimvars);
    result.push_back(HdMaterialBindingSchemaTokens->materialBinding);
    result.push_back(HdLegacyDisplayStyleSchemaTokens->displayStyle);
    result.push_back(HdCoordSysBindingSchemaTokens->coordSysBinding);
    result.push_back(HdPurposeSchemaTokens->purpose);
    result.push_back(HdVisibilitySchemaTokens->visibility);
    result.push_back(HdInstancedBySchemaTokens->instancedBy);
    result.push_back(HdCategoriesSchemaTokens->categories);
    result.push_back(HdXformSchemaTokens->xform);
    result.push_back(HdExtentSchemaTokens->extent);

    return result;
}

HdDataSourceBaseHandle
MayaHydraDataSourceRenderItem::Get(const TfToken& name)
{
    if (name == HdMeshSchemaTokens->mesh) {
        if (_type == HdPrimTypeTokens->mesh) {
            auto topology = _riAdapter->GetMeshTopology();
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
                    HdRetainedTypedSampledDataSource<bool>::New(_riAdapter->GetDoubleSided()))
                .Build();
        }
    }
    else if (name == HdBasisCurvesSchemaTokens->basisCurves) {
        if (_type == HdPrimTypeTokens->basisCurves) {
            auto topology = _riAdapter->GetBasisCurvesTopology();
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
        auto xform = _riAdapter->GetTransform();
        return HdXformSchema::Builder()
            .SetMatrix(
                HdRetainedTypedSampledDataSource<GfMatrix4d>::New(xform))
            .Build();
    }
    // TODO: Material
    //else if (name == HdLegacyDisplayStyleSchemaTokens->displayStyle) {
    //    return _GetDisplayStyleDataSource();
    //}
    else if (name == HdVisibilitySchemaTokens->visibility) {
        return _GetVisibilityDataSource();
    }

    return nullptr;
}

HdDataSourceBaseHandle MayaHydraDataSourceRenderItem::_GetVisibilityDataSource()
{
    bool vis = _riAdapter->GetVisible();
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

HdDataSourceBaseHandle MayaHydraDataSourceRenderItem::_GetPrimvarsDataSource()
{
    if (_primvarsBuilt.load()) {
        return HdContainerDataSource::AtomicLoad(_primvars);
    }

    MayaHydraDataSourceRenderItemPrimvarsHandle primvarsDs;

    for (size_t interpolation = HdInterpolationConstant;
        interpolation < HdInterpolationCount; ++interpolation) {

        HdPrimvarDescriptorVector v = _riAdapter->GetPrimvarDescriptors((HdInterpolation)interpolation);

        TfToken interpolationToken = _InterpolationAsToken(
            (HdInterpolation)interpolation);

        for (const auto& primvarDesc : v) {
            if (!primvarsDs) {
                primvarsDs = MayaHydraDataSourceRenderItemPrimvars::New(_riAdapter);
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

TfToken MayaHydraDataSourceRenderItem::_InterpolationAsToken(HdInterpolation interpolation)
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

PXR_NAMESPACE_CLOSE_SCOPE
