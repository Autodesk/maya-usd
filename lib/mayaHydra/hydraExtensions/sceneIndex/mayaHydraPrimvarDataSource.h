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

#ifndef MAYAHYDRAPRIMVARVALUEDATASOURCE_H
#define MAYAHYDRAPRIMVARVALUEDATASOURCE_H

#include <pxr/pxr.h>
#include <pxr/base/tf/denseHashMap.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/imaging/hd/dataSource.h>
#include <pxr/imaging/hd/enums.h>


PXR_NAMESPACE_OPEN_SCOPE

class MayaHydraAdapter;

/**
 * \brief A container data source representing data unique to primvars
 */
class MayaHydraPrimvarsDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(MayaHydraPrimvarsDataSource);

    MayaHydraPrimvarsDataSource(
        MayaHydraAdapter* adapter);

    void AddDesc(
        const TfToken& name,
        const TfToken& interpolation,
        const TfToken& role, bool indexed);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken& name) override;

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
    MayaHydraAdapter* _adapter;
};

HD_DECLARE_DATASOURCE_HANDLES(MayaHydraPrimvarsDataSource);

/**
 * \brief A container data source representing data unique to primvar value
 */
class MayaHydraPrimvarValueDataSource : public HdSampledDataSource
{
public:
    HD_DECLARE_DATASOURCE(MayaHydraPrimvarValueDataSource);

    MayaHydraPrimvarValueDataSource(
        const TfToken& primvarName,
        MayaHydraAdapter* adapter);

    VtValue GetValue(Time shutterOffset) override;

    bool GetContributingSampleTimesForInterval(
        Time startTime, Time endTime,
        std::vector<Time>* outSampleTimes) override;

private:
    TfToken _primvarName;
    MayaHydraAdapter* _adapter;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRAPRIMVARVALUEDATASOURCE_H
