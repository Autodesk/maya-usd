//
// Copyright 2026 Autodesk
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
#include <usdUfe/ufe/Global.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/UsdUndoDuplicateCommand.h>
#include <usdUfe/utils/layers.h>

#include <pxr/base/tf/notice.h>
#include <pxr/base/tf/weakBase.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/usd/editContext.h>
#include <pxr/usd/usd/notice.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>

#include <ufe/path.h>
#include <ufe/pathSegment.h>

#include <gtest/gtest.h>

#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

// Records the type name of a prim each time the stage reports a change and the
// prim exists. This is what observers (UFE, the Maya outliner) get to see.
class TypeRecorder : public TfWeakBase
{
public:
    TypeRecorder(const UsdStageRefPtr& stage, const SdfPath& path)
        : _stage(stage)
        , _path(path)
    {
        TfWeakPtr<TypeRecorder> me(this);
        _key = TfNotice::Register(me, &TypeRecorder::onChanged, UsdStageWeakPtr(stage));
    }

    ~TypeRecorder() { TfNotice::Revoke(_key); }

    const std::vector<std::string>& types() const { return _types; }

private:
    void onChanged(const UsdNotice::ObjectsChanged&)
    {
        UsdPrim prim = _stage->GetPrimAtPath(_path);
        if (prim)
            _types.push_back(prim.GetTypeName().GetString());
    }

    UsdStageRefPtr           _stage;
    SdfPath                  _path;
    TfNotice::Key            _key;
    std::vector<std::string> _types;
};

UsdUfe::UsdSceneItem::Ptr makeItem(const UsdPrim& prim)
{
    const Ufe::Path path(
        Ufe::PathSegment(prim.GetPath().GetString(), UsdUfe::getUsdRunTimeId(), '/'));
    return UsdUfe::UsdSceneItem::create(path, prim);
}

// Duplicate the prim at the root of the stage, back at the root of the same stage.
UsdUfe::UsdUndoDuplicateCommand::Ptr
makeDuplicateCommand(const UsdStageRefPtr& stage, const char* primPath)
{
    return UsdUfe::UsdUndoDuplicateCommand::create(
        makeItem(stage->GetPrimAtPath(SdfPath(primPath))), makeItem(stage->GetPseudoRoot()));
}

} // namespace

// A prim defined in the session layer must have its type as soon as it is created,
// otherwise the UFE add notification is sent for a prim without a type.
TEST(UsdUndoDuplicateCommand, sessionLayerPrimHasTypeWhenCreated)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    {
        UsdEditContext ctx(stage, stage->GetSessionLayer());
        stage->DefinePrim(SdfPath("/A"), TfToken("Xform"));
    }
    stage->SetEditTarget(stage->GetSessionLayer());

    TypeRecorder recorder(stage, SdfPath("/A1"));

    auto cmd = makeDuplicateCommand(stage, "/A");
    ASSERT_TRUE(cmd);
    cmd->execute();

    UsdPrim dup = stage->GetPrimAtPath(SdfPath("/A1"));
    ASSERT_TRUE(dup);
    EXPECT_EQ(dup.GetTypeName(), TfToken("Xform"));

    ASSERT_FALSE(recorder.types().empty());
    for (const std::string& type : recorder.types())
        EXPECT_EQ(type, "Xform");
}

// A class prim has no "def" spec. It must still be duplicated, with its children.
TEST(UsdUndoDuplicateCommand, classPrim)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    stage->CreateClassPrim(SdfPath("/MyClass"));
    stage->DefinePrim(SdfPath("/MyClass/Child"), TfToken("Xform"));

    auto cmd = makeDuplicateCommand(stage, "/MyClass");
    ASSERT_TRUE(cmd);
    cmd->execute();

    UsdPrim dup = stage->GetPrimAtPath(SdfPath("/MyClass1"));
    ASSERT_TRUE(dup);
    EXPECT_EQ(dup.GetSpecifier(), SdfSpecifierClass);

    UsdPrim child = stage->GetPrimAtPath(SdfPath("/MyClass1/Child"));
    ASSERT_TRUE(child);
    EXPECT_EQ(child.GetTypeName(), TfToken("Xform"));

    // Note: undo and redo are not tested here, since they need the undo state delegate
    //       that the host application installs on its stages. They are covered by the
    //       Python test testDuplicateClassPrim.
}

// getDefiningPrimSpec must prefer a "def" over a "class" and must find a class spec.
TEST(UsdUfeLayers, definingPrimSpecOfClassPrim)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdPrim        classPrim = stage->CreateClassPrim(SdfPath("/MyClass"));
    UsdPrim        defPrim = stage->DefinePrim(SdfPath("/Def"), TfToken("Xform"));

    SdfPrimSpecHandle classSpec = UsdUfe::getDefiningPrimSpec(classPrim);
    ASSERT_TRUE(classSpec);
    EXPECT_EQ(classSpec->GetSpecifier(), SdfSpecifierClass);
    EXPECT_FALSE(UsdUfe::getDefiningPrimStack(classPrim).empty());

    SdfPrimSpecHandle defSpec = UsdUfe::getDefiningPrimSpec(defPrim);
    ASSERT_TRUE(defSpec);
    EXPECT_EQ(defSpec->GetSpecifier(), SdfSpecifierDef);
}
