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
#include "AL/usdmaya/fileio/translators/TranslatorContext.h"
#include "AL/usdmaya/fileio/translators/TranslatorTestType.h"

#include <pxr/base/tf/type.h>
#include <pxr/pxr.h>

#include <maya/MStatus.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

//----------------------------------------------------------------------------------------------------------------------
#ifndef AL_GENERATING_DOCS
class TranslatorTestPlugin : public TranslatorBase
{
public:
    AL_USDMAYA_DECLARE_TRANSLATOR(TranslatorTestPlugin);

private:
    UsdPrim exportObject(
        UsdStageRefPtr        stage,
        MDagPath              dagPath,
        const SdfPath&        usdPath,
        const ExporterParams& params) override;
    MStatus    initialize() override;
    MStatus    import(const UsdPrim& prim, MObject& parent, MObject& createdObj) override;
    MStatus    postImport(const UsdPrim& prim) override;
    MStatus    preTearDown(UsdPrim& path) override;
    MStatus    tearDown(const SdfPath& path) override;
    ExportFlag canExport(const MObject& obj) override
    {
        return (
            obj.hasFn(MFn::kDistance) ? ExportFlag::kFallbackSupport : ExportFlag::kNotSupported);
    }
};
#endif

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
