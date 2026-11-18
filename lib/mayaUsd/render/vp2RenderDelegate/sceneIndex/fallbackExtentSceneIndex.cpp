//
// Copyright 2026 Autodesk
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
#include "fallbackExtentSceneIndex.h"

#include <pxr/imaging/hd/dataSource.h>
#include <pxr/imaging/hd/extentSchema.h>
#include <pxr/imaging/hd/retainedDataSource.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/bboxCache.h>
#include <pxr/usd/usdGeom/boundable.h>
#include <pxr/usd/usdGeom/modelAPI.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usdImaging/usdImaging/extentsHintSchema.h>
#include <pxr/usdImaging/usdImaging/geomModelSchema.h>

#include <algorithm>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

/*! The prim types whose schemas declare a fallback extent. UsdImaging publishes the "_1" variants
    of the cylinder and the capsule under the same types, and their fallbacks are the same.
*/
bool _IsImplicitSurface(const TfToken& primType)
{
    return primType == HdPrimTypeTokens->cube || primType == HdPrimTypeTokens->sphere
        || primType == HdPrimTypeTokens->cone || primType == HdPrimTypeTokens->cylinder
        || primType == HdPrimTypeTokens->capsule || primType == HdPrimTypeTokens->plane;
}

HdContainerDataSourceHandle _ExtentDataSource(const GfRange3d& range)
{
    return HdExtentSchema::Builder()
        .SetMin(HdRetainedTypedSampledDataSource<GfVec3d>::New(range.GetMin()))
        .SetMax(HdRetainedTypedSampledDataSource<GfVec3d>::New(range.GetMax()))
        .Build();
}

/*! The extent of a model, the box of its children. It is computed when first asked for, the way
    UsdImagingDrawModeAdapter computed it: with a bounding box cache of its own, for the purposes
    it drew, at the earliest time. A model with nothing to bound has no extent, where
    UsdImagingDrawModeAdapter went on with an empty one.
*/
class _ModelExtentDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_ModelExtentDataSource);

    TfTokenVector GetNames() override
    {
        return _Range().IsEmpty()
            ? TfTokenVector()
            : TfTokenVector { HdExtentSchemaTokens->min, HdExtentSchemaTokens->max };
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        const GfRange3d& range = _Range();
        if (range.IsEmpty()) {
            return nullptr;
        }
        if (name == HdExtentSchemaTokens->min) {
            return HdRetainedTypedSampledDataSource<GfVec3d>::New(range.GetMin());
        }
        if (name == HdExtentSchemaTokens->max) {
            return HdRetainedTypedSampledDataSource<GfVec3d>::New(range.GetMax());
        }
        return nullptr;
    }

private:
    _ModelExtentDataSource(const UsdStageWeakPtr& stage, const SdfPath& primPath)
        : _stage(stage)
        , _primPath(primPath)
    {
    }

    //! Computed once, on whichever thread asks first: a render index may read in parallel.
    const GfRange3d& _Range()
    {
        std::call_once(_computed, [this] { _range = _Compute(); });
        return _range;
    }

    GfRange3d _Compute() const
    {
        // The stage may be gone by the time a data source that outlived it is read.
        if (!_stage) {
            return GfRange3d();
        }
        const UsdPrim prim = _stage->GetPrimAtPath(_primPath);
        if (!prim) {
            return GfRange3d();
        }

        UsdGeomBBoxCache cache(
            UsdTimeCode::EarliestTime(),
            { UsdGeomTokens->default_, UsdGeomTokens->proxy, UsdGeomTokens->render },
            /* useExtentsHint = */ true);
        return cache.ComputeUntransformedBound(prim).ComputeAlignedBox();
    }

    const UsdStageWeakPtr _stage;
    const SdfPath         _primPath;
    std::once_flag        _computed;
    GfRange3d             _range;
};

/*! The prim, with the extent UsdImaging gave it none of. What is looked up is left to the moment
    someone asks for the extent, so a prim nobody asks costs the allocation of this object, and a
    prim UsdImaging gave an extent keeps it.
*/
class _PrimDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimDataSource);

    TfTokenVector GetNames() override
    {
        TfTokenVector names = _input->GetNames();
        if (std::find(names.begin(), names.end(), HdExtentSchemaTokens->extent) == names.end()
            && _Fallback()) {
            names.push_back(HdExtentSchemaTokens->extent);
        }
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        HdDataSourceBaseHandle result = _input->Get(name);
        if (!result && name == HdExtentSchemaTokens->extent) {
            result = _Fallback();
        }
        return result;
    }

private:
    _PrimDataSource(
        const HdContainerDataSourceHandle& input,
        const UsdStageWeakPtr&             stage,
        const SdfPath&                     primPath,
        const TfToken&                     primType)
        : _input(input)
        , _stage(stage)
        , _primPath(primPath)
        , _primType(primType)
    {
    }

    HdDataSourceBaseHandle _Fallback()
    {
        // The stage may be gone by the time a data source that outlived it is read.
        if (!_stage) {
            return nullptr;
        }
        return _IsImplicitSurface(_primType) ? _ImplicitSurfaceExtent() : _ModelExtent();
    }

    //! What the attribute reads as when nothing is authored: the schema's fallback.
    HdDataSourceBaseHandle _ImplicitSurfaceExtent() const
    {
        const UsdPrim prim = _stage->GetPrimAtPath(_primPath);
        VtVec3fArray  extent;
        if (prim && UsdGeomBoundable(prim).GetExtentAttr().Get(&extent, UsdTimeCode::Default())
            && extent.size() == 2) {
            return _ExtentDataSource(GfRange3d(extent.AsConst()[0], extent.AsConst()[1]));
        }
        return nullptr;
    }

    //! The extent a draw mode stand-in is built from: the same data source for as long as this
    //! prim is read, so the box of the children is computed once.
    HdDataSourceBaseHandle _ModelExtent()
    {
        std::call_once(_modelExtentMade, [this] { _modelExtent = _MakeModelExtent(); });
        return _modelExtent;
    }

    HdDataSourceBaseHandle _MakeModelExtent() const
    {
        // The model has to be drawn as a stand-in, and no extentsHint is to be resolved to an
        // extent in its stead; both are asked of the input, which is what the draw mode scene
        // index and the extent resolution ask.
        const HdBoolDataSourceHandle applyDrawMode
            = UsdImagingGeomModelSchema::GetFromParent(_input).GetApplyDrawMode();
        if (!applyDrawMode || !applyDrawMode->GetTypedValue(0.0f)
            || UsdImagingExtentsHintSchema::GetFromParent(_input).IsDefined()) {
            return nullptr;
        }

        // The draw mode that counts is the one the model inherits where it authors none.
        const UsdPrim prim = _stage->GetPrimAtPath(_primPath);
        if (!prim || UsdGeomModelAPI(prim).ComputeModelDrawMode() == UsdGeomTokens->default_) {
            return nullptr;
        }
        return _ModelExtentDataSource::New(_stage, _primPath);
    }

    const HdContainerDataSourceHandle _input;
    const UsdStageWeakPtr             _stage;
    const SdfPath                     _primPath;
    const TfToken                     _primType;

    std::once_flag         _modelExtentMade;
    HdDataSourceBaseHandle _modelExtent;
};

} // namespace

HdVP2FallbackExtentSceneIndexRefPtr
HdVP2FallbackExtentSceneIndex::New(const HdSceneIndexBaseRefPtr& inputSceneIndex)
{
    return TfCreateRefPtr(new HdVP2FallbackExtentSceneIndex(inputSceneIndex));
}

HdVP2FallbackExtentSceneIndex::HdVP2FallbackExtentSceneIndex(
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
    : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

void HdVP2FallbackExtentSceneIndex::SetStage(const UsdStageRefPtr& stage) { _stage = stage; }

HdSceneIndexPrim HdVP2FallbackExtentSceneIndex::GetPrim(const SdfPath& primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    // Only a prim path names a stage prim: the others, a subprim of an adapter for instance, have
    // no extent of their own to fall back from.
    if (_stage && prim.dataSource && primPath.IsPrimPath()) {
        prim.dataSource = _PrimDataSource::New(prim.dataSource, _stage, primPath, prim.primType);
    }
    return prim;
}

SdfPathVector HdVP2FallbackExtentSceneIndex::GetChildPrimPaths(const SdfPath& primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void HdVP2FallbackExtentSceneIndex::_PrimsAdded(
    const HdSceneIndexBase& /* sender */,
    const HdSceneIndexObserver::AddedPrimEntries& entries)
{
    _SendPrimsAdded(entries);
}

void HdVP2FallbackExtentSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase& /* sender */,
    const HdSceneIndexObserver::RemovedPrimEntries& entries)
{
    _SendPrimsRemoved(entries);
}

void HdVP2FallbackExtentSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase& /* sender */,
    const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    _SendPrimsDirtied(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
