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
#ifndef PXRUSDMAYA_TRANSLATOR_CAMERA_H
#define PXRUSDMAYA_TRANSLATOR_CAMERA_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primReaderArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>

#include <pxr/pxr.h>
#include <pxr/usd/usdGeom/camera.h>

#include <maya/MFnCamera.h>
#include <maya/MObject.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \brief Provides helper functions for translating to/from UsdGeomCamera
struct UsdMayaTranslatorCamera
{
    /// Reads a UsdGeomCamera \p usdCamera from USD and creates a Maya
    /// MFnCamera under \p parentNode.
    MAYAUSD_CORE_PUBLIC
    static bool Read(
        const UsdGeomCamera&         usdCamera,
        MObject                      parentNode,
        const UsdMayaPrimReaderArgs& args,
        UsdMayaPrimReaderContext*    context);

    /// Helper function to access just the logic that writes from a non-animated
    /// camera into an existing maya camera.
    MAYAUSD_CORE_PUBLIC
    static bool ReadToCamera(const UsdGeomCamera& usdCamera, MFnCamera& cameraObject);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
