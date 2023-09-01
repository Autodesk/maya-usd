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

#include "mayaHydraPrimvarDataSource.h"

#include <mayaHydraLib/adapters/adapter.h>

#include <pxr/imaging/hd/retainedDataSource.h>
#include <pxr/imaging/hd/primvarSchema.h>
#include <pxr/imaging/hd/primvarsSchema.h>

PXR_NAMESPACE_OPEN_SCOPE

MayaHydraPrimvarsDataSource::MayaHydraPrimvarsDataSource(
    MayaHydraAdapter* adapter)
    : _adapter(adapter)
{
}

void MayaHydraPrimvarsDataSource::AddDesc(
    const TfToken& name,
    const TfToken& interpolation,
    const TfToken& role, bool indexed)
{
    _entries[name] = { interpolation, role, indexed };
}

TfTokenVector MayaHydraPrimvarsDataSource::GetNames()
{
    TfTokenVector result;
    result.reserve(_entries.size());
    for (const auto& pair : _entries) {
        result.push_back(pair.first);
    }
    return result;
}

HdDataSourceBaseHandle MayaHydraPrimvarsDataSource::Get(const TfToken& name)
{
    _EntryMap::const_iterator it = _entries.find(name);
    if (it == _entries.end()) {
        return nullptr;
    }

    // Need to handle indexed case?
    assert(!(*it).second.indexed);
    return HdPrimvarSchema::Builder()
        .SetPrimvarValue(MayaHydraPrimvarValueDataSource::New(
            name, _adapter))
        .SetInterpolation(HdPrimvarSchema::BuildInterpolationDataSource(
            (*it).second.interpolation))
        .SetRole(HdPrimvarSchema::BuildRoleDataSource(
            (*it).second.role))
        .Build();
}

MayaHydraPrimvarValueDataSource::MayaHydraPrimvarValueDataSource(
    const TfToken& primvarName,
    MayaHydraAdapter* adapter)
    : _primvarName(primvarName)
    , _adapter(adapter)
{
}

VtValue MayaHydraPrimvarValueDataSource::GetValue(Time shutterOffset)
{
    return _adapter->Get(_primvarName);
}

bool MayaHydraPrimvarValueDataSource::GetContributingSampleTimesForInterval(
    Time startTime, Time endTime,
    std::vector<Time>* outSampleTimes)
{
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
