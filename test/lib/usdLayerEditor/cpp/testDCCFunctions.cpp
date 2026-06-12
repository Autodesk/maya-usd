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
#include "scopedLayerEditorDCCFunctions.h"

#include <layerEditorDCCFunctions.h>

#include <pxr/usd/sdf/layer.h>

#include <gtest/gtest.h>

#include <QtWidgets/QWidget>

#include <cstdint>

using namespace UsdLayerEditor;

// mainWindowParent() returns null when unset, the registered widget when set.
TEST(LayerEditorDCCFunctions, MainWindowParent_DefaultsToNull)
{
    ScopedLayerEditorDCCFunctions guard;
    setEnvironmentFns(EnvironmentFns {});
    EXPECT_EQ(mainWindowParent(), nullptr);
}

TEST(LayerEditorDCCFunctions, MainWindowParent_ReturnsRegisteredWidget)
{
    ScopedLayerEditorDCCFunctions guard;
    QWidget        w;
    EnvironmentFns env;
    env.mainWindowParent = [&w]() { return &w; };
    setEnvironmentFns(env);
    EXPECT_EQ(mainWindowParent(), &w);
}

// layer-contents size limits default to 8, return the registered values when set.
TEST(LayerEditorDCCFunctions, LayerContentsLimits_DefaultToEight)
{
    ScopedLayerEditorDCCFunctions guard;
    setEnvironmentFns(EnvironmentFns {});
    EXPECT_EQ(layerContentsArraySizeLimit(), 8);
    EXPECT_EQ(layerContentsTimeSamplesSizeLimit(), 8);
}

TEST(LayerEditorDCCFunctions, LayerContentsLimits_ReturnRegisteredValues)
{
    ScopedLayerEditorDCCFunctions guard;
    EnvironmentFns                env;
    env.layerContentsArraySizeLimit = []() -> int64_t { return 3; };
    env.layerContentsTimeSamplesSizeLimit = []() -> int64_t { return 5; };
    setEnvironmentFns(env);
    EXPECT_EQ(layerContentsArraySizeLimit(), 3);
    EXPECT_EQ(layerContentsTimeSamplesSizeLimit(), 5);
}

TEST(LayerEditorDCCFunctions, CaptureSessionLayer_NullByDefault)
{
    ScopedLayerEditorDCCFunctions guard;
    setComponentFns(ComponentFns {});
    EXPECT_FALSE(captureSessionLayer("|x"));
}

TEST(LayerEditorDCCFunctions, TransferSessionLayer_NoOpByDefault)
{
    ScopedLayerEditorDCCFunctions guard;
    setComponentFns(ComponentFns {});
    transferSessionLayer(PXR_NS::SdfLayerRefPtr {}, "|x"); // must not crash
    SUCCEED();
}

TEST(LayerEditorDCCFunctions, TransferSessionLayer_DispatchesWhenRegistered)
{
    ScopedLayerEditorDCCFunctions guard;
    PXR_NS::SdfLayerRefPtr seenSrc;
    std::string            seenDst;
    ComponentFns           fns;
    fns.transferSessionLayer
        = [&](const PXR_NS::SdfLayerRefPtr& src, const std::string& dst) { seenSrc = src; seenDst = dst; };
    setComponentFns(fns);
    auto layer = PXR_NS::SdfLayer::CreateAnonymous("xfer");
    transferSessionLayer(layer, "|newProxy");
    EXPECT_EQ(seenSrc, layer);
    EXPECT_EQ(seenDst, "|newProxy");
}

TEST(LayerEditorDCCFunctions, SetProxyRootLayerPath_DispatchesWhenRegistered)
{
    ScopedLayerEditorDCCFunctions guard;
    std::string seenObj, seenPath;
    ComponentFns fns;
    fns.setProxyRootLayerPath
        = [&](const std::string& obj, const std::string& path, const PXR_NS::SdfLayerRefPtr&) {
              seenObj = obj;
              seenPath = path;
          };
    setComponentFns(fns);
    setProxyRootLayerPath("|proxy", "/tmp/new.usd", nullptr);
    EXPECT_EQ(seenObj, "|proxy");
    EXPECT_EQ(seenPath, "/tmp/new.usd");
}
