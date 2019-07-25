//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "stageData.h"

#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/tf/staticTokens.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/stage.h"

#include <maya/MGlobal.h>
#include <maya/MSceneMessage.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(MayaUsdStageDataTokens,
                        PXRMAYAUSD_STAGE_DATA_TOKENS);


const MTypeId MayaUsdStageData::mayaTypeId(0x0010A257);
const MString MayaUsdStageData::typeName(
    MayaUsdStageDataTokens->MayaTypeName.GetText());


/* This exists solely to make sure that the usdStage instance
 * gets discarded when Maya exits, so that any temporary files
 * that might have been created are unlinked.
 */
namespace {
void
_cleanUp(void *gdPtr)
{
    MayaUsdStageData *gd = (MayaUsdStageData *)gdPtr;

    gd->unregisterExitCallback();

    gd->stage = UsdStageRefPtr();
}
}

/* static */
void*
MayaUsdStageData::creator()
{
    return new MayaUsdStageData();
}

/* virtual */
void
MayaUsdStageData::copy(const MPxData& src)
{
    const MayaUsdStageData* stageData =
        dynamic_cast<const MayaUsdStageData*>(&src);

    if (stageData) {
        stage = stageData->stage;
        primPath = stageData->primPath;
    }
}

/* virtual */
MTypeId
MayaUsdStageData::typeId() const
{
    return mayaTypeId;
}

/* virtual */
MString
MayaUsdStageData::name() const
{
    return typeName;
}

MayaUsdStageData::MayaUsdStageData() : MPxGeometryData()
{
    registerExitCallback();
}

void
MayaUsdStageData::registerExitCallback()
{
    _exitCallbackId = MSceneMessage::addCallback(MSceneMessage::kMayaExiting,
                                                 _cleanUp,
                                                 this);
}

void
MayaUsdStageData::unregisterExitCallback()
{
    MSceneMessage::removeCallback(_exitCallbackId);
}

/* virtual */
MayaUsdStageData::~MayaUsdStageData() {

    unregisterExitCallback();
}


PXR_NAMESPACE_CLOSE_SCOPE
