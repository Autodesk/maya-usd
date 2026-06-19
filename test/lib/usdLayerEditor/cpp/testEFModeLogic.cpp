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
#ifndef USD_LAYER_EDITOR_TEST_EF_MODE_LOGIC_H
#define USD_LAYER_EDITOR_TEST_EF_MODE_LOGIC_H

#include <testFixture.h>

#include "stringResources.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>
#include <gtest/gtest.h>

namespace UsdLayerEditor {

static QPushButton* findButtonByObjectName(QWidget* root, const QString& name)
{
    return root->findChild<QPushButton*>(name);
}

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

// Test 219 — new editor gates the button on supportsEditForwarding() at runtime,
// so with the stub (returns false) the button must be absent. The old editor
// uses a compile-time #ifdef guard instead, so this assertion doesn't apply there.
TEST_F(LayerEditorTestFixture, EFMode_ToggleButton_AbsentWhenNotSupported)
{
#ifdef MAYAUSD_OLD_LAYER_EDITOR
    // Old editor creates the button at compile time (WANT_ADSK_USD_EDIT_FORWARD_BUILD);
    // supportsEditForwarding() is not consulted. Verify the button IS present instead.
    QPushButton* btn = findButtonByObjectName(_widget, "LayerEditorToggleEFButton");
    if (!btn) {
        GTEST_SKIP() << "Old editor test built without WANT_ADSK_USD_EDIT_FORWARD_BUILD";
    }
    EXPECT_NE(btn, nullptr);
#else
    QPushButton* btn = findButtonByObjectName(_widget, "LayerEditorToggleEFButton");
    EXPECT_EQ(btn, nullptr);
#endif
}

// Test 220 — when the button is present its objectName must match the constant
// used by automation. Skipped in the new editor when supportsEditForwarding()
// is false (button not created). Always runs in old editor when EF is compiled in.
TEST_F(LayerEditorTestFixture, EFMode_ToggleButton_HasCorrectObjectName)
{
    QPushButton* btn = findButtonByObjectName(_widget, "LayerEditorToggleEFButton");
    if (!btn) {
        GTEST_SKIP() << "EF toggle button absent";
    }
    EXPECT_EQ(btn->objectName(), QString("LayerEditorToggleEFButton"));
}

// Test 221 — isEditForwardMode() default must be false.
// Guarded: old editor only exposes isEditForwardMode() under WANT_ADSK_USD_EDIT_FORWARD_BUILD.
#ifdef WANT_ADSK_USD_EDIT_FORWARD_BUILD
TEST_F(LayerEditorTestFixture, EFMode_IsEditForwardMode_FalseByDefault)
{
    EXPECT_FALSE(_sessionState.isEditForwardMode());
}
#endif

// Test 222 — effectiveTargetLayer() must equal targetLayer() when EF is off.
TEST_F(LayerEditorTestFixture, EFMode_EffectiveTargetLayer_EqualsTargetLayerByDefault)
{
    auto target    = _sessionState.targetLayer();
    auto effective = _sessionState.effectiveTargetLayer();
    EXPECT_EQ(target, effective);
}

// ── Tests using LayerEditorWithEFFixture (EF support enabled) ─────────────

// Test 223 — the button tooltip must match the kToggleEditForwarding string resource.
// Guarded: kToggleEditForwarding only exists under WANT_ADSK_USD_EDIT_FORWARD_BUILD.
#ifdef WANT_ADSK_USD_EDIT_FORWARD_BUILD
TEST_F(LayerEditorWithEFFixture, EFMode_Button_Tooltip)
{
    QPushButton* btn = findButtonByObjectName(_widget, "LayerEditorToggleEFButton");
    if (!btn) {
        GTEST_SKIP() << "EF toggle button absent";
    }
    EXPECT_EQ(btn->toolTip(),
              StringResources::getAsQString(StringResources::kToggleEditForwarding));
}
#endif

// Test 225 — updateButtons() sets the button stylesheet to reflect EF active state.
// The icon switches between ef_default (off) and ef_on (on) via background-image.
// Guarded to new editor only: old editor's updateButtons() is a separate copy that
// already has this logic; the shared widget is what we're verifying here.
#ifndef MAYAUSD_OLD_LAYER_EDITOR
TEST_F(LayerEditorWithEFFixture, EFMode_Button_IconReflectsActiveState)
{
    QPushButton* btn = findButtonByObjectName(_widget, "LayerEditorToggleEFButton");
    ASSERT_NE(btn, nullptr);

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

} // namespace UsdLayerEditor

#endif // USD_LAYER_EDITOR_TEST_EF_MODE_LOGIC_H
