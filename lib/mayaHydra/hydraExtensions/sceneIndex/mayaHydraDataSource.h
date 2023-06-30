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

#ifndef MAYAHYDRADATASOURCE_H
#define MAYAHYDRADATASOURCE_H

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/imaging/hd/dataSource.h>
#include <pxr/imaging/hd/enums.h>

PXR_NAMESPACE_OPEN_SCOPE

class MayaHydraAdapter;
class MayaHydraSceneIndex;

/**
 * \brief A container data source representing data unique to render item
 */
 class MayaHydraDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(MayaHydraDataSource);

    // ------------------------------------------------------------------------
    // HdContainerDataSource implementations
    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken& name) override;

private:
    MayaHydraDataSource(
        const SdfPath& id,
        TfToken type,
        MayaHydraSceneIndex* sceneIndex,
        MayaHydraAdapter* adapter);

    HdDataSourceBaseHandle _GetVisibilityDataSource();
    HdDataSourceBaseHandle _GetPrimvarsDataSource();
    TfToken _InterpolationAsToken(HdInterpolation interpolation);
    HdDataSourceBaseHandle _GetMaterialBindingDataSource();
    HdDataSourceBaseHandle _GetMaterialDataSource();
private:
    SdfPath _id;
    TfToken _type;
    MayaHydraSceneIndex* _sceneIndex = nullptr;
    MayaHydraAdapter* _adapter = nullptr;

    std::atomic_bool _primvarsBuilt;
    HdContainerDataSourceAtomicHandle _primvars;
};

HD_DECLARE_DATASOURCE_HANDLES(MayaHydraDataSource);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRADATASOURCE_H
