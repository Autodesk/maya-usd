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
#ifndef PXRUSDTRANSLATORS_CAMERA_WRITER_H
#define PXRUSDTRANSLATORS_CAMERA_WRITER_H

/// \file pxrUsdTranslators/cameraWriter.h

#include "pxr/pxr.h"

#include "../../fileio/primWriter.h"
#include "../../fileio/writeJobContext.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/camera.h"

#include <maya/MFnDependencyNode.h>


PXR_NAMESPACE_OPEN_SCOPE


/// Exports Maya cameras to UsdGeomCamera.
class PxrUsdTranslators_CameraWriter : public UsdMayaPrimWriter
{
    public:
        PxrUsdTranslators_CameraWriter(
                const MFnDependencyNode& depNodeFn,
                const SdfPath& usdPath,
                UsdMayaWriteJobContext& jobCtx);

        void Write(const UsdTimeCode& usdTime) override;

    protected:
        bool writeCameraAttrs(
                const UsdTimeCode& usdTime,
                UsdGeomCamera& primSchema);
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
