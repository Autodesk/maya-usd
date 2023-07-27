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

#include "mayaHydraDisplayStyleDataSource.h"

#include <mayaHydraLib/adapters/adapter.h>
#include <mayaHydraLib/sceneIndex/mayaHydraSceneIndex.h>

#include "pxr/imaging/hd/legacyDisplayStyleSchema.h"
#include <pxr/imaging/hd/retainedDataSource.h>

PXR_NAMESPACE_OPEN_SCOPE

MayaHydraDisplayStyleDataSource::MayaHydraDisplayStyleDataSource(
    const SdfPath& id,
    TfToken type,
    MayaHydraSceneIndex* sceneIndex,
    MayaHydraAdapter* adapter)
    : _id(id)
    , _type(type)
    , _sceneIndex(sceneIndex)
    , _adapter(adapter)
    , _displayStyleRead(false)
{
}


TfTokenVector
MayaHydraDisplayStyleDataSource::GetNames()
{
    TfTokenVector results;
    results.push_back(HdLegacyDisplayStyleSchemaTokens->refineLevel);
    results.push_back(HdLegacyDisplayStyleSchemaTokens->flatShadingEnabled);
    results.push_back(HdLegacyDisplayStyleSchemaTokens->displacementEnabled);
    results.push_back(HdLegacyDisplayStyleSchemaTokens->occludedSelectionShowsThrough);
    results.push_back(HdLegacyDisplayStyleSchemaTokens->pointsShadingEnabled);
    results.push_back(HdLegacyDisplayStyleSchemaTokens->materialIsFinal);
    results.push_back(HdLegacyDisplayStyleSchemaTokens->shadingStyle);
    results.push_back(HdLegacyDisplayStyleSchemaTokens->reprSelector);
    results.push_back(HdLegacyDisplayStyleSchemaTokens->cullStyle);
    return results;
}

HdDataSourceBaseHandle
MayaHydraDisplayStyleDataSource::Get(const TfToken& name)
{
    if (name == HdLegacyDisplayStyleSchemaTokens->refineLevel) {
        if (!_displayStyleRead) {
            _displayStyle = _adapter->GetDisplayStyle();
            _displayStyleRead = true;
        }
        return (_displayStyle.refineLevel != 0)
            ? HdRetainedTypedSampledDataSource<int>::New(
                _displayStyle.refineLevel)
            : nullptr;
    }
    else if (name == HdLegacyDisplayStyleSchemaTokens->flatShadingEnabled) {
        if (!_displayStyleRead) {
            _displayStyle = _adapter->GetDisplayStyle();
            _displayStyleRead = true;
        }
        return HdRetainedTypedSampledDataSource<bool>::New(
            _displayStyle.flatShadingEnabled);
    }
    else if (name == HdLegacyDisplayStyleSchemaTokens->displacementEnabled) {
        if (!_displayStyleRead) {
            _displayStyle = _adapter->GetDisplayStyle();
            _displayStyleRead = true;
        }
        return HdRetainedTypedSampledDataSource<bool>::New(
            _displayStyle.displacementEnabled);
    }
    else if (name == HdLegacyDisplayStyleSchemaTokens->occludedSelectionShowsThrough) {
        if (!_displayStyleRead) {
            _displayStyle = _adapter->GetDisplayStyle();
            _displayStyleRead = true;
        }
        return HdRetainedTypedSampledDataSource<bool>::New(
            _displayStyle.occludedSelectionShowsThrough);
    }
    else if (name == HdLegacyDisplayStyleSchemaTokens->pointsShadingEnabled) {
        if (!_displayStyleRead) {
            _displayStyle = _adapter->GetDisplayStyle();
            _displayStyleRead = true;
        }
        return HdRetainedTypedSampledDataSource<bool>::New(
            _displayStyle.pointsShadingEnabled);
    }
    else if (name == HdLegacyDisplayStyleSchemaTokens->materialIsFinal) {
        if (!_displayStyleRead) {
            _displayStyle = _adapter->GetDisplayStyle();
            _displayStyleRead = true;
        }
        return HdRetainedTypedSampledDataSource<bool>::New(
            _displayStyle.materialIsFinal);
    }
    else if (name == HdLegacyDisplayStyleSchemaTokens->shadingStyle) {
        TfToken shadingStyle = _sceneIndex->GetShadingStyle(_id)
            .GetWithDefault<TfToken>();
        if (shadingStyle.IsEmpty()) {
            return nullptr;
        }
        return HdRetainedTypedSampledDataSource<TfToken>::New(shadingStyle);
    }
    else if (name == HdLegacyDisplayStyleSchemaTokens->reprSelector) {
        HdReprSelector repr;
        // TODO: Get the reprSelector
        //HdSceneIndexPrim prim = _sceneIndex->GetPrim(_id);
        //if (HdLegacyDisplayStyleSchema styleSchema =
        //    HdLegacyDisplayStyleSchema::GetFromParent(prim.dataSource)) {

        //    if (HdTokenArrayDataSourceHandle ds =
        //        styleSchema.GetReprSelector()) {
        //        VtArray<TfToken> ar = ds->GetTypedValue(0.0f);
        //        ar.resize(HdReprSelector::MAX_TOPOLOGY_REPRS);
        //        repr = HdReprSelector(ar[0], ar[1], ar[2]);
        //    }
        //}
        HdTokenArrayDataSourceHandle reprSelectorDs = nullptr;
        bool empty = true;
        for (size_t i = 0; i < HdReprSelector::MAX_TOPOLOGY_REPRS; ++i) {
            if (!repr[i].IsEmpty()) {
                empty = false;
                break;
            }
        }
        if (!empty) {
            VtArray<TfToken> array(HdReprSelector::MAX_TOPOLOGY_REPRS);
            for (size_t i = 0; i < HdReprSelector::MAX_TOPOLOGY_REPRS; ++i) {
                array[i] = repr[i];
            }
            reprSelectorDs =
                HdRetainedTypedSampledDataSource<VtArray<TfToken>>::New(
                    array);
        }
        return reprSelectorDs;
    }
    else if (name == HdLegacyDisplayStyleSchemaTokens->cullStyle) {
        HdCullStyle cullStyle = _adapter->GetCullStyle();
        if (cullStyle == HdCullStyleDontCare) {
            return nullptr;
        }
        TfToken cullStyleToken;
        switch (cullStyle) {
        case HdCullStyleNothing:
            cullStyleToken = HdCullStyleTokens->nothing;
            break;
        case HdCullStyleBack:
            cullStyleToken = HdCullStyleTokens->back;
            break;
        case HdCullStyleFront:
            cullStyleToken = HdCullStyleTokens->front;
            break;
        case HdCullStyleBackUnlessDoubleSided:
            cullStyleToken = HdCullStyleTokens->backUnlessDoubleSided;
            break;
        case HdCullStyleFrontUnlessDoubleSided:
            cullStyleToken = HdCullStyleTokens->frontUnlessDoubleSided;
            break;
        default:
            cullStyleToken = HdCullStyleTokens->dontCare;
            break;
        }
        return HdRetainedTypedSampledDataSource<TfToken>::New(cullStyleToken);
    }
    else {
        return nullptr;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
