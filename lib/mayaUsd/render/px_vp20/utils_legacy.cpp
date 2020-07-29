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
#include "utils_legacy.h"

#include <pxr/base/gf/matrix4d.h>
#include <pxr/pxr.h>

#include <maya/M3dView.h>
#include <maya/MDagPath.h>
#include <maya/MMatrix.h>
#include <maya/MSelectInfo.h>
#include <maya/MStatus.h>

PXR_NAMESPACE_OPEN_SCOPE

/* static */
bool px_LegacyViewportUtils::GetSelectionMatrices(
    MSelectInfo& selectInfo,
    GfMatrix4d&  viewMatrix,
    GfMatrix4d&  projectionMatrix)
{
    MStatus status;

    M3dView view = selectInfo.view();

    MDagPath cameraDagPath;
    status = view.getCamera(cameraDagPath);
    CHECK_MSTATUS_AND_RETURN(status, false);

    const MMatrix transformMat = cameraDagPath.inclusiveMatrix(&status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MMatrix projectionMat;
    status = view.projectionMatrix(projectionMat);
    CHECK_MSTATUS_AND_RETURN(status, false);

    unsigned int viewportOriginX;
    unsigned int viewportOriginY;
    unsigned int viewportWidth;
    unsigned int viewportHeight;
    status = view.viewport(viewportOriginX, viewportOriginY, viewportWidth, viewportHeight);
    CHECK_MSTATUS_AND_RETURN(status, false);

    unsigned int selectRectX;
    unsigned int selectRectY;
    unsigned int selectRectWidth;
    unsigned int selectRectHeight;
    selectInfo.selectRect(selectRectX, selectRectY, selectRectWidth, selectRectHeight);

    MMatrix selectionMatrix;
    selectionMatrix[0][0] = (double)viewportWidth / (double)selectRectWidth;
    selectionMatrix[1][1] = (double)viewportHeight / (double)selectRectHeight;
    selectionMatrix[3][0] = ((double)viewportWidth - (double)(selectRectX * 2 + selectRectWidth))
        / (double)selectRectWidth;
    selectionMatrix[3][1] = ((double)viewportHeight - (double)(selectRectY * 2 + selectRectHeight))
        / (double)selectRectHeight;

    projectionMat *= selectionMatrix;

    viewMatrix = GfMatrix4d(transformMat.matrix).GetInverse();
    projectionMatrix = GfMatrix4d(projectionMat.matrix);

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
