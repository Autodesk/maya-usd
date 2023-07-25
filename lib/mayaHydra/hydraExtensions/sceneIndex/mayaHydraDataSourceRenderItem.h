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

#ifndef MAYAHYDRADATASOURCERENDERITEM_H
#define MAYAHYDRADATASOURCERENDERITEM_H

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/imaging/hd/dataSource.h>
#include <pxr/imaging/hd/enums.h>

PXR_NAMESPACE_OPEN_SCOPE

class MayaHydraRenderItemAdapter;
/**
 * \brief A container data source representing data unique to render item
 */
 class MayaHydraDataSourceRenderItem : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(MayaHydraDataSourceRenderItem);

    // ------------------------------------------------------------------------
    // HdContainerDataSource implementations
    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken& name) override;

private:
    MayaHydraDataSourceRenderItem(
        const SdfPath& id,
        TfToken type,
        MayaHydraRenderItemAdapter* riAdapter);

    HdDataSourceBaseHandle _GetVisibilityDataSource();
    HdDataSourceBaseHandle _GetPrimvarsDataSource();
    TfToken _InterpolationAsToken(HdInterpolation interpolation);
private:
    SdfPath _id;
    TfToken _type;
    MayaHydraRenderItemAdapter* _riAdapter = nullptr;

    std::atomic_bool _primvarsBuilt;
    HdContainerDataSourceAtomicHandle _primvars;
};

HD_DECLARE_DATASOURCE_HANDLES(MayaHydraDataSourceRenderItem);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRADATASOURCERENDERITEM_H
