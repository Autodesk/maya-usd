#include <usdUfe/ufe/UsdLabeledEditUndoableCommand.h>
#include <usdUfe/undo/UsdUndoStateDelegate.h>
#include <usdUfe/undo/UsdUndoUtils.h>

#include <pxr/base/gf/vec2i.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdRender/settings.h>

#include <gtest/gtest.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

UsdStageRefPtr makeStageWithRenderSettings()
{
    auto stage = UsdStage::CreateInMemory();
    UsdRenderSettings::Define(stage, SdfPath("/Render/Settings"));
    return stage;
}

GfVec2i getResolution(const UsdRenderSettings& settings)
{
    GfVec2i resolution;
    EXPECT_TRUE(settings.GetResolutionAttr().Get(&resolution));
    return resolution;
}

SdfPathVector getCameraTargets(const UsdRenderSettings& settings)
{
    SdfPathVector targets;
    settings.GetCameraRel().GetTargets(&targets);
    return targets;
}

} // namespace

TEST(UsdUndoUtils, trackStagesEditTargetsInstallsDelegate)
{
    auto stage = makeStageWithRenderSettings();
    UsdUfe::trackStagesEditTargets({ stage });

    const auto delegate = TfDynamic_cast<UsdUfe::UsdUndoStateDelegateRefPtr>(
        stage->GetEditTarget().GetLayer()->GetStateDelegate());
    ASSERT_TRUE(delegate);
}

TEST(UsdUndoUtils, trackStagesEditTargetsIsIdempotent)
{
    auto stage = makeStageWithRenderSettings();
    UsdUfe::trackStagesEditTargets({ stage });

    const auto delegateBefore = TfDynamic_cast<UsdUfe::UsdUndoStateDelegateRefPtr>(
        stage->GetEditTarget().GetLayer()->GetStateDelegate());
    ASSERT_TRUE(delegateBefore);

    UsdUfe::trackStagesEditTargets({ stage });

    const auto delegateAfter = TfDynamic_cast<UsdUfe::UsdUndoStateDelegateRefPtr>(
        stage->GetEditTarget().GetLayer()->GetStateDelegate());
    EXPECT_EQ(delegateBefore, delegateAfter);
}

TEST(UsdLabeledEditUndoableCommand, attributeEditRoundtrip)
{
    auto stage = makeStageWithRenderSettings();
    UsdUfe::trackStagesEditTargets({ stage });

    auto settings = UsdRenderSettings(stage->GetPrimAtPath(SdfPath("/Render/Settings")));
    ASSERT_TRUE(settings);

    auto cmd
        = std::make_shared<UsdUfe::UsdLabeledEditUndoableCommand>("Edit resolution", [&settings]() {
              settings.GetResolutionAttr().Set(GfVec2i(1920, 1080));
          });
    cmd->execute();

    EXPECT_EQ(getResolution(settings), GfVec2i(1920, 1080));

    cmd->undo();
    EXPECT_FALSE(settings.GetResolutionAttr().HasAuthoredValue());

    cmd->redo();
    EXPECT_EQ(getResolution(settings), GfVec2i(1920, 1080));
}

TEST(UsdLabeledEditUndoableCommand, relationshipEditRoundtrip)
{
    auto stage = makeStageWithRenderSettings();
    UsdUfe::trackStagesEditTargets({ stage });

    auto settings = UsdRenderSettings(stage->GetPrimAtPath(SdfPath("/Render/Settings")));
    ASSERT_TRUE(settings);

    const SdfPath cameraPath("/Camera");
    stage->DefinePrim(cameraPath, TfToken("Camera"));

    auto cmd = std::make_shared<UsdUfe::UsdLabeledEditUndoableCommand>(
        "Edit camera",
        [&settings, cameraPath]() { settings.GetCameraRel().SetTargets({ cameraPath }); });
    cmd->execute();

    EXPECT_EQ(getCameraTargets(settings), SdfPathVector({ cameraPath }));

    cmd->undo();
    EXPECT_FALSE(settings.GetCameraRel().HasAuthoredTargets());

    cmd->redo();
    EXPECT_EQ(getCameraTargets(settings), SdfPathVector({ cameraPath }));
}
