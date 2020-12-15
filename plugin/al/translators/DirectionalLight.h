//
// Copyright 2018 Animal Logic
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

#include <mayaUsdUtils/ForwardDeclares.h>

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

//----------------------------------------------------------------------------------------------------------------------
/// \brief Class to translate a directional light in and out of maya.
//----------------------------------------------------------------------------------------------------------------------
class DirectionalLight : public TranslatorBase
{
public:
    AL_USDMAYA_DECLARE_TRANSLATOR(DirectionalLight);

    MStatus initialize() override;
    MStatus import(const UsdPrim& prim, MObject& parent, MObject& createObj) override;
    UsdPrim exportObject(
        UsdStageRefPtr        stage,
        MDagPath              dagPath,
        const SdfPath&        usdPath,
        const ExporterParams& params) override;
    MStatus preTearDown(UsdPrim& prim) override;
    MStatus tearDown(const SdfPath& path) override;
    MStatus update(const UsdPrim& prim) override;
    MStatus updateMayaAttributes(MObject mayaObj, const UsdPrim& prim);
    bool    updateUsdPrim(const UsdStageRefPtr& stage, const SdfPath& path, const MObject& mayaObj);
    bool    supportsUpdate() const override { return true; }
    ExportFlag canExport(const MObject& obj) override
    {
        return obj.hasFn(MFn::kDirectionalLight) ? ExportFlag::kFallbackSupport
                                                 : ExportFlag::kNotSupported;
    }
    bool canBeOverridden() override { return true; }

private:
    static MObject m_pointWorld;
    static MObject m_lightAngle;
    static MObject m_color;
    static MObject m_intensity;
    static MObject m_exposure;
    static MObject m_diffuse;
    static MObject m_specular;
    static MObject m_normalize;
    static MObject m_enableColorTemperature;
};

} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//-----------------------------------------------------------------------------
