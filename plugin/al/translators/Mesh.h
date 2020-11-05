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

PXR_NAMESPACE_OPEN_SCOPE

// forward declare usd types
class UsdGeomMesh;

PXR_NAMESPACE_CLOSE_SCOPE

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

//----------------------------------------------------------------------------------------------------------------------
/// \brief Class to translate a mesh in and out of maya.
//----------------------------------------------------------------------------------------------------------------------
class Mesh : public TranslatorBase
{
public:
    AL_USDMAYA_DECLARE_TRANSLATOR(Mesh);

private:
    MStatus initialize() override;
    MStatus import(const UsdPrim& prim, MObject& parent, MObject& createdObj) override;
    UsdPrim exportObject(
        UsdStageRefPtr        stage,
        MDagPath              dagPath,
        const SdfPath&        usdPath,
        const ExporterParams& params) override;
    MStatus tearDown(const SdfPath& path) override;
    MStatus update(const UsdPrim& path) override;
    MStatus preTearDown(UsdPrim& prim) override;

    bool supportsUpdate() const override
    {
        return false;
    } // Turned off supportsUpdate to get tearDown working correctly
    bool importableByDefault() const override { return false; }

    ExportFlag canExport(const MObject& obj) override
    {
        return obj.hasFn(MFn::kMesh) ? ExportFlag::kFallbackSupport : ExportFlag::kNotSupported;
    }
    bool canBeOverridden() override { return true; }

private:
    enum WriteOptions
    {
        kPerformDiff = 1 << 0,
        kDynamicAttributes = 1 << 1
    };
    void writeEdits(
        MDagPath&            dagPath,
        PXR_NS::UsdGeomMesh& geomPrim,
        uint32_t             options = kDynamicAttributes);
    static MObject m_visible;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
