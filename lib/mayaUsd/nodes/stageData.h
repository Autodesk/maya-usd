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

#define PXRMAYAUSD_STAGE_DATA_TOKENS ((MayaTypeName, "pxrUsdStageData"))

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

    // The original Pixar code was the following:
    //
    // UsdStageRefPtr stage;
    //
    // Now using a weak pointer instead of a referencing pointer.  The AL
    // plugin originally used a referencing pointer, but ran into problems.
    // From a Rob Bateman @ AL e-mail, 7-Apr-2019:
    //
    // "The reason for the weak pointer was that we found that Maya seemed
    // to have a memory leak (as in, the MPxData derived objects were being
    // created, but never deleted when we expected them to be - possibly by
    // design?). We had originally used a direct mirror of the Pixar data
    // object, but because the data objects were never freed (even after
    // file new), the original stage would be retained. This meant the
    // internal SdfLayerCache in USD would keep hold of the previously
    // loaded layers. So we found we had this problem:
    //
    // 1. Import some.usda file into a proxy shape.
    // 2. Make some modifications
    // 3. File New
    // 4. Import the same "some.usda" file into a proxy shape.
    //
    // At this point, USD would essentially hand you back the stage
    // composed of the modified layers, rather than a clean stage composed
    // from the files on disk. Switching the type from a shared_ptr to a
    // weak_ptr worked around this issue. Basically if we use a shared
    // pointer here, the only way to reload a scene, would be to restart
    // Maya."
    //
    // Logged as https://github.com/Autodesk/maya-usd/issues/528

    UsdStageWeakPtr stage;
    SdfPath         primPath;

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
