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
#include "AL/usdmaya/utils/ForwardDeclares.h"

#include <maya/MStatus.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

//----------------------------------------------------------------------------------------------------------------------
/// \brief Class to setup the frame range in maya.
//----------------------------------------------------------------------------------------------------------------------

class FrameRange : public TranslatorBase
{
public:
    AL_USDMAYA_DECLARE_TRANSLATOR(FrameRange);

private:
    MStatus import(const UsdPrim& prim, MObject& parent, MObject& createdObj) override;
    MStatus postImport(const UsdPrim& prim) override;
    MStatus preTearDown(UsdPrim& prim) override;
    MStatus tearDown(const SdfPath& path) override;
    MStatus update(const UsdPrim& prim) override;

    bool supportsUpdate() const override { return true; }

    bool supportsInactive() const { return true; }

    bool needsTransformParent() const override { return false; }

    MStatus setFrameRange(const UsdPrim& prim, bool setCurrentFrame);
    MStatus getFrame(
        const UsdStageWeakPtr& stage,
        const UsdPrim&         prim,
        const UsdAttribute&    attr,
        const TfToken&         fallbackMetadataKey,
        double*                frame) const;
    bool canBeOverridden() override { return true; }
};

} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
