//
// Copyright 2017 Animal Logic
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
#pragma once

#include "AL/usdmaya/fileio/translators/TranslatorBase.h"

#include <pxr/usd/usd/stage.h>

#include <mayaUsdUtils/ForwardDeclares.h>

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

//----------------------------------------------------------------------------------------------------------------------
/// \brief Class to translate an image plane in and out of maya.
//----------------------------------------------------------------------------------------------------------------------
class Camera : public TranslatorBase
{
public:
    AL_USDMAYA_DECLARE_TRANSLATOR(Camera);

    AL_USDMAYA_PUBLIC MStatus initialize() override;
    MStatus import(const UsdPrim& prim, MObject& parent, MObject& createdObj) override;
    UsdPrim exportObject(
        UsdStageRefPtr        stage,
        MDagPath              dagPath,
        const SdfPath&        usdPath,
        const ExporterParams& params) override;
    MStatus tearDown(const SdfPath& path) override;
    MStatus update(const UsdPrim& path) override;
    bool    supportsUpdate() const override { return true; }

    void checkCurrentCameras(MObject cameraNode);

    ExportFlag canExport(const MObject& obj) override
    {
        return obj.hasFn(MFn::kCamera) ? ExportFlag::kFallbackSupport : ExportFlag::kNotSupported;
    }

protected:
    AL_USDMAYA_PUBLIC virtual MStatus updateAttributes(MObject to, const UsdPrim& prim);
    AL_USDMAYA_PUBLIC virtual void
    writePrim(UsdPrim& prim, MDagPath dagPath, const ExporterParams& params);

private:
    static MObject m_orthographic;
    static MObject m_horizontalFilmAperture;
    static MObject m_verticalFilmAperture;
    static MObject m_horizontalFilmApertureOffset;
    static MObject m_verticalFilmApertureOffset;
    static MObject m_focalLength;
    static MObject m_nearDistance;
    static MObject m_farDistance;
    static MObject m_fstop;
    static MObject m_focusDistance;
    static MObject m_lensSqueezeRatio;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
