# Layer Editor DCC-Functions Registry Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move the misplaced DCC-specific hooks (Component Creator, Edit-Forwarding queries, DCC object/stage queries) off `AbstractCommandHook` / `SessionState` onto a dedicated `UsdLayerEditor` function registry injected at Maya plugin init.

**Architecture:** A new `layerEditorDCCFunctions.{h,cpp}` in the shared LE lib holds three grouped `std::function` sub-structs (`ComponentFns`, `EditForwardingFns`, `DccObjectFns`) in a global registry, with thin accessor free functions that own the defaults. Maya populates the registry once at `UsdLayerEditor::initialize()` via lambdas wrapping the existing `ComponentUtils` / edit-forward helpers. Shared-lib call sites read the accessors. The base-class virtuals and Maya overrides are then deleted.

**Tech Stack:** C++17, Qt, USD, CMake, Maya plugin API. Build/test run on the host via the `_host_command` relay (never directly).

**Source spec:** `docs/superpowers/specs/2026-05-29-layer-editor-dcc-functions-registry-design.md`

---

## Ordering rationale (read before starting)

The phases are ordered so **every commit leaves both the full build and all tests green, with no interim production regression**:

1. **Phase 1** creates the registry (dormant — nothing reads or writes it).
2. **Phase 2** populates the registry from Maya at plugin init, while the old base-class virtuals still exist and still drive behavior. Registry is populated but unused → no behavior change.
3. **Phase 3** switches the shared-lib call sites to read the registry. Production now reads the already-populated registry (correct); C++ tests read a fixture-installed registry. The base virtuals/overrides become dead but still compile.
4. **Phase 4** deletes the now-dead virtuals, Maya overrides, and stub overrides.

**Two facts that shape the test work (Phase 3):**

- `isDccObjectSharedStage`'s accessor default is **`true`**, but the C++ test base fixture historically relies on the stub default **`false`**. The new fixture must install a baseline registry so this default flips correctly per test.
- The `*Logic.h` test headers in `lib/usdUfe/usd-layer-editor/test/cpp/` are compiled into **both** the new-editor test target *and* the old-editor parity target (`mayaUsdOldLayerEditorTests`). The old-editor target does **not** link `UsdLayerEditorLib`, so it cannot see the registry symbols. Therefore the shared headers must never reference registry symbols directly — they call per-editor fixture methods (`setEditForwardingSupported` / `setSharedStage` / `setStageIncoming`) that each editor's `LayerEditorTestFixture` implements differently (new = drive the registry; old = set the legacy stub members).

## Relay preflight (run once at the start of each work session)

```bash
cd /d/repos/agent_repos/ecg-maya-usd
python3 _host_command/relay_client.py --help > /dev/null && echo "relay_client ok"
```

Build / test invocations used throughout (run from `/d/repos/agent_repos/ecg-maya-usd`):

```bash
# Build everything
python3 _host_command/relay_client.py run build \
  --db _host_command/relay.db --commands-json _host_command/commands.json

# Run tests, optionally filtered by a test-name regex
python3 _host_command/relay_client.py run test LayerEditor \
  --db _host_command/relay.db --commands-json _host_command/commands.json
```

Always read `stdout`/`stderr` of the result JSON. Retry only on transient failures (`STALL:`, exit 2/3, no output) per `CLAUDE.md` retry policy.

---

## Phase 1 — Registry core (dormant)

### Task 1: Create the registry header

**Files:**
- Create: `lib/usdUfe/usd-layer-editor/lib/layerEditorDCCFunctions.h`

- [ ] **Step 1: Write the header**

Mirror the `LayerEditorAPI` + `std::function` setter style already used by `utilSerialization.h` (`setUpdateDCCObjectRootLayerFunction`). Note: `hasEditForwarding` is intentionally absent (deleted, per spec).

```cpp
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
#ifndef LAYER_EDITOR_DCC_FUNCTIONS_H
#define LAYER_EDITOR_DCC_FUNCTIONS_H

#include "LayerEditorAPI.h"

#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>

#include <functional>
#include <string>
#include <vector>

namespace UsdLayerEditor {

// std::function typedefs use the EXACT signatures of the former base-class overrides.
using SaveComponentFn    = std::function<void(const PXR_NS::UsdStageRefPtr&, const std::string&)>;
using ReloadComponentFn  = std::function<void(const std::string&)>;
using RenameProxyShapeFn = std::function<void(const std::string&, const std::string&)>;
using IsStageAComponentFn  = std::function<bool(const std::string&)>;
using IsUnsavedComponentFn = std::function<bool(const PXR_NS::UsdStageRefPtr&)>;
using ShouldDisplayComponentInitialSaveDialogFn
    = std::function<bool(const PXR_NS::UsdStageRefPtr&, const std::string&)>;
using SceneFolderFn = std::function<std::string()>;
using MoveComponentFn
    = std::function<std::string(const std::string&, const std::string&, const std::string&)>;
using PreviewComponentSaveFn
    = std::function<std::string(const std::string&, const std::string&, const std::string&)>;
using GetComponentLayersToSaveFn = std::function<std::vector<std::string>(const std::string&)>;

using SupportsEditForwardingFn = std::function<bool()>;
using EchoEditForwardingFn     = std::function<bool()>;
using SetEchoEditForwardingFn  = std::function<void(bool)>;

using IsDccObjectStageIncomingFn = std::function<bool(const std::string&)>;
using IsDccObjectSharedStageFn   = std::function<bool(const std::string&)>;

struct ComponentFns
{
    SaveComponentFn                           saveComponent;
    ReloadComponentFn                         reloadComponent;
    RenameProxyShapeFn                        renameProxyShape;
    IsStageAComponentFn                       isStageAComponent;
    IsUnsavedComponentFn                      isUnsavedComponent;
    ShouldDisplayComponentInitialSaveDialogFn shouldDisplayComponentInitialSaveDialog;
    SceneFolderFn                             sceneFolder;
    MoveComponentFn                           moveComponent;
    PreviewComponentSaveFn                    previewComponentSave;
    GetComponentLayersToSaveFn                getComponentLayersToSave;
};

struct EditForwardingFns
{
    SupportsEditForwardingFn supportsEditForwarding;
    EchoEditForwardingFn     echoEditForwarding;
    SetEchoEditForwardingFn  setEchoEditForwarding;
};

struct DccObjectFns
{
    IsDccObjectStageIncomingFn isDccObjectStageIncoming;
    IsDccObjectSharedStageFn   isDccObjectSharedStage;
};

struct LayerEditorDCCFunctions
{
    ComponentFns      component;
    EditForwardingFns editForwarding;
    DccObjectFns      dccObject;
};

// Registration API — per-group setters (play cleanly with #ifdef guards), plus a
// full-struct setter and a getter used by the test RAII helper.
LayerEditorAPI void setComponentFns(const ComponentFns&);
LayerEditorAPI void setEditForwardingFns(const EditForwardingFns&);
LayerEditorAPI void setDccObjectFns(const DccObjectFns&);
LayerEditorAPI void setLayerEditorDCCFunctions(const LayerEditorDCCFunctions&);
LayerEditorAPI const LayerEditorDCCFunctions& layerEditorDCCFunctions();

// Accessor free functions — callers never null-check; an unset std::function
// yields the documented default (false / empty / no-op, except
// isDccObjectSharedStage which defaults to true).
LayerEditorAPI void        saveComponent(const PXR_NS::UsdStageRefPtr&, const std::string&);
LayerEditorAPI void        reloadComponent(const std::string&);
LayerEditorAPI void        renameProxyShape(const std::string&, const std::string&);
LayerEditorAPI bool        isStageAComponent(const std::string&);
LayerEditorAPI bool        isUnsavedComponent(const PXR_NS::UsdStageRefPtr&);
LayerEditorAPI bool        shouldDisplayComponentInitialSaveDialog(
           const PXR_NS::UsdStageRefPtr&,
           const std::string&);
LayerEditorAPI std::string sceneFolder();
LayerEditorAPI std::string
moveComponent(const std::string&, const std::string&, const std::string&);
LayerEditorAPI std::string
previewComponentSave(const std::string&, const std::string&, const std::string&);
LayerEditorAPI std::vector<std::string> getComponentLayersToSave(const std::string&);

LayerEditorAPI bool supportsEditForwarding();
LayerEditorAPI bool echoEditForwarding();
LayerEditorAPI void setEchoEditForwarding(bool);

LayerEditorAPI bool isDccObjectStageIncoming(const std::string&);
LayerEditorAPI bool isDccObjectSharedStage(const std::string&);

} // namespace UsdLayerEditor

#endif // LAYER_EDITOR_DCC_FUNCTIONS_H
```

- [ ] **Step 2: Commit**

```bash
git add lib/usdUfe/usd-layer-editor/lib/layerEditorDCCFunctions.h
git commit -m "feat(le): add layer-editor DCC-functions registry header"
```

### Task 2: Create the registry implementation

**Files:**
- Create: `lib/usdUfe/usd-layer-editor/lib/layerEditorDCCFunctions.cpp`

- [ ] **Step 1: Write the implementation**

A single global instance; setters store; accessors null-check and reproduce today's defaults exactly. `isDccObjectSharedStage` defaults to `true`; everything else false / empty / no-op.

```cpp
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
#include "layerEditorDCCFunctions.h"

namespace UsdLayerEditor {

namespace {
LayerEditorDCCFunctions& registry()
{
    static LayerEditorDCCFunctions sFunctions;
    return sFunctions;
}
} // namespace

void setComponentFns(const ComponentFns& fns) { registry().component = fns; }
void setEditForwardingFns(const EditForwardingFns& fns) { registry().editForwarding = fns; }
void setDccObjectFns(const DccObjectFns& fns) { registry().dccObject = fns; }
void setLayerEditorDCCFunctions(const LayerEditorDCCFunctions& fns) { registry() = fns; }
const LayerEditorDCCFunctions& layerEditorDCCFunctions() { return registry(); }

// ---- Component ----
void saveComponent(const PXR_NS::UsdStageRefPtr& stage, const std::string& dccObjectPath)
{
    if (registry().component.saveComponent)
        registry().component.saveComponent(stage, dccObjectPath);
}
void reloadComponent(const std::string& dccObjectPath)
{
    if (registry().component.reloadComponent)
        registry().component.reloadComponent(dccObjectPath);
}
void renameProxyShape(const std::string& oldDccObjectPath, const std::string& newName)
{
    if (registry().component.renameProxyShape)
        registry().component.renameProxyShape(oldDccObjectPath, newName);
}
bool isStageAComponent(const std::string& dccObjectPath)
{
    return registry().component.isStageAComponent
        ? registry().component.isStageAComponent(dccObjectPath)
        : false;
}
bool isUnsavedComponent(const PXR_NS::UsdStageRefPtr& stage)
{
    return registry().component.isUnsavedComponent
        ? registry().component.isUnsavedComponent(stage)
        : false;
}
bool shouldDisplayComponentInitialSaveDialog(
    const PXR_NS::UsdStageRefPtr& stage,
    const std::string&            dccObjectPath)
{
    return registry().component.shouldDisplayComponentInitialSaveDialog
        ? registry().component.shouldDisplayComponentInitialSaveDialog(stage, dccObjectPath)
        : false;
}
std::string sceneFolder()
{
    return registry().component.sceneFolder ? registry().component.sceneFolder() : std::string {};
}
std::string moveComponent(
    const std::string& saveLocation,
    const std::string& componentName,
    const std::string& dccObjectPath)
{
    return registry().component.moveComponent
        ? registry().component.moveComponent(saveLocation, componentName, dccObjectPath)
        : std::string {};
}
std::string previewComponentSave(
    const std::string& saveLocation,
    const std::string& componentName,
    const std::string& dccObjectPath)
{
    return registry().component.previewComponentSave
        ? registry().component.previewComponentSave(saveLocation, componentName, dccObjectPath)
        : std::string {};
}
std::vector<std::string> getComponentLayersToSave(const std::string& dccObjectPath)
{
    return registry().component.getComponentLayersToSave
        ? registry().component.getComponentLayersToSave(dccObjectPath)
        : std::vector<std::string> {};
}

// ---- Edit Forwarding ----
bool supportsEditForwarding()
{
    return registry().editForwarding.supportsEditForwarding
        ? registry().editForwarding.supportsEditForwarding()
        : false;
}
bool echoEditForwarding()
{
    return registry().editForwarding.echoEditForwarding
        ? registry().editForwarding.echoEditForwarding()
        : false;
}
void setEchoEditForwarding(bool echo)
{
    if (registry().editForwarding.setEchoEditForwarding)
        registry().editForwarding.setEchoEditForwarding(echo);
}

// ---- DCC object/stage queries ----
bool isDccObjectStageIncoming(const std::string& dccObjectPath)
{
    return registry().dccObject.isDccObjectStageIncoming
        ? registry().dccObject.isDccObjectStageIncoming(dccObjectPath)
        : false;
}
bool isDccObjectSharedStage(const std::string& dccObjectPath)
{
    return registry().dccObject.isDccObjectSharedStage
        ? registry().dccObject.isDccObjectSharedStage(dccObjectPath)
        : true; // matches the former AbstractCommandHook default
}

} // namespace UsdLayerEditor
```

- [ ] **Step 2: Commit**

```bash
git add lib/usdUfe/usd-layer-editor/lib/layerEditorDCCFunctions.cpp
git commit -m "feat(le): implement layer-editor DCC-functions registry"
```

### Task 3: Register the new files in CMake and build

**Files:**
- Modify: `lib/usdUfe/usd-layer-editor/lib/CMakeLists.txt`

- [ ] **Step 1: Add the sources to the `add_library(UsdLayerEditorLib SHARED ...)` list**

Insert alphabetically, immediately after the `layerEditorCommands.cpp` line (line 42):

```cmake
    layerEditorCommands.cpp
    layerEditorDCCFunctions.cpp
    layerEditorDCCFunctions.h
    layerEditorWidget.cpp
```

- [ ] **Step 2: Build**

Run the relay `build`. Expected: `exit_code` 0; `UsdLayerEditorLib` compiles with the new TU.

- [ ] **Step 3: Run the existing layer-editor C++ tests (sanity — nothing reads the registry yet)**

Run the relay `test` with filter `LayerEditor`. Expected: all currently-passing tests still pass.

- [ ] **Step 4: Commit**

```bash
git add lib/usdUfe/usd-layer-editor/lib/CMakeLists.txt
git commit -m "build(le): compile layerEditorDCCFunctions into UsdLayerEditorLib"
```

---

## Phase 2 — Maya populates the registry at plugin init (dormant)

The registry is filled with lambdas that reproduce the current Maya override bodies. Behavior is unchanged because the shared-lib call sites still use the base-class virtuals (switched in Phase 3).

### Task 4: Create the Maya registration unit

**Files:**
- Create: `lib/usd/ui/layerEditor/mayaLayerEditorDCCFunctions.h`
- Create: `lib/usd/ui/layerEditor/mayaLayerEditorDCCFunctions.cpp`

- [ ] **Step 1: Write the header**

```cpp
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
#ifndef MAYA_LAYER_EDITOR_DCC_FUNCTIONS_H
#define MAYA_LAYER_EDITOR_DCC_FUNCTIONS_H

namespace UsdLayerEditor {

// Populates the shared layer-editor DCC-functions registry with the Maya
// implementations (Component Creator, Edit Forwarding, DCC object/stage
// queries). Call once at Maya plugin initialization.
void registerLayerEditorDCCFunctions();

// Clears the registry back to defaults. Call at plugin unload.
void deregisterLayerEditorDCCFunctions();

} // namespace UsdLayerEditor

#endif // MAYA_LAYER_EDITOR_DCC_FUNCTIONS_H
```

- [ ] **Step 2: Write the implementation**

The Component and Edit-Forwarding lambdas reproduce the bodies currently in `mayaSessionState.cpp`; the DCC-object lambdas reproduce `mayaCommandHook.cpp`. `getBooleanAttributeOnProxyShape` lives in an anonymous namespace in `mayaCommandHook.cpp` and is not reachable here, so a local copy is provided (Maya-only, ~15 lines). The echo state is read/written via the optionVar + EF host directly (it no longer lives on `MayaSessionState`).

```cpp
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
#include "mayaLayerEditorDCCFunctions.h"

#include <layerEditorDCCFunctions.h>

#include <mayaUsd/utils/utilComponentCreator.h>
#include <mayaUsd/utils/utilSerialization.h>

#include <pxr/usd/usd/stage.h>

#include <mayaUsd/utils/util.h>

#include <maya/MDagModifier.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#ifdef WANT_ADSK_USD_EDIT_FORWARD_BUILD
#include <mayaUsd/editForward/MayaUsdEditForwardHost.h>

#include <AdskUsdEditForward/Host.h>
#include <AdskUsdEditForward/StageRuleProvider.h>
#endif

namespace {

const MString kEchoEditForwardingOptionVar("mayaUsd_LayerEditor_EchoEditForwarding");

// Local copy of the proxy-shape boolean attribute reader (the original lives in
// an anonymous namespace in mayaCommandHook.cpp and is not reachable here).
std::string proxyShapeName(const std::string& proxyShapePath)
{
    std::size_t found = proxyShapePath.find_last_of("|");
    return (std::string::npos != found) ? proxyShapePath.substr(found + 1) : proxyShapePath;
}

bool getBooleanAttributeOnProxyShape(
    const std::string& proxyShapePath,
    const std::string& attributeName)
{
    if (proxyShapePath.empty())
        return false;

    MObject mobj;
    MStatus status = PXR_NS::UsdMayaUtil::GetMObjectByName(proxyShapeName(proxyShapePath), mobj);
    if (status == MStatus::kSuccess) {
        MFnDependencyNode fn;
        fn.setObject(mobj);
        bool attribute;
        if (PXR_NS::UsdMayaUtil::getPlugValue(fn, attributeName.c_str(), &attribute))
            return attribute;
    }
    return false;
}

} // namespace

namespace UsdLayerEditor {

void registerLayerEditorDCCFunctions()
{
#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)
    ComponentFns component;
    component.saveComponent
        = [](const PXR_NS::UsdStageRefPtr& /*stage*/, const std::string& dccObjectPath) {
              MayaUsd::ComponentUtils::saveAdskUsdComponent(dccObjectPath);
          };
    component.reloadComponent = [](const std::string& dccObjectPath) {
        MayaUsd::ComponentUtils::reloadAdskUsdComponent(dccObjectPath);
    };
    component.renameProxyShape
        = [](const std::string& oldDccObjectPath, const std::string& newName) {
              if (oldDccObjectPath.empty() || newName.empty())
                  return;
              MObject proxyNode;
              if (PXR_NS::UsdMayaUtil::GetMObjectByName(oldDccObjectPath, proxyNode)
                  != MStatus::kSuccess)
                  return;
              MDagModifier dagMod;
              if (dagMod.renameNode(proxyNode, newName.c_str()) == MStatus::kSuccess)
                  dagMod.doIt();
          };
    component.isStageAComponent = [](const std::string& dccObjectPath) {
        if (dccObjectPath.empty())
            return false;
        return MayaUsd::ComponentUtils::isAdskUsdComponent(dccObjectPath);
    };
    component.isUnsavedComponent = [](const PXR_NS::UsdStageRefPtr& stage) {
        return MayaUsd::ComponentUtils::isUnsavedAdskUsdComponent(stage);
    };
    component.shouldDisplayComponentInitialSaveDialog
        = [](const PXR_NS::UsdStageRefPtr& stage, const std::string& dccObjectPath) {
              return MayaUsd::ComponentUtils::shouldDisplayComponentInitialSaveDialog(
                  stage, dccObjectPath);
          };
    component.sceneFolder = []() { return MayaUsd::utils::getSceneFolder(); };
    component.moveComponent = [](const std::string& saveLocation,
                                 const std::string& componentName,
                                 const std::string& dccObjectPath) {
        return MayaUsd::ComponentUtils::moveAdskUsdComponent(
            saveLocation, componentName, dccObjectPath);
    };
    component.previewComponentSave = [](const std::string& saveLocation,
                                        const std::string& componentName,
                                        const std::string& dccObjectPath) {
        return MayaUsd::ComponentUtils::previewSaveAdskUsdComponent(
            saveLocation, componentName, dccObjectPath);
    };
    component.getComponentLayersToSave = [](const std::string& dccObjectPath) {
        return MayaUsd::ComponentUtils::getAdskUsdComponentLayersToSave(dccObjectPath);
    };
    setComponentFns(component);

    DccObjectFns dccObject;
    dccObject.isDccObjectStageIncoming = [](const std::string& dccObjectPath) {
        return getBooleanAttributeOnProxyShape(dccObjectPath, "stageIncoming");
    };
    dccObject.isDccObjectSharedStage = [](const std::string& dccObjectPath) {
        return getBooleanAttributeOnProxyShape(dccObjectPath, "shareStage");
    };
    setDccObjectFns(dccObject);

#ifdef WANT_ADSK_USD_EDIT_FORWARD_BUILD
    EditForwardingFns editForwarding;
    editForwarding.supportsEditForwarding = []() { return true; };
    editForwarding.echoEditForwarding = []() {
        return MGlobal::optionVarExists(kEchoEditForwardingOptionVar)
            && MGlobal::optionVarIntValue(kEchoEditForwardingOptionVar) != 0;
    };
    editForwarding.setEchoEditForwarding = [](bool echo) {
        MGlobal::setOptionVarValue(kEchoEditForwardingOptionVar, echo ? 1 : 0);
        if (auto host = std::dynamic_pointer_cast<MayaUsdEditForwardHost>(
                AdskUsdEditForward::Host::GetInstance())) {
            host->SetWantsEcho(echo);
        }
    };
    setEditForwardingFns(editForwarding);
#endif
#endif // MAYAUSD_USE_SHARED_LAYER_EDITOR
}

void deregisterLayerEditorDCCFunctions()
{
    setLayerEditorDCCFunctions(LayerEditorDCCFunctions {});
}

} // namespace UsdLayerEditor
```

> **Note for the implementer:** confirm the exact option-var string for echo. The current code declares `MString ECHO_EDIT_FORWARDING_OPTION_VAR` near `mayaSessionState.cpp:75` — copy its literal value into `kEchoEditForwardingOptionVar` above so the echo preference key is byte-identical (do **not** invent a new key). Likewise confirm `MayaUsd::ComponentUtils::previewSaveAdskUsdComponent` is the exact symbol name used by `MayaSessionState::previewComponentSave` (`mayaSessionState.cpp:732`).

- [ ] **Step 3: Commit**

```bash
git add lib/usd/ui/layerEditor/mayaLayerEditorDCCFunctions.h \
        lib/usd/ui/layerEditor/mayaLayerEditorDCCFunctions.cpp
git commit -m "feat(maya-le): add DCC-functions registration unit"
```

### Task 5: Compile the new unit into mayaUsdUI and wire register/deregister

**Files:**
- Modify: `lib/usd/ui/layerEditor/CMakeLists.txt`
- Modify: `lib/usd/ui/layerEditor/batchSaveLayersUIDelegate.cpp` (call register inside `UsdLayerEditor::initialize()`)
- Modify: `plugin/adsk/plugin/plugin.cpp` (call deregister at unload)

- [ ] **Step 1: Add the source to the `mayaUsdUI` target**

In `lib/usd/ui/layerEditor/CMakeLists.txt`, find the Maya-wiring sources list (the one that already lists `mayaCommandHook.cpp` / `mayaSessionState.cpp`) and add:

```cmake
    mayaLayerEditorDCCFunctions.cpp
    mayaLayerEditorDCCFunctions.h
```

- [ ] **Step 2: Call `registerLayerEditorDCCFunctions()` from `UsdLayerEditor::initialize()`**

In `lib/usd/ui/layerEditor/batchSaveLayersUIDelegate.cpp`, add the include near the top:

```cpp
#include "mayaLayerEditorDCCFunctions.h"
```

Then, inside `void UsdLayerEditor::initialize()`, within the existing `#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)` block (alongside the other injection calls such as `setDCCSceneLocationFunc`), add as the first statement of the block:

```cpp
    UsdLayerEditor::registerLayerEditorDCCFunctions();
```

- [ ] **Step 3: Call `deregisterLayerEditorDCCFunctions()` at plugin unload**

In `plugin/adsk/plugin/plugin.cpp`, add the include near the other layer-editor includes (around line 34-35):

```cpp
#include <mayaUsdUI/ui/mayaLayerEditorDCCFunctions.h>
```

In `uninitializePlugin`, inside the existing `#if defined(WANT_QT_BUILD)` teardown block that calls `MayaUsd::LayerManager::SetBatchSaveDelegate(nullptr);` (around line 630-631), add immediately after that line:

```cpp
    UsdLayerEditor::deregisterLayerEditorDCCFunctions();
```

> **Note:** confirm the include path prefix (`mayaUsdUI/ui/...`) matches how `plugin.cpp` already includes `batchSaveLayersUIDelegate.h` at line 99 (`#include <mayaUsdUI/ui/batchSaveLayersUIDelegate.h>`). Use the identical prefix.

- [ ] **Step 4: Build everything**

Run the relay `build`. Expected: `exit_code` 0. The registry is now populated at plugin init but still unused (call sites unchanged) → no behavior change.

- [ ] **Step 5: Run all layer-editor tests**

Run the relay `test` with filter `LayerEditor`. Expected: unchanged pass set.

- [ ] **Step 6: Commit**

```bash
git add lib/usd/ui/layerEditor/CMakeLists.txt \
        lib/usd/ui/layerEditor/batchSaveLayersUIDelegate.cpp \
        plugin/adsk/plugin/plugin.cpp
git commit -m "feat(maya-le): register DCC functions at plugin init / clear at unload"
```

---

## Phase 3 — Switch shared-lib call sites to the registry (+ test plumbing)

This phase is **atomic**: the call-site switch and the test-fixture switch land together so both editor test targets stay green. Do all steps, then build+test once at the end.

### Task 6: Add the RAII test helper

**Files:**
- Create: `lib/usdUfe/usd-layer-editor/test/cpp/scopedLayerEditorDCCFunctions.h`

- [ ] **Step 1: Write the helper (header-only)**

```cpp
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

#include "layerEditorDCCFunctions.h"

namespace UsdLayerEditor {

// Installs registry state on construction and restores the previous state on
// destruction, so tests that exercise component / EF / DCC-object behavior do
// not leak global registry state between cases.
class ScopedLayerEditorDCCFunctions
{
public:
    ScopedLayerEditorDCCFunctions()
        : _saved(layerEditorDCCFunctions())
    {
    }
    ~ScopedLayerEditorDCCFunctions() { setLayerEditorDCCFunctions(_saved); }

    ScopedLayerEditorDCCFunctions(const ScopedLayerEditorDCCFunctions&) = delete;
    ScopedLayerEditorDCCFunctions& operator=(const ScopedLayerEditorDCCFunctions&) = delete;

private:
    LayerEditorDCCFunctions _saved;
};

} // namespace UsdLayerEditor
```

- [ ] **Step 2: Commit**

```bash
git add lib/usdUfe/usd-layer-editor/test/cpp/scopedLayerEditorDCCFunctions.h
git commit -m "test(le): add ScopedLayerEditorDCCFunctions RAII helper"
```

### Task 7: Switch the four shared-lib call-site files to the registry accessors

**Files:**
- Modify: `lib/usdUfe/usd-layer-editor/lib/layerTreeModel.cpp`
- Modify: `lib/usdUfe/usd-layer-editor/lib/saveLayersDialog.cpp`
- Modify: `lib/usdUfe/usd-layer-editor/lib/componentSaveWidget.cpp`
- Modify: `lib/usdUfe/usd-layer-editor/lib/layerEditorWidget.cpp`

In each file, add `#include "layerEditorDCCFunctions.h"` with the other local includes, then replace the member/`commandHook()` calls with the `UsdLayerEditor::` free functions. The replacements (do these exact edits):

- [ ] **Step 1: `layerTreeModel.cpp`**

`isDccObjectSharedStage` (line ~332):
```cpp
    auto                  sharedStage = UsdLayerEditor::isDccObjectSharedStage(
        _sessionState->stageEntry()._dccObjectPath);
```
`isDccObjectStageIncoming` (line ~347):
```cpp
    if (UsdLayerEditor::isDccObjectStageIncoming(
            _sessionState->stageEntry()._dccObjectPath)) {
```
`isStageAComponent` + `saveComponent` (lines ~571-574):
```cpp
        if (_sessionState
            && UsdLayerEditor::isStageAComponent(_sessionState->stageEntry()._dccObjectPath)) {
            UsdLayerEditor::saveComponent(
                _sessionState->stageEntry()._stage, _sessionState->stageEntry()._dccObjectPath);
            return;
        }
```
`shouldDisplayComponentInitialSaveDialog` (lines ~604-606):
```cpp
    if (_sessionState
        && UsdLayerEditor::shouldDisplayComponentInitialSaveDialog(
            _sessionState->stageEntry()._stage, _sessionState->stageEntry()._dccObjectPath)) {
```
`isUnsavedComponent` + `reloadComponent` (lines ~648, ~651):
```cpp
    if (UsdLayerEditor::isUnsavedComponent(_sessionState->stage())) {
        return;
    }
    UsdLayerEditor::reloadComponent(_sessionState->stageEntry()._dccObjectPath);
```

- [ ] **Step 2: `saveLayersDialog.cpp`**

`shouldDisplayComponentInitialSaveDialog` (line ~485):
```cpp
        const bool isComponent
            = _sessionState
            && UsdLayerEditor::shouldDisplayComponentInitialSaveDialog(info.stage, dccObjectPath);
```
`shouldDisplayComponentInitialSaveDialog` (line ~532):
```cpp
        if (UsdLayerEditor::shouldDisplayComponentInitialSaveDialog(
                stageEntry._stage, stageEntry._dccObjectPath)) {
```
`moveComponent` (line ~880):
```cpp
            newRootPath = UsdLayerEditor::moveComponent(saveLocation, componentName, dccObjectPath);
```
`renameProxyShape` (line ~891):
```cpp
                UsdLayerEditor::renameProxyShape(dccObjectPath, componentName);
```

- [ ] **Step 3: `componentSaveWidget.cpp`**

`sceneFolder` (lines ~76 and ~233): replace `_sessionState->sceneFolder()` with `UsdLayerEditor::sceneFolder()`. (Keep the surrounding `if (_sessionState)` guards as-is — they guard other setup and keep `_sessionState` referenced.)
`previewComponentSave` (line ~340):
```cpp
        result = UsdLayerEditor::previewComponentSave(saveLocation, componentName, _dccObjectPath);
```

- [ ] **Step 4: `layerEditorWidget.cpp`**

`supportsEditForwarding` (line ~123):
```cpp
        if (UsdLayerEditor::supportsEditForwarding()) {
```
EF menu connect + checked (lines ~127-133): connect to a lambda (a free function can't be a Qt receiver-slot the same way) and read the registry:
```cpp
            QObject::connect(
                _actions._echoEditForwarding,
                &QAction::toggled,
                [](bool checked) { UsdLayerEditor::setEchoEditForwarding(checked); });
            _actions._echoEditForwarding->setCheckable(true);
            _actions._echoEditForwarding->setChecked(UsdLayerEditor::echoEditForwarding());
```
`supportsEditForwarding` (line ~223):
```cpp
    if (UsdLayerEditor::supportsEditForwarding()) {
```
`isDccObjectSharedStage` (line ~459):
```cpp
    if (UsdLayerEditor::isDccObjectSharedStage(
            _sessionState.stageEntry()._dccObjectPath)) {
```
`isStageAComponent` + `getComponentLayersToSave` (lines ~476-478):
```cpp
        if (UsdLayerEditor::isStageAComponent(_sessionState.stageEntry()._dccObjectPath)) {
            const auto layerIds = UsdLayerEditor::getComponentLayersToSave(
                _sessionState.stageEntry()._dccObjectPath);
```

Do not commit yet — the C++ tests would fail until Task 8/9 land. Proceed.

### Task 8: New-editor fixture drives the registry

**Files:**
- Modify: `lib/usdUfe/usd-layer-editor/test/cpp/testFixture.h`
- Modify: `lib/usdUfe/usd-layer-editor/test/cpp/testFixture.cpp`

- [ ] **Step 1: Add include, flags, scoped member, and setter methods to `testFixture.h`**

Add the include near the existing test includes:
```cpp
#include "scopedLayerEditorDCCFunctions.h"
```
Add to the `protected:` section of `LayerEditorTestFixture` (after `_widget`):
```cpp
    // DCC-function registry driven by these flags (installed in SetUp,
    // restored in TearDown). Lambdas read the flags at call time, so flips
    // mid-test take effect on the next model rebuild.
    bool _efSupported  { false };
    bool _sharedStage  { false };
    bool _stageIncoming { false };
    ScopedLayerEditorDCCFunctions _scopedDCCFunctions;

    void setEditForwardingSupported(bool supported) { _efSupported = supported; }
    void setSharedStage(bool shared) { _sharedStage = shared; }
    void setStageIncoming(bool incoming) { _stageIncoming = incoming; }
```

> Declaration order matters: declare `_scopedDCCFunctions` **after** the three bool flags so it is destroyed first (restoring the registry while the flags are still alive is harmless, but keep ordering tidy).

- [ ] **Step 2: Install the baseline registry at the start of `LayerEditorTestFixture::SetUp()`**

Add `#include "layerEditorDCCFunctions.h"` to `testFixture.cpp` (if not already pulled in transitively). Then, in `testFixture.cpp`, at the top of `SetUp()` (before the widget is constructed), add — assigning each field explicitly so the `DccObjectFns` field order can't be transposed:

```cpp
    EditForwardingFns ef;
    ef.supportsEditForwarding = [this]() { return _efSupported; };
    ef.echoEditForwarding = []() { return false; };
    ef.setEchoEditForwarding = [](bool) {};
    setEditForwardingFns(ef);

    DccObjectFns dcc;
    dcc.isDccObjectStageIncoming = [this](const std::string&) { return _stageIncoming; };
    dcc.isDccObjectSharedStage = [this](const std::string&) { return _sharedStage; };
    setDccObjectFns(dcc);
```

The lambdas capture `this` and read the flags at call time, so flipping a flag mid-test (then rebuilding the model) takes effect. Component functions are left unset — their accessor defaults (false / empty) match the previous stub behavior.

### Task 9: Old-editor fixture sets the legacy stub members; shared logic headers call fixture methods

**Files:**
- Modify: `lib/usd/ui/layerEditor/test/cpp/testFixture.h`
- Modify: `lib/usdUfe/usd-layer-editor/test/cpp/testEFModeLogic.h`
- Modify: `lib/usdUfe/usd-layer-editor/test/cpp/testSharedStageLogic.h`

- [ ] **Step 1: Add the same-signature setter methods to the OLD fixture**

In `lib/usd/ui/layerEditor/test/cpp/testFixture.h`, add to the `protected:` section of `LayerEditorTestFixture` (the old one, member `_sessionState` is `OldEditorStubSessionState`):
```cpp
    void setEditForwardingSupported(bool supported) { _sessionState._supportsEditForwarding = supported; }
    void setSharedStage(bool shared) { _sessionState._commandHookImpl._isSharedStage = shared; }
    void setStageIncoming(bool incoming) { _sessionState._commandHookImpl._isStageIncoming = incoming; }
```

- [ ] **Step 2: Replace direct member pokes in `testEFModeLogic.h`**

Line ~44 (`LayerEditorWithEFFixture::SetUp`):
```cpp
        setEditForwardingSupported(true);
```
(replacing `_sessionState._supportsEditForwarding = true;`)

- [ ] **Step 3: Replace direct member pokes in `testSharedStageLogic.h`**

Every `_sessionState._commandHookImpl._isSharedStage = X;` → `setSharedStage(X);`
Every `_sessionState._commandHookImpl._isStageIncoming = X;` → `setStageIncoming(X);`

Known sites (verify by grep before/after): lines ~61, ~119, ~130 (`_isSharedStage`), and ~240-241 (`_isSharedStage` + `_isStageIncoming`). Run:
```bash
grep -n "_commandHookImpl._isSharedStage\|_commandHookImpl._isStageIncoming" \
  lib/usdUfe/usd-layer-editor/test/cpp/testSharedStageLogic.h
```
Expected after edits: no matches.

### Task 10: Build and test BOTH editor targets

- [ ] **Step 1: Build everything**

Run the relay `build`. Expected: `exit_code` 0. Both `UsdLayerEditorLib`, `mayaUsdUI`, the new-editor C++ test, and `mayaUsdOldLayerEditorTests` compile. The new stubs still carry their (now-dead) overrides — harmless.

- [ ] **Step 2: Run all layer-editor tests (both editors)**

Run the relay `test` with filter `LayerEditor`. Expected: the full pre-existing pass set, including `EFMode_*` and `SharedStage*` for both the new and old targets.

If `SharedStage*` tests fail with "needs saving" assertions, the registry baseline default didn't take — re-check Task 8 Step 2 installs `isDccObjectSharedStage` returning `_sharedStage` (default false), not the registry's hard default of true.

- [ ] **Step 3: Commit the whole atomic switch**

```bash
git add lib/usdUfe/usd-layer-editor/lib/layerTreeModel.cpp \
        lib/usdUfe/usd-layer-editor/lib/saveLayersDialog.cpp \
        lib/usdUfe/usd-layer-editor/lib/componentSaveWidget.cpp \
        lib/usdUfe/usd-layer-editor/lib/layerEditorWidget.cpp \
        lib/usdUfe/usd-layer-editor/test/cpp/testFixture.h \
        lib/usdUfe/usd-layer-editor/test/cpp/testFixture.cpp \
        lib/usdUfe/usd-layer-editor/test/cpp/testEFModeLogic.h \
        lib/usdUfe/usd-layer-editor/test/cpp/testSharedStageLogic.h \
        lib/usd/ui/layerEditor/test/cpp/testFixture.h
git commit -m "refactor(le): read DCC functions from the registry; route tests through fixture"
```

---

## Phase 4 — Delete the dead virtuals, overrides, and stub members

Now that nothing calls the base-class virtuals, remove them. This is atomic with removing the Maya overrides (otherwise the full build won't compile).

### Task 11: Remove the virtuals from the shared base classes

**Files:**
- Modify: `lib/usdUfe/usd-layer-editor/lib/abstractCommandHook.h`
- Modify: `lib/usdUfe/usd-layer-editor/lib/sessionState.h`

- [ ] **Step 1: `abstractCommandHook.h`** — delete the five virtuals and their comment block (lines ~109-131): `isDccObjectStageIncoming`, `isDccObjectSharedStage`, `saveComponent`, `reloadComponent`, `renameProxyShape`.

- [ ] **Step 2: `sessionState.h`** — delete:
  - the Edit-Forwarding comment block (lines ~94-105) and the three EF query virtuals `supportsEditForwarding`, `echoEditForwarding`, `setEchoEditForwarding`, **and** the now-orphaned `hasEditForwarding` virtual (line ~107). **Keep** `isEditForwardMode()` and `effectiveTargetLayer()` (lines ~110-111) — they are out of scope.
  - the Component-Creator comment block (lines ~113-121) and the seven CC virtuals: `isStageAComponent`, `isUnsavedComponent`, `shouldDisplayComponentInitialSaveDialog`, `sceneFolder`, `moveComponent`, `previewComponentSave`, `getComponentLayersToSave` (lines ~122-158).
  - **Keep** the `editForwardingChanged()` signal and the `displayLayer*` options/members.

### Task 12: Remove the Maya overrides and the echo member

**Files:**
- Modify: `lib/usd/ui/layerEditor/mayaCommandHook.h`
- Modify: `lib/usd/ui/layerEditor/mayaCommandHook.cpp`
- Modify: `lib/usd/ui/layerEditor/mayaSessionState.h`
- Modify: `lib/usd/ui/layerEditor/mayaSessionState.cpp`

- [ ] **Step 1: `mayaCommandHook.h`** — remove the override declarations: `isDccObjectStageIncoming`, `isDccObjectSharedStage`, `saveComponent`, `reloadComponent`, `renameProxyShape` (lines ~106-117, the ones currently marked `override`).

- [ ] **Step 2: `mayaCommandHook.cpp`** — remove the override definitions in the `#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)` block (lines ~328-366): `isDccObjectStageIncoming`, `isDccObjectSharedStage`, `saveComponent`, `reloadComponent`, `renameProxyShape`. Leave the `#else` legacy branch (`isProxyShape*`) intact. The anonymous-namespace `getBooleanAttributeOnProxyShape` is still referenced by the `#else` branch — leave it.

- [ ] **Step 3: `mayaSessionState.h`** — remove the override declarations for: `setEchoEditForwarding`, `supportsEditForwarding`, `hasEditForwarding`, `echoEditForwarding`, `isStageAComponent`, `isUnsavedComponent`, `shouldDisplayComponentInitialSaveDialog`, `sceneFolder`, `moveComponent`, `previewComponentSave`, `getComponentLayersToSave`. Also remove the `bool _echoEditForwarding { false };` member (line ~165). **Keep** `isEditForwardMode()` and `effectiveTargetLayer()` overrides.

- [ ] **Step 4: `mayaSessionState.cpp`** — remove the corresponding definitions: `setEchoEditForwarding` (lines ~530-542), `supportsEditForwarding` (~673-680), `hasEditForwarding` (~682-693), `echoEditForwarding` (~695), and the seven CC definitions (`isStageAComponent` … `getComponentLayersToSave`, ~697-740). **Keep** `isEditForwardMode` (~544-551) and `effectiveTargetLayer` (~553-567). Also remove the `_echoEditForwarding` initialization from the constructor (the optionVar seeding near lines ~76 and ~93-94) since the member no longer exists.

> The echo preference is now owned entirely by the registry lambdas (optionVar + EF host) installed in `registerLayerEditorDCCFunctions()`. Verify after removal that `ECHO_EDIT_FORWARDING_OPTION_VAR` is no longer referenced in `mayaSessionState.cpp` (grep) — if it becomes unused, remove its declaration too.

### Task 13: Remove the dead overrides/members from the new-editor stubs

**Files:**
- Modify: `lib/usdUfe/usd-layer-editor/test/cpp/stubSessionState.h`
- Modify: `lib/usdUfe/usd-layer-editor/test/cpp/stubCommandHook.h`

- [ ] **Step 1: `stubSessionState.h`** — remove `bool supportsEditForwarding() const override { return _supportsEditForwarding; }` (line ~54) and the `bool _supportsEditForwarding { false };` member (line ~63). **Keep** `isEditForwardMode()` override, `setIsEditForwardMode`, and `_isEFModeActive` (those drive `editForwardingChanged` and the still-present `isEditForwardMode` virtual).

- [ ] **Step 2: `stubCommandHook.h`** — remove the two overrides `isDccObjectSharedStage` / `isDccObjectStageIncoming` (lines ~40-41) and the `_isSharedStage` / `_isStageIncoming` members (lines ~37-38).

> Do **not** touch the old-editor stubs (`lib/usd/ui/layerEditor/test/cpp/stub*.h`) — they target the legacy base classes and the old fixture still reads their members.

### Task 14: Full build + complete test run (both editors)

- [ ] **Step 1: Build everything**

Run the relay `build`. Expected: `exit_code` 0. Resolve any "no member named ... in SessionState/AbstractCommandHook" errors — they indicate a call site or override that still references a removed virtual; it must already be on the registry path.

- [ ] **Step 2: Run the full test suite (unfiltered)**

```bash
python3 _host_command/relay_client.py run test \
  --db _host_command/relay.db --commands-json _host_command/commands.json
```
Expected: `exit_code` 0. Confirm both `UsdLayerEditorNewTests` (new editor) and `mayaUsdOldLayerEditorTests` (old editor parity) pass, plus the Python layer-editor tests.

- [ ] **Step 3: Commit**

```bash
git add lib/usdUfe/usd-layer-editor/lib/abstractCommandHook.h \
        lib/usdUfe/usd-layer-editor/lib/sessionState.h \
        lib/usd/ui/layerEditor/mayaCommandHook.h \
        lib/usd/ui/layerEditor/mayaCommandHook.cpp \
        lib/usd/ui/layerEditor/mayaSessionState.h \
        lib/usd/ui/layerEditor/mayaSessionState.cpp \
        lib/usdUfe/usd-layer-editor/test/cpp/stubSessionState.h \
        lib/usdUfe/usd-layer-editor/test/cpp/stubCommandHook.h
git commit -m "refactor(le): remove DCC virtuals/overrides now served by the registry"
```

---

## Final verification checklist

- [ ] Full relay `build` is green.
- [ ] Full relay `test` is green — **new editor** (`UsdLayerEditorNewTests`) and **old editor** (`mayaUsdOldLayerEditorTests`) and Python LE tests.
- [ ] `grep -rn "hasEditForwarding" lib/` returns only the MIGRATION.md historical note (no code).
- [ ] `grep -rn "saveComponent\|isDccObjectSharedStage\|supportsEditForwarding" lib/usdUfe/usd-layer-editor/lib/sessionState.h lib/usdUfe/usd-layer-editor/lib/abstractCommandHook.h` returns nothing.
- [ ] No call site in `lib/usdUfe/usd-layer-editor/lib/` calls these methods via `_sessionState->` or `commandHook()->` anymore (all go through `UsdLayerEditor::`).
- [ ] Update `MIGRATION.md`'s stale "bridge deferred" note if appropriate (optional; coordinate with the user).
```

