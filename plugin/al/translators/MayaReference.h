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
#include "AL/usdmaya/utils/ForwardDeclares.h"

#include <pxr/usd/usd/stage.h>

#include <maya/MFnReference.h>

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

//----------------------------------------------------------------------------------------------------------------------
/// \brief Class to translate an maya reference into maya.
//----------------------------------------------------------------------------------------------------------------------
class MayaReference : public TranslatorBase
{
public:
    AL_USDMAYA_DECLARE_TRANSLATOR(MayaReference);

protected:
    MStatus initialize() override;
    MStatus import(const UsdPrim& prim, MObject& parent, MObject& createdObj) override;
    MStatus tearDown(const SdfPath& path) override;
    MStatus update(const UsdPrim& path) override;
    bool    supportsUpdate() const override { return true; }

    bool canBeOverridden() override { return true; }
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief Class to translate an old schema maya reference into maya.
//----------------------------------------------------------------------------------------------------------------------
class ALMayaReference : public MayaReference
{
public:
    AL_USDMAYA_DECLARE_TRANSLATOR(ALMayaReference);
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace translators
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
