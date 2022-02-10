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
#include "AL/usdmaya/Metadata.h"
#include "AL/usdmaya/StageCache.h"
#include "AL/usdmaya/nodes/Layer.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "test_usdmaya.h"

#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>

#include <maya/MDagModifier.h>
#include <maya/MFileIO.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MSelectionList.h>
#include <maya/MStringArray.h>

using AL::maya::test::buildTempPath;

/*
 * Test that prims that are marked as unselectable are picked when opening a new stage
 * and that prims whos selectability is modified is picked up
 */
TEST(ProxyShapeSelectabilityDB, selectablesOnOpen)
{
    SdfPath                         expectedSelectable("/A/B");
    std::function<UsdStageRefPtr()> constructTransformChain = [&expectedSelectable]() {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        stage->DefinePrim(SdfPath("/A/B/C"));
        UsdPrim b = stage->GetPrimAtPath(expectedSelectable);
        b.SetMetadata<TfToken>(
            AL::usdmaya::Metadata::selectability, AL::usdmaya::Metadata::unselectable);
        return stage;
    };

    MFileIO::newFile(true);
    const std::string temp_path
        = buildTempPath("AL_USDMayaTests_ProxyShape_selectablesOnOpen.usda");
    AL::usdmaya::nodes::ProxyShape* proxyShape
        = CreateMayaProxyShape(constructTransformChain, temp_path);

    // Check that the path is selectable directly using the selectableDB object
    EXPECT_TRUE(proxyShape->isPathUnselectable(expectedSelectable));
}

/*
 * Tests that changing a prim to be unselectable on the fly is picked up by the proxy shape
 */
TEST(ProxyShapeSelectabilityDB, selectablesOnModification)
{
    std::function<UsdStageRefPtr()> constructTransformChain = []() {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        stage->DefinePrim(SdfPath("/A/B/C"));
        return stage;
    };

    MFileIO::newFile(true);

    // unsure undo is enabled for this test
    const std::string temp_path
        = buildTempPath("AL_USDMayaTests_ProxyShape_selectablesOnModification.usda");
    AL::usdmaya::nodes::ProxyShape* proxyShape
        = CreateMayaProxyShape(constructTransformChain, temp_path);

    SdfPath expectedSelectable("/A/B");
    EXPECT_FALSE(proxyShape->isPathUnselectable(expectedSelectable));

    UsdPrim b = proxyShape->getUsdStage()->GetPrimAtPath(expectedSelectable);
    b.SetMetadata(AL::usdmaya::Metadata::selectability, AL::usdmaya::Metadata::unselectable);

    // Check that the path is selectable directly using the selectableDB object
    EXPECT_TRUE(proxyShape->isPathUnselectable(expectedSelectable));
}

/*
 * Tests that when a prim is tagged as unselected, and it's selectability changes from not being
 * unselectable any more, that the selectability database removes it from the unselectable list.
 */

TEST(ProxyShapeSelectabilityDB, selectableIsRemoval)
{
    SdfPath                         expectedSelectable("/A/B");
    std::function<UsdStageRefPtr()> constructTransformChain = [&expectedSelectable]() {
        UsdStageRefPtr stage = UsdStage::CreateInMemory();
        stage->DefinePrim(SdfPath("/A/B/C"));
        UsdPrim b = stage->GetPrimAtPath(expectedSelectable);

        b.SetMetadata(AL::usdmaya::Metadata::selectability, AL::usdmaya::Metadata::unselectable);
        return stage;
    };

    MFileIO::newFile(true);
    const std::string temp_path
        = buildTempPath("AL_USDMayaTests_ProxyShape_selectableIsRemoval.usda");
    AL::usdmaya::nodes::ProxyShape* proxyShape
        = CreateMayaProxyShape(constructTransformChain, temp_path);

    EXPECT_TRUE(proxyShape->isPathUnselectable(expectedSelectable));

    UsdPrim b = proxyShape->getUsdStage()->GetPrimAtPath(expectedSelectable);
    b.SetMetadata(AL::usdmaya::Metadata::selectability, AL::usdmaya::Metadata::selectable);

    // Check that the path has been removed from the selectable list
    EXPECT_FALSE(proxyShape->isPathUnselectable(expectedSelectable));
}
