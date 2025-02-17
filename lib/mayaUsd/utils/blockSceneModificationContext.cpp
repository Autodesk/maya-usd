//
// Copyright 2018 Pixar
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
#include "blockSceneModificationContext.h"

#include <pxr/base/tf/stringUtils.h>

#include <maya/MGlobal.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

namespace MAYAUSD_NS_DEF {
namespace utils {

MAYAUSD_VERIFY_CLASS_NOT_MOVE_OR_COPY(BlockSceneModificationContext);
MAYAUSD_VERIFY_CLASS_VIRTUAL_DESTRUCTOR(BlockSceneModificationContext);

BlockSceneModificationContext::BlockSceneModificationContext()
{
    const MString fileModifiedCmd("file -query -modified");
    int           cmdResult = 0;
    MStatus       status = MGlobal::executeCommand(fileModifiedCmd, cmdResult);
    CHECK_MSTATUS(status);

    _sceneWasModified = (cmdResult != 0);
}

/* virtual */
BlockSceneModificationContext::~BlockSceneModificationContext()
{
    const MString setFileModifiedCmd(
        PXR_NS::TfStringPrintf("file -modified %d", _sceneWasModified ? 1 : 0).c_str());

    MStatus status = MGlobal::executeCommand(setFileModifiedCmd);
    CHECK_MSTATUS(status);
}

} // namespace utils
} // namespace MAYAUSD_NS_DEF
