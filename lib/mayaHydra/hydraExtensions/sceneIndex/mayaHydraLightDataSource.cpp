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

#include "mayaHydraLightDataSource.h"

#include <mayaHydraLib/adapters/lightAdapter.h>

#include <pxr/imaging/hd/lightSchema.h>
#include <pxr/imaging/hd/retainedDataSource.h>

PXR_NAMESPACE_OPEN_SCOPE

MayaHydraLightDataSource::MayaHydraLightDataSource(
    const SdfPath& id,
    TfToken type,
    MayaHydraAdapter* adapter)
    : _id(id)
    , _type(type)
    , _adapter(adapter)
{
}


TfTokenVector
MayaHydraLightDataSource::GetNames()
{
    TfTokenVector result = {
        HdTokens->filters,
        HdTokens->lightLink,
        HdTokens->shadowLink,
        HdTokens->lightFilterLink,
        HdTokens->isLight,
    };
    return result;
}

HdDataSourceBaseHandle
MayaHydraLightDataSource::Get(const TfToken& name)
{
    VtValue v;
    if (_UseGet(name)) {
        v = _adapter->Get(name);
    }
    else {
        MayaHydraLightAdapter* lightAdapter = dynamic_cast<MayaHydraLightAdapter*>(_adapter);
        if (!lightAdapter) {
            return nullptr;
        }
        v = lightAdapter->GetLightParamValue(name);
    }

    if (name == HdLightTokens->params) {
        return HdRetainedSampledDataSource::New(v);
    }
    else if (name == HdLightTokens->shadowParams) {
        return HdRetainedSampledDataSource::New(v);
    }
    else if (name == HdLightTokens->shadowCollection) {
        return HdRetainedSampledDataSource::New(v);
    }
    else {
        return HdCreateTypedRetainedDataSource(v);
    }
}

bool MayaHydraLightDataSource::_UseGet(const TfToken& name) const {
    if (name == HdLightTokens->params ||
        name == HdLightTokens->shadowParams ||
        name == HdLightTokens->shadowCollection) {
        return true;
    }

    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
