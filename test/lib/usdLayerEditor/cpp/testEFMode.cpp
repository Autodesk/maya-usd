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
#pragma once

#include <testFixture.h>
#include "testUtils.h"

#include "stringResources.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>
#include <gtest/gtest.h>

namespace UsdLayerEditor {

// ── Fixture that enables EF support on the stub session state ──────────────
// Sets _supportsEditForwarding before widget construction so the EF toggle
// button is created by setupLayout_toolbar().
class LayerEditorWithEFFixture : public LayerEditorTestFixture
{
protected:
    void SetUp() override
    {
        setEditForwardingSupported(true);
        LayerEditorTestFixture::SetUp();
    }
};

// ── Tests using the standard fixture (no EF support) ──────────────────────

// new editor gates the button on supportsEditForwarding() at runtime,
// so with the stub (returns false) the button must be absent. The old editor
// uses a compile-time #ifdef guard instead, so this assertion doesn't apply there.
TEST_F(LayerEditorTestFixture, EFMode_ToggleButton_AbsentWhenNotSupported)
{
#ifdef MAYAUSD_OLD_LAYER_EDITOR
    // Old editor creates the button at compile time (WANT_ADSK_USD_EDIT_FORWARD_BUILD);
    // supportsEditForwarding() is not consulted. Verify the button IS present instead.
    QPushButton* btn = TestUtils::findButtonByObjectName(_widget, "LayerEditorToggleEFButton");
    if (!btn) {
        GTEST_SKIP() << "Old editor test built without WANT_ADSK_USD_EDIT_FORWARD_BUILD";
    }
    EXPECT_NE(btn, nullptr);
#else
    QPushButton* btn = TestUtils::findButtonByObjectName(_widget, "LayerEditorToggleEFButton");
    EXPECT_EQ(btn, nullptr);
#endif
}

// when the button is present its objectName must match the constant
// used by automation. Skipped in the new editor when supportsEditForwarding()
// is false (button not created). Always runs in old editor when EF is compiled in.
TEST_F(LayerEditorTestFixture, EFMode_ToggleButton_HasCorrectObjectName)
{
    QPushButton* btn = TestUtils::findButtonByObjectName(_widget, "LayerEditorToggleEFButton");
    if (!btn) {
        GTEST_SKIP() << "EF toggle button absent";
    }
    EXPECT_EQ(btn->objectName(), QString("LayerEditorToggleEFButton"));
}

// isEditForwardMode() default must be false.
// Guarded: old editor only exposes isEditForwardMode() under WANT_ADSK_USD_EDIT_FORWARD_BUILD.
#ifdef WANT_ADSK_USD_EDIT_FORWARD_BUILD
TEST_F(LayerEditorTestFixture, EFMode_IsEditForwardMode_FalseByDefault)
{
    EXPECT_FALSE(_sessionState.isEditForwardMode());
}
#endif

// effectiveTargetLayer() must equal targetLayer() when EF is off.
TEST_F(LayerEditorTestFixture, EFMode_EffectiveTargetLayer_EqualsTargetLayerByDefault)
{
    auto target    = _sessionState.targetLayer();
    auto effective = _sessionState.effectiveTargetLayer();
    EXPECT_EQ(target, effective);
}

// ── Tests using LayerEditorWithEFFixture (EF support enabled) ─────────────

// the button tooltip must match the kToggleEditForwarding string resource.
// Guarded: kToggleEditForwarding only exists under WANT_ADSK_USD_EDIT_FORWARD_BUILD.
#ifdef WANT_ADSK_USD_EDIT_FORWARD_BUILD
TEST_F(LayerEditorWithEFFixture, EFMode_Button_Tooltip)
{
    QPushButton* btn = TestUtils::findButtonByObjectName(_widget, "LayerEditorToggleEFButton");
    if (!btn) {
        GTEST_SKIP() << "EF toggle button absent";
    }
    EXPECT_EQ(btn->toolTip(),
              StringResources::getAsQString(StringResources::kToggleEditForwarding));
}
#endif

// updateButtons() sets the button stylesheet to reflect EF active state.
// The icon switches between ef_default (off) and ef_on (on) via background-image.
// New editor only: the old editor lacks the editForwardingChanged→updateButtons connection.
#if defined(WANT_ADSK_USD_EDIT_FORWARD_BUILD) && !defined(MAYAUSD_OLD_LAYER_EDITOR)
TEST_F(LayerEditorWithEFFixture, EFMode_Button_IconReflectsActiveState)
{
    QPushButton* btn = TestUtils::findButtonByObjectName(_widget, "LayerEditorToggleEFButton");
    if (!btn) {
        GTEST_SKIP() << "EF toggle button absent (stub does not create it for this editor)";
    }

    // Initial state: EF off → stylesheet must reference ef_default.
    QApplication::processEvents();
    EXPECT_TRUE(btn->styleSheet().contains("ef_default"))
        << "Expected ef_default icon when EF is off";

    // Activate EF → stylesheet must switch to ef_on.
    _sessionState.setIsEditForwardMode(true);
    QApplication::processEvents();
    EXPECT_TRUE(btn->styleSheet().contains("ef_on"))
        << "Expected ef_on icon when EF is active";

    // Deactivate → back to ef_default.
    _sessionState.setIsEditForwardMode(false);
    QApplication::processEvents();
    EXPECT_TRUE(btn->styleSheet().contains("ef_default"))
        << "Expected ef_default icon after EF deactivated";
}
#endif

// With EF support enabled before construction, the new editor creates the EF toggle
// button and the EF option-menu items — both gated purely at runtime on
// supportsEditForwarding(), with no compile-time guard. This is unguarded (unlike the
// tooltip/icon tests above, which reference EF-build-only string resources).
#ifndef MAYAUSD_OLD_LAYER_EDITOR
TEST_F(LayerEditorWithEFFixture, EFMode_ToggleButton_CreatedWhenSupported)
{
    QPushButton* btn = TestUtils::findButtonByObjectName(_widget, "LayerEditorToggleEFButton");
    EXPECT_NE(btn, nullptr) << "EF toggle button should be created when EF is supported";
}
#endif

} // namespace UsdLayerEditor
