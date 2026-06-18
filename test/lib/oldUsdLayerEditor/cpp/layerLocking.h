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
// Compatibility shim: re-exports old-editor locking API into UsdLayerEditor namespace
// so shared test *Logic.h headers compile unchanged against the old editor.
#pragma once
#include <mayaUsd/utils/layerLocking.h>

namespace UsdLayerEditor {
using MayaUsd::LayerLockType;
using MayaUsd::LayerLock_Unlocked;
using MayaUsd::LayerLock_Locked;
using MayaUsd::LayerLock_SystemLocked;
using MayaUsd::lockLayer;
using MayaUsd::isLayerLocked;
using MayaUsd::isLayerSystemLocked;
using MayaUsd::addLockedLayer;
using MayaUsd::removeLockedLayer;
using MayaUsd::forgetLockedLayers;
using MayaUsd::addSystemLockedLayer;
using MayaUsd::removeSystemLockedLayer;
using MayaUsd::forgetSystemLockedLayers;
} // namespace UsdLayerEditor
