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

#include "testUtils.h"
#include "LayerEditorCommands.h"
#include "layerLocking.h"
#include "layerMuting.h"

#include <usdUfe/ufe/Utils.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

#include <ufe/globalSelection.h>
#include <ufe/observableSelection.h>
#include <ufe/path.h>

#include <gtest/gtest.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace UsdLayerEditor {

namespace {

// Stub stage-path accessor: returns an empty UFE path for any stage.
// This is sufficient for tests that don't inspect the path value.
Ufe::Path stubStagePathAccessor(PXR_NS::UsdStageWeakPtr /*stage*/)
{
    return Ufe::Path();
}

} // namespace

TEST(LayerEditorCommandsSmokeTest, HeaderIncludesCompile)
{
    SUCCEED();
}

class UpdateEditTargetTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Register a no-op stage-path accessor so that MuteLayerCmd::saveSelection()
        // can call UsdUfe::stagePath() without crashing in this test context.
        UsdUfe::setStagePathAccessorFn(stubStagePathAccessor);

        // Initialize the UFE global selection singleton if not already done.
        // MuteLayerCmd::saveSelection() calls Ufe::GlobalSelection::get().
        if (!Ufe::GlobalSelection::get()) {
            Ufe::GlobalSelection::initializeInstance(
                std::make_shared<Ufe::ObservableSelection>());
        }

        forgetLockedLayers();
        _stage    = PXR_NS::UsdStage::CreateInMemory();
        _subLayer = PXR_NS::SdfLayer::CreateAnonymous("sub");
        _stage->GetRootLayer()->InsertSubLayerPath(_subLayer->GetIdentifier(), 0);
    }

    void TearDown() override
    {
        BaseCmd::setAutoRetargetDisabledChecker(nullptr);
        forgetLockedLayers();
    }

    PXR_NS::UsdStageRefPtr _stage;
    PXR_NS::SdfLayerRefPtr _subLayer;
};

// When all layers are non-modifiable, updateEditTarget should switch to session layer.
TEST_F(UpdateEditTargetTest, WhenNoModifiableLayers_EditTargetChangesToSessionLayer)
{
    // Lock all non-session layers so nothing is modifiable.
    lockLayer("", _stage->GetRootLayer(), LayerLock_Locked, /*updateDCCAttr=*/false);
    lockLayer("", _subLayer,              LayerLock_Locked, /*updateDCCAttr=*/false);
    _stage->SetEditTarget(_stage->GetRootLayer());

    // Mute the sublayer — this calls updateEditTarget() internally.
    auto cmd = std::make_shared<MuteLayerCmd>(_stage, _subLayer, /*muteIt=*/true);
    cmd->execute();

    EXPECT_EQ(_stage->GetEditTarget().GetLayer(), _stage->GetSessionLayer());
}

// When the checker returns true, updateEditTarget should be suppressed entirely.
TEST_F(UpdateEditTargetTest, WhenCheckerDisablesAutoRetarget_EditTargetUnchanged)
{
    BaseCmd::setAutoRetargetDisabledChecker([] { return true; });

    lockLayer("", _stage->GetRootLayer(), LayerLock_Locked, false);
    lockLayer("", _subLayer,              LayerLock_Locked, false);
    _stage->SetEditTarget(_stage->GetRootLayer());

    auto cmd = std::make_shared<MuteLayerCmd>(_stage, _subLayer, /*muteIt=*/true);
    cmd->execute();

    // Must NOT have changed to session layer.
    EXPECT_NE(_stage->GetEditTarget().GetLayer(), _stage->GetSessionLayer());
}

} // namespace UsdLayerEditor
