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
#include "AL/usdmaya/fileio/translators/TranslatorBase.h"
#include "AL/usdmaya/fileio/translators/TranslatorContext.h"
#include "AL/usdmaya/fileio/translators/TranslatorTestType.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "test_usdmaya.h"

#include <mayaUsd/nodes/stageData.h>

#include <pxr/base/tf/refPtr.h>
#include <pxr/base/tf/type.h>
#include <pxr/usd/usd/schemaBase.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MDagModifier.h>

using namespace AL::usdmaya::fileio::translators;

//----------------------------------------------------------------------------------------------------------------------
/// \brief  Test some of the functionality of the AL::usdmaya::TranslatorBase.
//----------------------------------------------------------------------------------------------------------------------

// test manufacturing of a TranslatorTest translator
// its instanciation looks for a TranslatorTestType TfType
TEST(translators_Translator, manufactureTranslator)
{
    // create a TranslatorTestType usd prim
    UsdStageRefPtr     m_stage = UsdStage::CreateInMemory();
    TranslatorTestType testPrim = TranslatorTestType::Define(m_stage, SdfPath("/testPrim"));
    UsdPrim            m_prim = testPrim.GetPrim();

    // dummy context
    TranslatorContextPtr context = TranslatorContext::create(0);

    TranslatorManufacture manufacture(context);
    TranslatorRefPtr      torBase = manufacture.getTranslatorFromId(
        TranslatorManufacture::TranslatorPrefixSchemaType.GetString()
        + m_prim.GetTypeName().GetString());

    EXPECT_TRUE(torBase);
}

TEST(translators_Translator, translatorContext)
{
    // create a TranslatorTestType usd prim
    UsdStageRefPtr     m_stage = UsdStage::CreateInMemory();
    TranslatorTestType testPrim = TranslatorTestType::Define(m_stage, SdfPath("/testPrim"));
    UsdPrim            m_prim = testPrim.GetPrim();

    TranslatorContextPtr context = TranslatorContext::create(nullptr);

    EXPECT_TRUE(context->getProxyShape() == nullptr);

    MDagModifier dm;
    MObject      tm = dm.createNode("transform");
    dm.doIt();
    context->registerItem(m_prim, tm);

    MObjectHandle handle;
    EXPECT_TRUE(context->getTransform(m_prim, handle));
    EXPECT_TRUE(handle.object() == tm);
}
