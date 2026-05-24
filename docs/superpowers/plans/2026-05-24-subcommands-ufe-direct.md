# Replace _subCommands with Direct UFE Commands Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Eliminate the entire `Impl` anonymous namespace from `layerEditorCommand.cpp` by storing `Ufe::UndoableCommand` instances directly in `_subCommands`, removing ~750 lines of thin-wrapper boilerplate.

**Architecture:** Move layer/stage retrieval into `parseArgs()`, construct shared-library UFE commands there with arguments baked in, change `_subCommands` to `vector<shared_ptr<Ufe::UndoableCommand>>`, and simplify `redoIt()`/`undoIt()` to trivial loops. `IndexAdjustments` (the only non-wrapper utility class in `Impl`) moves to the file-level anonymous namespace.

**Tech Stack:** C++17, OpenUSD (`SdfLayer`, `UsdStage`), UFE (`Ufe::UndoableCommand`), Maya API (`MPxCommand`, `MArgParser`), shared layer editor library (`UsdLayerEditor::*Cmd` types from `LayerEditorCommands.h`).

---

## Files

- **Modify:** `lib/mayaUsd/commands/layerEditorCommand.h` — change `_subCommands` type, add UFE include, remove `Impl` forward declaration
- **Modify:** `lib/mayaUsd/commands/layerEditorCommand.cpp` — delete `Impl` namespace (keep `IndexAdjustments` in anon namespace), rewrite `parseArgs()` command-construction block, rewrite `redoIt()`/`undoIt()`

---

### Task 1: Update the header

**Files:**
- Modify: `lib/mayaUsd/commands/layerEditorCommand.h`

- [ ] **Step 1: Replace the forward declaration and member type**

Replace the entire current header content with the following (only the forward-decl block and `_subCommands` line change; everything else is identical):

```cpp
//
// Copyright 2020 Autodesk
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

#ifndef MAYAUSD_COMMANDS_LAYER_EDITOR_COMMAND_H
#define MAYAUSD_COMMANDS_LAYER_EDITOR_COMMAND_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/mayaUsd.h>

#include <ufe/undoableCommand.h>

#include <maya/MPxCommand.h>
#include <maya/MString.h>

#include <memory>
#include <string>
#include <vector>

namespace MAYAUSD_NS_DEF {

class MAYAUSD_CORE_PUBLIC LayerEditorCommand : public MPxCommand
{
public:
    // plugin registration requirements
    static const char commandName[];
    static void*      creator();
    static MSyntax    createSyntax();

    // Lifecycle hooks called from plugin initialize/uninitialize
    static void registerBackupStagesProvider();
    static void unregisterBackupStagesProvider();

    // MPxCommand callbacks
    MStatus doIt(const MArgList& argList) override;
    MStatus undoIt() override;
    MStatus redoIt() override;
    bool    isUndoable() const override;

private:
    MStatus parseArgs(const MArgList& argList);

    enum class Mode
    {
        Create,
        Edit,
        Query
    } _cmdMode
        = Mode::Create;
    bool isEdit() const { return _cmdMode == Mode::Edit; }
    bool isQuery() const { return _cmdMode == Mode::Query; }

    std::string                                          _layerIdentifier;
    std::vector<std::shared_ptr<Ufe::UndoableCommand>>  _subCommands;
};

} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_COMMANDS_LAYER_EDITOR_COMMAND_H
```

Changes from current:
- Removed `#include <pxr/usd/sdf/layer.h>` (no longer needed in the header)
- Removed `namespace Impl { class BaseCmd; }` forward declaration
- Added `#include <ufe/undoableCommand.h>`
- Changed `_subCommands` from `vector<shared_ptr<Impl::BaseCmd>>` to `vector<shared_ptr<Ufe::UndoableCommand>>`

---

### Task 2: Move `IndexAdjustments` to the anonymous namespace

**Files:**
- Modify: `lib/mayaUsd/commands/layerEditorCommand.cpp` lines 69–106

`IndexAdjustments` is the only class in `Impl` that is not a command wrapper — it tracks index offsets when multiple insert/remove operations are batched in one `parseArgs()` call. It must be kept.

Current anonymous namespace ends at line 106. The `Impl` namespace begins at line 110.

- [ ] **Step 1: Cut `IndexAdjustments` from the `Impl` namespace and paste into the anonymous namespace**

The anonymous namespace currently ends at line 106:
```cpp
} // namespace
```

Replace that closing brace with the `IndexAdjustments` class inserted before it:

```cpp
// Tracks index offsets when multiple insert/remove sublayer operations are batched
// in a single command invocation. Removal shifts subsequent indexes down by 1;
// insertion shifts them up by 1.
class IndexAdjustments
{
public:
    IndexAdjustments() = default;

    int insertionAdjustment(int originalIndex)
    {
        const int adjustedIndex = getAdjustedIndex(originalIndex);
        addInsertionAdjustment(originalIndex);
        return adjustedIndex;
    }

    int removalAdjustment(int originalIndex)
    {
        const int adjustedIndex = getAdjustedIndex(originalIndex);
        addRemovalAdjustment(originalIndex);
        return adjustedIndex;
    }

private:
    void addInsertionAdjustment(int index) { _indexAdjustments[index] += 1; }
    void addRemovalAdjustment(int index) { _indexAdjustments[index] -= 1; }

    int getAdjustedIndex(int index) const
    {
        int adjustedIndex = index;
        for (const auto& indexAndAdjustement : _indexAdjustments) {
            if (indexAndAdjustement.first > index)
                break;
            adjustedIndex += indexAndAdjustement.second;
        }
        return adjustedIndex;
    }

    std::map<int, int> _indexAdjustments;
};

} // namespace
```

The edit to make: find the line `} // namespace` that closes the anonymous namespace (line 106) and replace it with the block above.

---

### Task 3: Delete the `Impl` namespace

**Files:**
- Modify: `lib/mayaUsd/commands/layerEditorCommand.cpp`

After Task 2, the `Impl` namespace spans from line 110 (`namespace Impl {`) to approximately line 862 (`} // namespace Impl`). It contains: `CmdId` enum, `BaseCmd` and all thin-wrapper subclasses, `InsertRemoveSubPathBase`, and `IndexAdjustments` (now already moved).

- [ ] **Step 1: Delete everything from `namespace Impl {` through `} // namespace Impl`**

Delete the block starting with:
```cpp
namespace Impl {

enum class CmdId
```
and ending with:
```cpp
} // namespace Impl
```

This removes all of: `CmdId`, `BaseCmd`, `InsertRemoveSubPathBase`, `InsertSubPath`, `RemoveSubPath`, `MoveSubPath`, `ReplaceSubPath`, `AddAnonSubLayer`, `DiscardEdit`, `ClearLayer`, `FlattenLayer`, `StitchLayers`, `MuteLayer`, `LockLayer`, `RefreshSystemLockLayer`, and `IndexAdjustments` (already moved).

---

### Task 4: Rewrite `parseArgs()` command-construction block

**Files:**
- Modify: `lib/mayaUsd/commands/layerEditorCommand.cpp`

The existing `parseArgs()` function is at approximately line 916. The first half (mode detection, `_layerIdentifier` extraction) stays unchanged. Only the `if (!isQuery())` block is replaced.

- [ ] **Step 1: Replace the `if (!isQuery())` block**

Find the current block starting with:
```cpp
    if (!isQuery()) {
        Impl::IndexAdjustments indexAdjustments;
```
and ending with the closing `}` of `if (!isQuery())` (currently around line 1144, just before `return MS::kSuccess;`).

Replace it with:

```cpp
    if (!isQuery()) {
        auto layer = SdfLayer::FindOrOpen(_layerIdentifier);
        if (!layer) {
            displayError(MString("Layer not found: ") + _layerIdentifier.c_str());
            return MS::kInvalidParameter;
        }

        IndexAdjustments indexAdjustments;

        const bool skipSystemLockedLayers = argParser.isFlagSet(kSkipSystemLockedFlag);

        if (argParser.isFlagSet(kInsertSubPathFlag)) {
            auto count = argParser.numberOfFlagUses(kInsertSubPathFlag);
            for (unsigned i = 0; i < count; i++) {
                MArgList listOfArgs;
                argParser.getFlagArgumentList(kInsertSubPathFlag, i, listOfArgs);
                const int originalIndex = listOfArgs.asInt(0);
                const int adjustedIndex = indexAdjustments.insertionAdjustment(originalIndex);
                _subCommands.push_back(std::make_shared<UsdLayerEditor::InsertSubPathCmd>(
                    UsdStageRefPtr {}, layer, listOfArgs.asString(1).asUTF8(), adjustedIndex));
            }
        }

        if (argParser.isFlagSet(kRemoveSubPathFlag)) {
            auto count = argParser.numberOfFlagUses(kRemoveSubPathFlag);
            for (unsigned i = 0; i < count; i++) {
                MArgList listOfArgs;
                argParser.getFlagArgumentList(kRemoveSubPathFlag, i, listOfArgs);
                auto shapePath = listOfArgs.asString(1);
                auto prim = UsdMayaQuery::GetPrim(shapePath.asChar());
                if (prim == UsdPrim()) {
                    displayError(MString("Invalid proxy shape \"") + shapePath.asChar() + "\"");
                    return MS::kInvalidParameter;
                }
                UsdStageRefPtr     stage = prim.GetStage();
                const int          originalIndex = listOfArgs.asInt(0);
                const int          adjustedIndex = indexAdjustments.removalAdjustment(originalIndex);
                _subCommands.push_back(
                    std::make_shared<UsdLayerEditor::RemoveSubPathCmd>(stage, layer, adjustedIndex));
            }
        }

        if (argParser.isFlagSet(kReplaceSubPathFlag)) {
            auto count = argParser.numberOfFlagUses(kReplaceSubPathFlag);
            for (unsigned i = 0; i < count; i++) {
                MArgList listOfArgs;
                argParser.getFlagArgumentList(kReplaceSubPathFlag, i, listOfArgs);
                _subCommands.push_back(std::make_shared<UsdLayerEditor::ReplaceSubPathCmd>(
                    layer, listOfArgs.asString(0).asUTF8(), listOfArgs.asString(1).asUTF8()));
            }
        }

        if (argParser.isFlagSet(kMoveSubPathFlag)) {
            MString subPath;
            argParser.getFlagArgument(kMoveSubPathFlag, 0, subPath);
            MString newParentLayerStr;
            argParser.getFlagArgument(kMoveSubPathFlag, 1, newParentLayerStr);
            int originalIndex { 0 };
            argParser.getFlagArgument(kMoveSubPathFlag, 2, originalIndex);
            const int adjustedIndex = indexAdjustments.removalAdjustment(originalIndex);

            SdfLayerHandle newParentLayerH;
            if (layer->GetIdentifier() == newParentLayerStr.asUTF8()) {
                newParentLayerH = layer;
            } else {
                newParentLayerH = SdfLayer::Find(newParentLayerStr.asUTF8());
                if (!newParentLayerH) {
                    displayError(MString("Layer not found: ") + newParentLayerStr);
                    return MS::kInvalidParameter;
                }
            }
            _subCommands.push_back(std::make_shared<UsdLayerEditor::MoveSubPathCmd>(
                layer, newParentLayerH, subPath.asUTF8(), adjustedIndex));
        }

        if (argParser.isFlagSet(kDiscardEditsFlag)) {
            _subCommands.push_back(std::make_shared<UsdLayerEditor::DiscardEditCmd>(layer));
        }

        if (argParser.isFlagSet(kClearLayerFlag)) {
            _subCommands.push_back(std::make_shared<UsdLayerEditor::ClearLayerCmd>(layer));
        }

        if (argParser.isFlagSet(kFlattenLayerFlag)) {
            _subCommands.push_back(std::make_shared<UsdLayerEditor::FlattenLayerCmd>(layer));
        }

        if (argParser.isFlagSet(kAddAnonSublayerFlag)) {
            auto count = argParser.numberOfFlagUses(kAddAnonSublayerFlag);
            for (unsigned i = 0; i < count; i++) {
                MArgList listOfArgs;
                argParser.getFlagArgumentList(kAddAnonSublayerFlag, i, listOfArgs);
                auto cmd = std::make_shared<UsdLayerEditor::AddAnonSubLayerCmd>(
                    UsdStageRefPtr {}, layer);
                cmd->_anonName = listOfArgs.asString(0).asUTF8();
                _subCommands.push_back(std::move(cmd));
            }
        }

        if (argParser.isFlagSet(kMuteLayerFlag)) {
            bool muteIt = true;
            argParser.getFlagArgument(kMuteLayerFlag, 0, muteIt);
            MString proxyShapeName;
            argParser.getFlagArgument(kMuteLayerFlag, 1, proxyShapeName);
            auto prim = UsdMayaQuery::GetPrim(proxyShapeName.asChar());
            if (prim == UsdPrim()) {
                displayError(
                    MString("Invalid proxy shape \"") + proxyShapeName.asChar() + "\"");
                return MS::kInvalidParameter;
            }
            UsdStageRefPtr stage = prim.GetStage();
            _subCommands.push_back(
                std::make_shared<UsdLayerEditor::MuteLayerCmd>(stage, layer, muteIt));
        }

        if (argParser.isFlagSet(kLockLayerFlag)) {
            int lockValue = 0;
            argParser.getFlagArgument(kLockLayerFlag, 0, lockValue);
            bool includeSublayers = false;
            argParser.getFlagArgument(kLockLayerFlag, 1, includeSublayers);
            MString proxyShapeName;
            argParser.getFlagArgument(kLockLayerFlag, 2, proxyShapeName);
            auto prim = UsdMayaQuery::GetPrim(proxyShapeName.asChar());
            if (prim == UsdPrim()) {
                displayError(
                    MString("Invalid proxy shape \"") + proxyShapeName.asChar() + "\"");
                return MS::kInvalidParameter;
            }
            UsdStageRefPtr stage = prim.GetStage();
            UsdLayerEditor::LayerLockType lockType;
            switch (lockValue) {
            case 1:  lockType = UsdLayerEditor::LayerLock_Locked;        break;
            case 2:  lockType = UsdLayerEditor::LayerLock_SystemLocked;  break;
            default: lockType = UsdLayerEditor::LayerLock_Unlocked;      break;
            }
            _subCommands.push_back(std::make_shared<UsdLayerEditor::LockLayerCmd>(
                stage, layer, lockType, includeSublayers, skipSystemLockedLayers));
        }

        if (argParser.isFlagSet(kRefreshSystemLockFlag)) {
            MString proxyShapeName;
            argParser.getFlagArgument(kRefreshSystemLockFlag, 0, proxyShapeName);
            bool refreshSubLayers = true;
            argParser.getFlagArgument(kRefreshSystemLockFlag, 1, refreshSubLayers);
            auto prim = UsdMayaQuery::GetPrim(proxyShapeName.asChar());
            if (prim == UsdPrim()) {
                displayError(
                    MString("Invalid proxy shape \"") + proxyShapeName.asChar() + "\"");
                return MS::kInvalidParameter;
            }
            UsdStageRefPtr stage = prim.GetStage();
            auto cmd = std::make_shared<UsdLayerEditor::RefreshSystemLockLayerCmd>(
                stage, layer, refreshSubLayers);
            cmd->addCallbackContext(
                "proxyShapePath", PXR_NS::VtValue(std::string(proxyShapeName.asChar())));
            _subCommands.push_back(std::move(cmd));
        }

        if (argParser.isFlagSet(kStitchLayersFlag)) {
            std::vector<std::string> layerIdentifiers;
            const auto               layerCount = argParser.numberOfFlagUses(kStitchLayersFlag);
            MString                  proxyShapeName;
            for (unsigned i = 0; i < layerCount; ++i) {
                MArgList listOfArgs;
                argParser.getFlagArgumentList(kStitchLayersFlag, i, listOfArgs);
                if (i == 0)
                    proxyShapeName = listOfArgs.asString(0);
                layerIdentifiers.push_back(listOfArgs.asString(1).asChar());
            }
            const UsdPrim prim = UsdMayaQuery::GetPrim(proxyShapeName.asChar());
            if (prim == UsdPrim()) {
                displayError(
                    MString("Invalid proxy shape \"") + proxyShapeName.asChar() + "\"");
                return MS::kInvalidParameter;
            }
            UsdStageRefPtr stage = prim.GetStage();
            _subCommands.push_back(
                std::make_shared<UsdLayerEditor::StitchLayersCmd>(stage, layerIdentifiers));
        }
    }
```

Key changes from the old block:
- `SdfLayer::FindOrOpen(_layerIdentifier)` moves here from `redoIt()` — if the layer doesn't exist at parse time, return `kInvalidParameter` immediately.
- `Impl::IndexAdjustments` → `IndexAdjustments` (now in the anon namespace).
- Every `Impl::Foo` construction replaced with the corresponding `UsdLayerEditor::FooCmd` construction, with layer/stage baked in at construction time rather than passed later via `doIt(layer)`.
- `MoveSubPath`: new-parent layer lookup and error check moved from `doIt()` into `parseArgs()`.
- `LockLayer`: uses `UsdLayerEditor::LayerLockType` directly (same underlying values as `MayaUsd::LayerLockType`; no cast needed since we never store the Maya type).

---

### Task 5: Rewrite `redoIt()` and `undoIt()`

**Files:**
- Modify: `lib/mayaUsd/commands/layerEditorCommand.cpp`

- [ ] **Step 1: Replace `redoIt()`**

Find the current `redoIt()`:
```cpp
MStatus LayerEditorCommand::redoIt()
{

    auto layer = SdfLayer::FindOrOpen(_layerIdentifier);
    if (!layer) {
        return MS::kInvalidParameter;
    }

    for (auto it = _subCommands.begin(); it != _subCommands.end(); ++it) {
        if (!(*it)->doIt(layer)) {
            return MS::kFailure;
        }
        const auto& result = (*it)->_cmdResult;
        if (!result.empty()) {
            appendToResult(result.c_str());
        }
    }

    return MS::kSuccess;
}
```

Replace with:
```cpp
MStatus LayerEditorCommand::redoIt()
{
    for (auto& cmd : _subCommands) {
        cmd->redo();
        // AddAnonSubLayerCmd is the only command that returns a result
        // (the new anonymous layer identifier).
        if (auto* anon = dynamic_cast<UsdLayerEditor::AddAnonSubLayerCmd*>(cmd.get()))
            appendToResult(anon->addedLayer().c_str());
    }
    return MS::kSuccess;
}
```

- [ ] **Step 2: Replace `undoIt()`**

Find the current `undoIt()`:
```cpp
MStatus LayerEditorCommand::undoIt()
{

    auto layer = SdfLayer::FindOrOpen(_layerIdentifier);
    if (!layer) {
        return MS::kInvalidParameter;
    }

    // clang-format off
    for (auto it = _subCommands.rbegin(); it != _subCommands.rend(); ++it) {
        if (!(*it)->undoIt(layer)) {
            return MS::kFailure;
        }
    }

    // clang-format on
    return MS::kSuccess;
}
```

Replace with:
```cpp
MStatus LayerEditorCommand::undoIt()
{
    for (auto it = _subCommands.rbegin(); it != _subCommands.rend(); ++it)
        (*it)->undo();
    return MS::kSuccess;
}
```

---

### Task 6: Build, test, and commit

**Files:** none (verification only)

- [ ] **Step 1: Build**

Run the host relay build command from the repo root (`/d/repos/agent_repos/ecg-maya-usd/maya-usd`):

```bash
# Pre-flight
if [ -f _host_command/result.json ] && [ ! -f _host_command/in-progress.json ]; then rm _host_command/result.json; fi

for _attempt in 1 2 3; do
  echo '{"repo":"ecg-maya-usd","command":"build"}' > _host_command/request.json
  _wait_start=$(date +%s)
  until [ -f _host_command/result.json ]; do
    sleep 2
    _age=$(( $(date +%s) - _wait_start ))
    [ $_age -gt 300 ] && { echo "ERROR: timeout" >&2; exit 1; }
  done
  python3 -c "
import json
with open('_host_command/result.json','rb') as f:
    r = json.loads(f.read().lstrip(b'\xef\xbb\xbf').decode('utf-8'))
print('EXIT:', r['exit_code'])
stdout = r.get('stdout') or ''
stderr = r.get('stderr') or ''
print((stdout+stderr)[-4000:])
"
  _exit=$(python3 -c "
import json
with open('_host_command/result.json','rb') as f:
    r = json.loads(f.read().lstrip(b'\xef\xbb\xbf').decode('utf-8'))
print(r['exit_code'])
")
  rm _host_command/result.json; rm -f _host_command/in-progress.json
  [ "$_exit" = "0" ] && break
  [ $_attempt -lt 3 ] && echo "Attempt $_attempt failed, retrying..." && sleep 5
done
```

Expected: `Success MayaUsd build and install!`, exit 0.

If the build fails, read the error lines from the output (lines containing `error C` on Windows). Common issues:
- Missing include for a type used in the new `parseArgs()` → add it under the `#if defined(MAYAUSD_USE_SHARED_LAYER_EDITOR)` block
- `UsdLayerEditor::LayerLock_SystemLocked` not found → check the exact value name in `lib/usdUfe/usd-layer-editor/lib/layerLocking.h`

- [ ] **Step 2: Run shared library tests**

```bash
if [ -f _host_command/result.json ] && [ ! -f _host_command/in-progress.json ]; then rm _host_command/result.json; fi

for _attempt in 1 2 3; do
  echo '{"repo":"ecg-maya-usd","command":"test","args":["UsdLayerEditorNewTests"]}' > _host_command/request.json
  _wait_start=$(date +%s)
  until [ -f _host_command/result.json ]; do
    sleep 2
    [ $(( $(date +%s) - _wait_start )) -gt 300 ] && { echo "timeout" >&2; exit 1; }
  done
  python3 -c "
import json
with open('_host_command/result.json','rb') as f:
    r = json.loads(f.read().lstrip(b'\xef\xbb\xbf').decode('utf-8'))
print('EXIT:', r['exit_code'])
stdout = r.get('stdout') or ''
stderr = r.get('stderr') or ''
print((stdout+stderr)[-3000:])
"
  _exit=$(python3 -c "
import json
with open('_host_command/result.json','rb') as f:
    r = json.loads(f.read().lstrip(b'\xef\xbb\xbf').decode('utf-8'))
print(r['exit_code'])
")
  rm _host_command/result.json; rm -f _host_command/in-progress.json
  [ "$_exit" = "0" ] && break
  [ $_attempt -lt 3 ] && echo "Attempt $_attempt failed, retrying..." && sleep 5
done
```

Expected: all 214 tests pass, exit 0.

- [ ] **Step 3: Run Maya integration tests**

```bash
if [ -f _host_command/result.json ] && [ ! -f _host_command/in-progress.json ]; then rm _host_command/result.json; fi

for _attempt in 1 2 3; do
  echo '{"repo":"ecg-maya-usd","command":"test","args":["testMayaUsdLayerEditorCommands"]}' > _host_command/request.json
  _wait_start=$(date +%s)
  until [ -f _host_command/result.json ]; do
    sleep 2
    [ $(( $(date +%s) - _wait_start )) -gt 300 ] && { echo "timeout" >&2; exit 1; }
  done
  python3 -c "
import json
with open('_host_command/result.json','rb') as f:
    r = json.loads(f.read().lstrip(b'\xef\xbb\xbf').decode('utf-8'))
print('EXIT:', r['exit_code'])
stdout = r.get('stdout') or ''
stderr = r.get('stderr') or ''
print((stdout+stderr)[-3000:])
"
  _exit=$(python3 -c "
import json
with open('_host_command/result.json','rb') as f:
    r = json.loads(f.read().lstrip(b'\xef\xbb\xbf').decode('utf-8'))
print(r['exit_code'])
")
  rm _host_command/result.json; rm -f _host_command/in-progress.json
  [ "$_exit" = "0" ] && break
  [ $_attempt -lt 3 ] && echo "Attempt $_attempt failed, retrying..." && sleep 5
done
```

Expected: `Ran 30 tests … OK`, exit 0.

- [ ] **Step 4: Commit**

```bash
git add lib/mayaUsd/commands/layerEditorCommand.h \
        lib/mayaUsd/commands/layerEditorCommand.cpp
git commit -m "$(cat <<'EOF'
refactor: replace _subCommands with direct UFE commands

Removes the entire Impl anonymous namespace (~750 lines of thin wrappers).
Layer and stage are now retrieved in parseArgs() and baked into UFE command
constructors directly. redoIt()/undoIt() become trivial loops over
Ufe::UndoableCommand::redo()/undo().

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>
EOF
)"
```
