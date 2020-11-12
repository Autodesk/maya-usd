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
#include "AL/maya/utils/Utils.h"
#include "AL/usdmaya/Metadata.h"
#include "AL/usdmaya/StageCache.h"
#include "AL/usdmaya/nodes/LayerManager.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "test_usdmaya.h"

#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/usd/variantSets.h>
#include <pxr/usd/usdGeom/xform.h>

#include <maya/MFileIO.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MSelectionList.h>
#include <maya/MUuid.h>

#include <functional>

using AL::maya::test::buildTempPath;

namespace {

bool checkLocked(const char* const name)
{
    MSelectionList sl;
    sl.add(name);
    MObject obj;
    EXPECT_TRUE(sl.getDependNode(0, obj) == MS::kSuccess);

    MFnDependencyNode fn(obj);
    MPlug             T = fn.findPlug("t");
    MPlug             S = fn.findPlug("s");
    MPlug             R = fn.findPlug("r");
    return T.isLocked() && S.isLocked() && R.isLocked();
}

bool checkUnlocked(const char* const name)
{
    MSelectionList sl;
    sl.add(name);
    MObject obj;
    EXPECT_TRUE(sl.getDependNode(0, obj) == MS::kSuccess);

    MFnDependencyNode fn(obj);
    MPlug             T = fn.findPlug("t");
    MPlug             S = fn.findPlug("s");
    MPlug             R = fn.findPlug("r");
    return !T.isLocked() && !S.isLocked() && !R.isLocked();
}

} // namespace

// This test loads a file that contains variants for each permutation of the locked status.
// A variant is selected, and then we check to see if the locked status changes have been
// updated on the maya transform nodes.
TEST(LockPrims, lockMetaData)
{
    MString filePath = MString(AL_USDMAYA_TEST_DATA) + "/lock_prim_variants.usda";

    MFileIO::newFile(true);
    MFnDagNode fn;
    MObject    xform = fn.create("transform");
    MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);
    fn.setName("proxy");

    AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();
    proxy->filePathPlug().setString(filePath);

    auto stage = proxy->usdStage();
    ASSERT_TRUE(stage);

    auto hello = stage->GetPrimAtPath(SdfPath("/hello"));
    auto world = stage->GetPrimAtPath(SdfPath("/hello/world"));
    auto cam = stage->GetPrimAtPath(SdfPath("/hello/world/cam"));
    ASSERT_TRUE(hello);
    ASSERT_TRUE(world);
    ASSERT_TRUE(cam);

    EXPECT_TRUE(checkUnlocked("hello"));
    EXPECT_TRUE(checkLocked("world"));
    EXPECT_TRUE(checkLocked("cam"));

    UsdVariantSets sets = hello.GetVariantSets();
    EXPECT_TRUE(sets.SetSelection("lockedVariant", "unlocked"));

    EXPECT_TRUE(checkUnlocked("hello"));
    EXPECT_TRUE(checkUnlocked("world"));
    EXPECT_TRUE(checkUnlocked("cam"));

    EXPECT_TRUE(sets.SetSelection("lockedVariant", "locked"));

    EXPECT_TRUE(checkUnlocked("hello"));
    EXPECT_TRUE(checkLocked("world"));
    EXPECT_TRUE(checkLocked("cam"));

    EXPECT_TRUE(sets.SetSelection("lockedVariant", "unlocked_cam"));

    EXPECT_TRUE(checkUnlocked("hello"));
    EXPECT_TRUE(checkLocked("world"));
    EXPECT_TRUE(checkUnlocked("cam"));

    EXPECT_TRUE(sets.SetSelection("lockedVariant", "inherit"));

    EXPECT_TRUE(checkUnlocked("hello"));
    EXPECT_TRUE(checkLocked("world"));
    EXPECT_TRUE(checkLocked("cam"));

    EXPECT_TRUE(sets.SetSelection("lockedVariant", "inherit_unlocked"));

    EXPECT_TRUE(checkUnlocked("hello"));
    EXPECT_TRUE(checkUnlocked("world"));
    EXPECT_TRUE(checkUnlocked("cam"));
}

// This test loads a file that contains variants for each permutation of the locked status.
// A variant is selected, and then we check to see if the locked status changes have been
// updated on the maya transform nodes.
TEST(LockPrims, lockMetaDataTranslatePrim)
{
    MString filePath = MString(AL_USDMAYA_TEST_DATA) + "/lock_prim_variants2.usda";

    MFileIO::newFile(true);
    MFnDagNode fn;
    MObject    xform = fn.create("transform");
    MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);
    fn.setName("proxy");

    AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();
    proxy->filePathPlug().setString(filePath);

    auto stage = proxy->usdStage();
    ASSERT_TRUE(stage);

    auto hello = stage->GetPrimAtPath(SdfPath("/hello"));
    auto world = stage->GetPrimAtPath(SdfPath("/hello/world"));
    auto cam = stage->GetPrimAtPath(SdfPath("/hello/world/cam"));
    ASSERT_TRUE(hello);
    ASSERT_TRUE(world);
    ASSERT_TRUE(cam);

    ASSERT_TRUE(MGlobal::executeCommand(
        "AL_usdmaya_TranslatePrim -fi -ip \"/hello/world/cam\" -p \"proxy\""));

    EXPECT_TRUE(checkUnlocked("hello"));
    EXPECT_TRUE(checkLocked("world"));
    EXPECT_TRUE(checkLocked("cam"));

    ASSERT_TRUE(
        MGlobal::executeCommand("AL_usdmaya_TranslatePrim -tp \"/hello/world/cam\" -p \"proxy\""));

    UsdVariantSets sets = hello.GetVariantSets();
    EXPECT_TRUE(sets.SetSelection("lockedVariant", "unlocked"));

    ASSERT_TRUE(MGlobal::executeCommand(
        "AL_usdmaya_TranslatePrim -fi -ip \"/hello/world/cam\" -p \"proxy\""));

    EXPECT_TRUE(checkUnlocked("hello"));
    EXPECT_TRUE(checkUnlocked("world"));
    EXPECT_TRUE(checkUnlocked("cam"));

    ASSERT_TRUE(
        MGlobal::executeCommand("AL_usdmaya_TranslatePrim -tp \"/hello/world/cam\" -p \"proxy\""));

    EXPECT_TRUE(sets.SetSelection("lockedVariant", "locked"));

    ASSERT_TRUE(MGlobal::executeCommand(
        "AL_usdmaya_TranslatePrim -fi -ip \"/hello/world/cam\" -p \"proxy\""));

    EXPECT_TRUE(checkUnlocked("hello"));
    EXPECT_TRUE(checkLocked("world"));
    EXPECT_TRUE(checkLocked("cam"));

    ASSERT_TRUE(
        MGlobal::executeCommand("AL_usdmaya_TranslatePrim -tp \"/hello/world/cam\" -p \"proxy\""));

    EXPECT_TRUE(sets.SetSelection("lockedVariant", "unlocked_cam"));

    ASSERT_TRUE(MGlobal::executeCommand(
        "AL_usdmaya_TranslatePrim -fi -ip \"/hello/world/cam\" -p \"proxy\""));

    EXPECT_TRUE(checkUnlocked("hello"));
    EXPECT_TRUE(checkLocked("world"));
    EXPECT_TRUE(checkUnlocked("cam"));

    ASSERT_TRUE(
        MGlobal::executeCommand("AL_usdmaya_TranslatePrim -tp \"/hello/world/cam\" -p \"proxy\""));

    EXPECT_TRUE(sets.SetSelection("lockedVariant", "inherit"));

    ASSERT_TRUE(MGlobal::executeCommand(
        "AL_usdmaya_TranslatePrim -fi -ip \"/hello/world/cam\" -p \"proxy\""));

    EXPECT_TRUE(checkUnlocked("hello"));
    EXPECT_TRUE(checkLocked("world"));
    EXPECT_TRUE(checkLocked("cam"));

    ASSERT_TRUE(
        MGlobal::executeCommand("AL_usdmaya_TranslatePrim -tp \"/hello/world/cam\" -p \"proxy\""));

    EXPECT_TRUE(sets.SetSelection("lockedVariant", "inherit_unlocked"));

    ASSERT_TRUE(MGlobal::executeCommand(
        "AL_usdmaya_TranslatePrim -fi -ip \"/hello/world/cam\" -p \"proxy\""));

    EXPECT_TRUE(checkUnlocked("hello"));
    EXPECT_TRUE(checkUnlocked("world"));
    EXPECT_TRUE(checkUnlocked("cam"));
}

// This test loads a usd file that contains variants for all permutations of the selectable meta
// data. Each variant is selected in turn, and hopefully the selectability DB within the proxy shape
// has been correctly updated.
TEST(Selectability, selectableMetaData)
{
    MString filePath = MString(AL_USDMAYA_TEST_DATA) + "/selectable_prim_variants.usda";

    MFileIO::newFile(true);
    MFnDagNode fn;
    MObject    xform = fn.create("transform");
    MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);
    fn.setName("proxy");

    AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();
    proxy->filePathPlug().setString(filePath);

    auto stage = proxy->usdStage();
    ASSERT_TRUE(stage);

    auto hello = stage->GetPrimAtPath(SdfPath("/hello"));
    auto world = stage->GetPrimAtPath(SdfPath("/hello/world"));
    auto cam = stage->GetPrimAtPath(SdfPath("/hello/world/cam"));
    ASSERT_TRUE(hello);
    ASSERT_TRUE(world);
    ASSERT_TRUE(cam);

    EXPECT_FALSE(proxy->isPathUnselectable(SdfPath("/hello")));
    EXPECT_TRUE(proxy->isPathUnselectable(SdfPath("/hello/world")));
    EXPECT_TRUE(proxy->isPathUnselectable(SdfPath("/hello/world/cam")));

    UsdVariantSets sets = hello.GetVariantSets();
    EXPECT_TRUE(sets.SetSelection("slVariant", "selectable"));

    EXPECT_FALSE(proxy->isPathUnselectable(SdfPath("/hello")));
    EXPECT_FALSE(proxy->isPathUnselectable(SdfPath("/hello/world")));
    EXPECT_FALSE(proxy->isPathUnselectable(SdfPath("/hello/world/cam")));

    EXPECT_TRUE(sets.SetSelection("slVariant", "unselectable"));

    EXPECT_FALSE(proxy->isPathUnselectable(SdfPath("/hello")));
    EXPECT_TRUE(proxy->isPathUnselectable(SdfPath("/hello/world")));
    EXPECT_TRUE(proxy->isPathUnselectable(SdfPath("/hello/world/cam")));

    EXPECT_TRUE(sets.SetSelection("slVariant", "selectable_cam"));

    EXPECT_FALSE(proxy->isPathUnselectable(SdfPath("/hello")));
    EXPECT_TRUE(proxy->isPathUnselectable(SdfPath("/hello/world")));
    EXPECT_FALSE(proxy->isPathUnselectable(SdfPath("/hello/world/cam")));
}

// This test loads a usda file containing variants for the permutations of the excludedGeom tag.
// Hopefully the proxy shape has been updated to reflect the changes after a variant switch
TEST(ExcludePrims, onVariantSwitch)
{
    MString filePath = MString(AL_USDMAYA_TEST_DATA) + "/exclude_prim_variants.usda";

    MFileIO::newFile(true);
    MFnDagNode fn;
    MObject    xform = fn.create("transform");
    MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);
    fn.setName("proxy");

    AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();
    proxy->filePathPlug().setString(filePath);

    auto stage = proxy->usdStage();
    ASSERT_TRUE(stage);

    auto hello = stage->GetPrimAtPath(SdfPath("/hello"));
    auto world = stage->GetPrimAtPath(SdfPath("/hello/world"));
    auto cam = stage->GetPrimAtPath(SdfPath("/hello/world/cam"));
    ASSERT_TRUE(hello);
    ASSERT_TRUE(world);
    ASSERT_TRUE(cam);

    EXPECT_FALSE(proxy->primHasExcludedParent(stage->GetPrimAtPath(SdfPath("/hello"))));
    EXPECT_FALSE(proxy->primHasExcludedParent(stage->GetPrimAtPath(SdfPath("/hello/world"))));
    EXPECT_FALSE(proxy->primHasExcludedParent(stage->GetPrimAtPath(SdfPath("/hello/world/cam"))));

    UsdVariantSets sets = hello.GetVariantSets();
    EXPECT_TRUE(sets.SetSelection("excludeVariant", "exclude"));

    EXPECT_FALSE(proxy->primHasExcludedParent(stage->GetPrimAtPath(SdfPath("/hello"))));
    EXPECT_TRUE(proxy->primHasExcludedParent(stage->GetPrimAtPath(SdfPath("/hello/world"))));
    EXPECT_TRUE(proxy->primHasExcludedParent(stage->GetPrimAtPath(SdfPath("/hello/world/cam"))));

    EXPECT_TRUE(sets.SetSelection("excludeVariant", "include"));

    EXPECT_FALSE(proxy->primHasExcludedParent(stage->GetPrimAtPath(SdfPath("/hello"))));
    EXPECT_FALSE(proxy->primHasExcludedParent(stage->GetPrimAtPath(SdfPath("/hello/world"))));
    EXPECT_FALSE(proxy->primHasExcludedParent(stage->GetPrimAtPath(SdfPath("/hello/world/cam"))));
}

// This test loads a file that contains variants for each permutation of the locked status.
// A variant is selected, and then we check to see if the locked status changes have been
// updated on the maya transform nodes.
TEST(LockPrims, onObjectsChanged)
{
    MString filePath = MString(AL_USDMAYA_TEST_DATA) + "/lock_prim_variants3.usda";

    MFileIO::newFile(true);
    MFnDagNode fn;
    MObject    xform = fn.create("transform");
    MObject    shape = fn.create("AL_usdmaya_ProxyShape", xform);
    fn.setName("proxy");

    AL::usdmaya::nodes::ProxyShape* proxy = (AL::usdmaya::nodes::ProxyShape*)fn.userNode();
    proxy->filePathPlug().setString(filePath);

    auto stage = proxy->usdStage();
    ASSERT_TRUE(stage);

    auto hello = stage->GetPrimAtPath(SdfPath("/hello"));
    auto world = stage->GetPrimAtPath(SdfPath("/hello/world"));
    auto cam = stage->GetPrimAtPath(SdfPath("/hello/world/cam"));
    ASSERT_TRUE(hello);
    ASSERT_TRUE(world);
    ASSERT_TRUE(cam);

    EXPECT_TRUE(checkUnlocked("hello"));
    EXPECT_TRUE(checkUnlocked("world"));
    EXPECT_TRUE(checkUnlocked("cam"));

    hello.SetMetadata<TfToken>(AL::usdmaya::Metadata::locked, AL::usdmaya::Metadata::lockTransform);
    world.SetMetadata<TfToken>(AL::usdmaya::Metadata::locked, AL::usdmaya::Metadata::lockUnlocked);

    EXPECT_TRUE(checkLocked("hello"));
    EXPECT_TRUE(checkUnlocked("world"));
    EXPECT_TRUE(checkUnlocked("cam"));

    hello.SetMetadata<TfToken>(AL::usdmaya::Metadata::locked, AL::usdmaya::Metadata::lockUnlocked);
    world.SetMetadata<TfToken>(AL::usdmaya::Metadata::locked, AL::usdmaya::Metadata::lockTransform);

    EXPECT_TRUE(checkUnlocked("hello"));
    EXPECT_TRUE(checkLocked("world"));
    EXPECT_TRUE(checkLocked("cam"));

    world.SetMetadata<TfToken>(AL::usdmaya::Metadata::locked, AL::usdmaya::Metadata::lockUnlocked);

    EXPECT_TRUE(checkUnlocked("hello"));
    EXPECT_TRUE(checkUnlocked("world"));
    EXPECT_TRUE(checkUnlocked("cam"));
}
