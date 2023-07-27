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

#ifndef MAYAHYDRALIGHTDATASOURCE_H
#define MAYAHYDRALIGHTDATASOURCE_H

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/imaging/hd/dataSource.h>
#include <pxr/imaging/hd/enums.h>

PXR_NAMESPACE_OPEN_SCOPE

class MayaHydraAdapter;
/**
 * \brief A container data source representing data unique to light
 */
 class MayaHydraLightDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(MayaHydraLightDataSource);

    // ------------------------------------------------------------------------
    // HdContainerDataSource implementations
    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken& name) override;

private:
    MayaHydraLightDataSource(
        const SdfPath& id,
        TfToken type,
        MayaHydraAdapter* adapter);

    bool _UseGet(const TfToken& name) const;

    SdfPath _id;
    TfToken _type;
    MayaHydraAdapter* _adapter = nullptr;
};

HD_DECLARE_DATASOURCE_HANDLES(MayaHydraLightDataSource);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRALIGHTDATASOURCE_H
