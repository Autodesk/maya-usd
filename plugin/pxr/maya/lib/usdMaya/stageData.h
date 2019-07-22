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

/// \file usdMaya/stageData.h

#include "usdMaya/api.h"

#include "pxr/pxr.h"

#include "pxr/base/tf/staticTokens.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/stage.h"

#include <maya/MMessage.h>
#include <maya/MPxData.h>
#include <maya/MPxGeometryData.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>


PXR_NAMESPACE_OPEN_SCOPE


#define PXRUSDMAYA_STAGE_DATA_TOKENS \
    ((MayaTypeName, "pxrUsdStageData"))

TF_DECLARE_PUBLIC_TOKENS(UsdMayaStageDataTokens,
                         PXRUSDMAYA_API,
                         PXRUSDMAYA_STAGE_DATA_TOKENS);


class UsdMayaStageData : public MPxGeometryData
{
    public:
        /// Unlike other Maya node types, MPxData/MPxGeometryData declare
        /// typeId() as a pure virtual method that must be overridden in
        /// derived classes, so we have to call this static member "mayaTypeId"
        /// instead of just "typeId" as we usually would.
        PXRUSDMAYA_API
        static const MTypeId mayaTypeId;
        PXRUSDMAYA_API
        static const MString typeName;

        PXRUSDMAYA_API
        static void* creator();

        /**
         * \name MPxGeometryData overrides
         */
        //@{

        PXRUSDMAYA_API
        void copy(const MPxData& src) override;

        PXRUSDMAYA_API
        MTypeId typeId() const override;

        PXRUSDMAYA_API
        MString name() const override;
        //@}

        PXRUSDMAYA_API
        void registerExitCallback();

        PXRUSDMAYA_API
        void unregisterExitCallback();

        /**
         * \name data
         */
        //@{

        UsdStageRefPtr stage;
        SdfPath primPath;

        //@}

    private:
        UsdMayaStageData();
        ~UsdMayaStageData() override;

        UsdMayaStageData(const UsdMayaStageData&);
        UsdMayaStageData& operator=(const UsdMayaStageData&);

        MCallbackId _exitCallbackId;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
