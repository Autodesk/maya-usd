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
#ifndef PXRUSDMAYAGL_USER_DATA_H
#define PXRUSDMAYAGL_USER_DATA_H

/// \file pxrUsdMayaGL/userData.h

#include <mayaUsd/base/api.h>

#include <pxr/base/gf/vec4f.h>
#include <pxr/pxr.h>

#include <maya/MBoundingBox.h>
#include <maya/MUserData.h>

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

/// Container for all of the information needed for a draw request in the
/// legacy viewport or Viewport 2.0, without requiring shape querying at draw
/// time.
///
/// Maya shapes may implement their own derived classes of this class if they
/// require storage for additional data that's not specific to the batch
/// renderer.
class PxrMayaHdUserData : public MUserData
{
public:
    std::unique_ptr<MBoundingBox> boundingBox;
    std::unique_ptr<GfVec4f>      wireframeColor;

    MAYAUSD_CORE_PUBLIC
    PxrMayaHdUserData();

    MAYAUSD_CORE_PUBLIC
    ~PxrMayaHdUserData() override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
