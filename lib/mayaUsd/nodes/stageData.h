//
// Copyright 2016 Pixar
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
#ifndef PXRUSDMAYA_STAGE_DATA_H
#define PXRUSDMAYA_STAGE_DATA_H

#include <mayaUsd/base/api.h>

#include <pxr/base/tf/staticTokens.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MMessage.h>
#include <maya/MPxData.h>
#include <maya/MPxGeometryData.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
#define PXRMAYAUSD_STAGE_DATA_TOKENS \
    ((MayaTypeName, "pxrUsdStageData"))
// clang-format on

TF_DECLARE_PUBLIC_TOKENS(MayaUsdStageDataTokens, MAYAUSD_CORE_PUBLIC, PXRMAYAUSD_STAGE_DATA_TOKENS);

class MayaUsdStageData : public MPxGeometryData
{
public:
    /// Unlike other Maya node types, MPxData/MPxGeometryData declare
    /// typeId() as a pure virtual method that must be overridden in
    /// derived classes, so we have to call this static member "mayaTypeId"
    /// instead of just "typeId" as we usually would.
    MAYAUSD_CORE_PUBLIC
    static const MTypeId mayaTypeId;
    MAYAUSD_CORE_PUBLIC
    static const MString typeName;

    MAYAUSD_CORE_PUBLIC
    static void* creator();

    /**
     * \name MPxGeometryData overrides
     */
    //@{

    MAYAUSD_CORE_PUBLIC
    void copy(const MPxData& src) override;

    MAYAUSD_CORE_PUBLIC
    MTypeId typeId() const override;

    MAYAUSD_CORE_PUBLIC
    MString name() const override;
    //@}

    void unregisterExitCallback();

    /**
     * \name data
     */
    //@{

    UsdStageRefPtr stage;

    SdfPath primPath;

    //@}

protected:
    MAYAUSD_CORE_PUBLIC
    MayaUsdStageData();
    MAYAUSD_CORE_PUBLIC
    ~MayaUsdStageData() override;

private:
    MayaUsdStageData(const MayaUsdStageData&);
    MayaUsdStageData& operator=(const MayaUsdStageData&);

    void registerExitCallback();

    MCallbackId _exitCallbackId;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
