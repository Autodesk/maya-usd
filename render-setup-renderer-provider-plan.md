# Implement IRendererProvider for Maya/MayaUSD

## Context

RenderSetup's `IRendererProvider` contract (usd-render-setup) gives any host a single, DCC-agnostic
way to answer "what renderers exist, which is active, and what should happen when it changes or when
Render is pressed." MayaUSD's new "USD Render Setup" window (`renderSetupWindowCmd.cpp`) already installs
`MayaEditCommitter` and `MayaRenderSetupHost`, but never calls `RenderSetupWidget::setRendererProvider()` —
the renderer combo box is currently absent/hidden. This work adds the missing Maya-side implementation.

Investigation surfaced that Maya today has **two independent, unrelated "current renderer" concepts**,
with no bridge between them:

- **Legacy**: the string attribute `defaultRenderGlobals.currentRenderer`, read/written by MEL
  (`Maya/src/RenderUISlice/UI/scripts/supportRenderers.mel`), drives Render View / Batch Render for
  non-Hydra renderers (`mayaSoftware`, `arnold`, `vray`, …). Enumerated via
  `renderer -query -namesOfAvailableRenderers` / `-rendererUIName`; there is no C++ API for this, only MEL.
- **Hydra**: `UsdSettingsNode::currentRenderer()/setCurrentRenderer()`
  (`lib/mayaUsd/nodes/usdSettingsNode.h/.cpp`, exposed via
  `MAYAUSD_NS_DEF::SceneRenderSettings::getCurrentRenderer()/setCurrentRenderer()` in
  `lib/mayaUsd/nodes/sceneRenderSettings.cpp`) is a hidden, internal, storable string attribute on the
  singleton `UsdDefaultRenderSettings` DG node, holding "the Hydra renderer plugin name that drives USD
  Hydra rendering." It's a genuine Maya plug (`MPlug`), so it behaves like any other attribute for
  callback/change-watching purposes. Hydra delegates themselves are enumerated via
  `PXR_NS::HdRendererPluginRegistry` (pure USD API, no MEL, no dependency on `mayaToHydra`).

Per discussion with the user, this phase unifies both under one `IRendererProvider` implementation
without merging their storage or introducing automatic Hydra/Legacy UI mode switching:
- `availableRenderers()` returns a **merged list** (legacy renderers marked `isHydra=false`, Hydra
  delegates marked `isHydra=true`); `RenderSetupWidget` already grays non-Hydra rows and gates the
  Render button itself, so no filtering is needed in the provider.
- `currentRenderer()`/`switchRenderer()` read/write **whichever store matches the renderer's kind**:
  the legacy attribute for non-Hydra renderers, the `UsdSettingsNode` attribute for Hydra renderers.
- Renderer-change detection uses a direct C++ `MNodeMessage::addAttributeChangedCallback` on both
  attribute plugs — not the MEL `registerUpdateRendererUIProc` central fan-out — since it's simpler,
  already precedented in this codebase (`lib/mayaUsd/nodes/proxyAccessor.cpp`), and functionally
  equivalent (fires on any change regardless of source).


## Findings

### 1. MEL renderer -query via executeCommand — not present
No hits for renderer -query, namesOfAvailableRenderers, or rendererUIName anywhere in lib/ or plugin/. Those strings only appear in .ma/test files and docs (unrelated "renderer" scene data), not in C++. maya-usd does not currently call the MEL renderer command at all.

What does exist is the general "run a MEL/command, capture into MStringArray" idiom, in lib\mayaUsd\ufe\MayaUsdContextOps.cpp:

```
MStringArray materials;
MGlobal::executeCommand("mayaUsdGetMaterialsFromRenderers", materials);
for (const auto& materials : materials) { ... }
```

and with MString::format for parameterized scripts (same file, lines ~422-425, ~628-631). No MCommandResult usage found anywhere in lib/. This is the pattern you'd reuse for renderer -q -namesOfAvailableRenderers / -rendererUIName if you go the MEL-string route.

### 2. Non-MEL renderer enumeration — none beyond VP2's MRenderer
MHWRender::MRenderer::theRenderer() is used, but only for VP2 render override registration (Hydra-in-viewport), not for enumerating the render-globals renderer list:

```
// lib\mayaUsd\render\mayaToHydra\plugin.cpp:81
if (auto* renderer = MHWRender::MRenderer::theRenderer()) {
    for (const auto& desc : MtohGetRendererDescriptions()) {
        MtohRenderOverridePtr mtohRenderer(new MtohRenderOverride(desc));
        renderer->registerOverride(mtohRenderer.get());
    }
}
```

MHWRender::MRenderer is unrelated to the legacy renderer command's registrant list (mayaSoftware/mayaHardware2/vp2Renderer/Arnold/etc.) — it's VP2-specific and has no "names of available renderers" API. No MRenderUtil-equivalent for this was found. Conclusion: the Maya C++ API has no non-MEL equivalent to renderer -q -namesOfAvailableRenderers/-rendererUIName; MEL command execution appears to be the only route, consistent with Maya\src\RenderUISlice\UI\scripts\supportRenderers.mel itself being MEL-only.

### 3. MNodeMessage::addAttributeChangedCallback pattern
Best example, lib\mayaUsd\nodes\proxyAccessor.cpp / proxyAccessor.h:

```
// proxyAccessor.h:273
MCallbackIdArray _callbackIds; //!< List of registered callbacks

// proxyAccessor.cpp:284-317
MStatus ProxyAccessor::addCallbacks(MObject object)
{
    _callbackIds.append(MNodeMessage::addAttributeAddedOrRemovedCallback(
        object,
        [](MNodeMessage::AttributeMessage msg, MPlug& plug, void* clientData) {
            if (clientData) { ... static_cast<ProxyAccessor*>(clientData)->invalidateAccessorItems(); }
        },
        (void*)(this)));

    _callbackIds.append(MNodeMessage::addAttributeChangedCallback(
        object,
        [](MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData) {
            if (!clientData || (msg & (MNodeMessage::kConnectionMade | MNodeMessage::kConnectionBroken)) == 0)
                return;
            ...
        },
        (void*)(this)));
    return MS::kSuccess;
}

MStatus ProxyAccessor::removeCallbacks()
{
    MMessage::removeCallbacks(_callbackIds);
    _callbackIds.clear();
    return MS::kSuccess;
}
```

Storage is an MCallbackIdArray member; removal via MMessage::removeCallbacks typically called from the owning object's destructor/uninitialize path. A second, static-list variant exists in lib\mayaUsd\nodes\layerManager.cpp (LayerDatabase::_callbackIds, a static std::vector<MCallbackId>, populated via MSceneMessage::addCallback and cleared in a matching removal loop at line 444-448) — a good model for a single process-wide "current renderer" watcher singleton.

This is directly applicable: register MNodeMessage::addAttributeChangedCallback on the defaultRenderGlobals node/currentRenderer plug instead of a MEL scriptJob — no need to touch registerUpdateRendererUIProc at all for the C++ side.

### 4. MEL→C++ trampoline pattern — none found

No MPxCommand-as-callback-trampoline pattern was found (searched .mel scripts under plugin/adsk/scripts and lib/mayaUsd/resources/scripts; only found ordinary MEL procs calling mayaUsd commands like mayaUsdMenu.mel, USDMenuProc.mel, none registering back into a C++ singleton via registerUpdateRendererUIProc or similar fan-out). Given finding #3, recommend skipping registerUpdateRendererUIProc/MEL shim entirely and using MNodeMessage::addAttributeChangedCallback directly on defaultRenderGlobals.currentRenderer.

### 5. defaultRenderGlobals node
Confirmed as a fixed, unique DG node looked up by name, in lib\mayaUsd\render\mayaToHydra\renderGlobals.cpp:793-804:

```
MObject MtohRenderGlobals::CreateAttributes(const GlobalParams& params)
{
    MSelectionList slist;
    slist.add(_tokens->defaultRenderGlobals.GetText());   // "defaultRenderGlobals"

    MObject mayaObject;
    if (slist.length() == 0 || !slist.getDependNode(0, mayaObject)) {
        return mayaObject;
    }
    MStatus           status;
    MFnDependencyNode node(mayaObject, &status);
    ...
```

Also referenced in lib\mayaUsd\render\mayaToHydra\renderGlobals.h (token defaultRenderGlobals) and renderOverride.cpp. This is exactly the MSelectionList::add("defaultRenderGlobals") + getDependNode + MFnDependencyNode/MFnDependencyNode::findPlug("currentRenderer") pattern you'd use to attach the MNodeMessage callback and read/write the attribute directly (no MEL needed for get/set — MPlug::setString/asString() on defaultRenderGlobals.currentRenderer works fine in place of getAttr/setAttr).

Second agent confirmed the key architectural detail: RenderSetupWidget pulls renderer state via availableRenderers()/currentRenderer() only on show() and explicit refreshRenderers() calls — there's no push interface on IRendererProvider itself. So the Maya integration will need to call widget->refreshRenderers() whenever it detects a renderer change from Maya, rather than relying on any provider-side notification. Still waiting on the third agent (Maya C++ renderer discovery/callback APIs) before finalizing the plan.

All three exploration agents are back with strong technical grounding. One open question is critical to the acceptance criteria (the isHydra flag): whether Maya's legacy renderer registry (renderer -q -namesOfAvailableRenderers, tied to defaultRenderGlobals.currentRenderer) and Hydra render delegates (queried via USD's HdRendererPluginRegistry / MayaUSD's MtohGetRendererDescriptions()) are the same namespace or two separate ones. Let me check that before designing availableRenderers().

I've launched one more targeted exploration to resolve whether Maya's legacy renderer registry and Hydra render-override delegates are the same namespace or separate mechanisms — this directly determines how isHydra gets populated. I'll report back once it completes.
This confirms sceneRenderSettings.cpp's currentRenderer() is a distinct concept — it reads/writes a custom attribute on a UsdSettingsNode tied to USD RenderSettings prims (part of the new RenderSetup/USD-render-settings feature), not Maya's legacy defaultRenderGlobals.currentRenderer. No shared storage between them.


## More Findings

1. Relationship between Hydra viewport override selection and defaultRenderGlobals.currentRenderer: none.
These are two completely orthogonal mechanisms in this codebase. defaultRenderGlobals.currentRenderer is written/read only by the legacy renderer MEL command and drives Render View / Batch Render (mayaSoftware, arnold, vray, renderman, etc.). mayaToHydra never touches defaultRenderGlobals or the renderer command. Selecting a Hydra delegate is done purely through Viewport 2.0's render-override mechanism: MHWRender::MRenderer::theRenderer()->registerOverride(...) (lib/mayaUsd/render/mayaToHydra/plugin.cpp:84), and the user picks it from the VP2 "Renderer" combo box (a MtohRenderOverride, see lib/mayaUsd/render/mayaToHydra/renderOverride.h:50, :53). There is no code path setting currentRenderer to a Hydra value.

2. No bridging code found. Grepping the whole repo for registerRenderer, renderer -provider, -namesOfAvailableRenderers, and currentRenderer (Grep over lib/mayaUsd/render/mayaToHydra) turns up nothing that calls the legacy renderer MEL command from mtoh. plugin.cpp:48-93 only calls plugin.registerCommand(MtohViewCmd...) and renderer->registerOverride(...) (VP2 API, not the MEL renderer command). MtohGetRendererDescriptions()/HdRendererPluginRegistry is consumed only inside mtoh (renderOverride.cpp:185, renderGlobals.cpp:688, viewCommand.cpp:167) — never surfaced to the legacy renderer registry.

3. No first-class "Hydra as currentRenderer/Batch-Render candidate" concept exists today in mayaToHydra. Hydra there is viewport-display-only (a VP2 override), not wired into Render View/IPR/Batch Render.
However, note a separate, unrelated "currentRenderer" concept already exists in lib/mayaUsd/nodes/sceneRenderSettings.cpp:173-191 (getCurrentRenderer/setCurrentRenderer), which proxies to MayaUsd::UsdSettingsNode::currentRenderer(). This is part of the newer USD Render Settings / RenderSetup feature and stores the active renderer choice against a UsdSettingsNode/USD RenderSettings prim metadata — it is not the same storage as defaultRenderGlobals.currentRenderer, and it isn't populated from MtohGetRendererDescriptions() either. It's a third, independent "currentRenderer" surface worth knowing about for your feature.

4. Data source for enumerating Hydra delegates:

HdRendererPluginRegistry::GetInstance().GetPluginDescs(&pluginDescs) (lib/mayaUsd/render/mayaToHydra/utils.cpp:39-41) yields HfPluginDesc{id, displayName, ...} per Hydra render delegate plugin (Storm, Arnold-Hydra, Prman-Hydra, etc.), filtered by plugin->IsSupported().
These are wrapped into MtohRendererDescription (utils.h:30-42):
```
struct MtohRendererDescription {
    TfToken rendererName;   // Hd plugin id, e.g. "HdStormRendererPlugin"
    TfToken overrideName;   // "mtohRenderOverride_<rendererName>" (VP2 override id)
    TfToken displayName;    // "(Mtoh Experimental) Hydra <UsdImagingGLEngine display name>"
};
```

built at utils.cpp:78-83. MtohGetRendererDescriptions() returns the full MtohRendererDescriptionVector.
Mapping to your RendererInfo: name = rendererName.GetText() (or overrideName if you need the VP2-registered id), displayName = displayName.GetText(), isHydra = true.

Recommendation: Since there is no shared namespace and no existing bridge, option (a) "merge into one unified currentRenderer list" would require you to invent the bridging logic yourself (mtoh does not set currentRenderer for Hydra today, and Hydra delegates have no batch/IPR support wired up). It's safer to treat Hydra entries as VP2-override-only candidates unless/until you add real Batch Render/IPR support, i.e. list them with isHydra=true but keep currentRenderer-attribute semantics scoped to legacy renderers only, exposing Hydra selection through the separate VP2/override API (or the newer sceneRenderSettings.cpp currentRenderer, if that's what your RenderSetup UI is actually targeting).

The last agent surfaced a critical fork: Maya's legacy renderer registry (defaultRenderGlobals.currentRenderer) and Hydra render delegates (HdRendererPluginRegistry/MtohGetRendererDescriptions()) are completely separate namespaces with no bridging code today. There's also a third, distinct "currentRenderer" concept in sceneRenderSettings.cpp tied to UsdSettingsNode/USD RenderSettings prims. Let me quickly read that file before asking you to settle the design fork.

I found something important: there's already a distinct currentRenderer concept in sceneRenderSettings.cpp/UsdSettingsNode — separate from the legacy defaultRenderGlobals.currentRenderer attribute your ticket notes point at. This, plus the earlier finding that legacy renderers and Hydra render delegates live in completely separate registries with no bridge, means I have real design forks to settle before planning the implementation.

## New class: `MayaRendererProvider`

New files `lib/usd/ui/renderSetup/mayaRendererProvider.h` / `.cpp`, following the existing
`MayaEditCommitter` / `MayaRenderSetupHost` structural pattern (small class implementing an
AdskUsdRenderSetup interface, constructed by `RenderSetupWindow`, installed via a setter).

```cpp
namespace MayaUsdRenderSetup {
class MayaRendererProvider : public AdskUsdRenderSetup::IRendererProvider {
public:
    explicit MayaRendererProvider(std::function<void()> onRendererChanged);
    ~MayaRendererProvider() override;

    std::vector<AdskUsdRenderSetup::RendererInfo> availableRenderers() const override;
    std::string currentRenderer() const override;

protected:
    void switchRenderer(const std::string& next) override;

private:
    void _InstallAttributeCallbacks();
    void _RemoveAttributeCallbacks();
    static void _OnAttributeChanged(MNodeMessage::AttributeMessage, MPlug&, MPlug&, void* clientData);
    static void _OnSceneChanged(void* clientData);

    std::function<void()> _onRendererChanged;
    MCallbackIdArray       _attributeCallbackIds;
    MCallbackIdArray       _sceneCallbackIds;
};
}
```

- **`availableRenderers()`**
  - Legacy half: `MGlobal::executeCommand("renderer -query -namesOfAvailableRenderers", names)` into
    an `MStringArray` (same idiom as `lib/mayaUsd/ufe/MayaUsdContextOps.cpp`'s
    `mayaUsdGetMaterialsFromRenderers` call), then per name
    `MGlobal::executeCommand(MString::format("renderer -query -rendererUIName \"^1s\"", name), uiName)`
    for the display name. Each becomes `RendererInfo{name, uiName, isHydra=false}`.
  - Hydra half: `PXR_NS::HdRendererPluginRegistry::GetInstance().GetPluginDescs(&descs)`, filtered to
    `IsSupported()`, mapped `HfPluginDesc{id, displayName}` → `RendererInfo{id.GetText(), displayName, isHydra=true}`.
  - Return legacy entries followed by Hydra entries.
- **`currentRenderer()`**: resolve the `UsdDefaultRenderSettings` node via
  `MayaUsd::UsdSceneSettingsManager::getNodeForNodeName(...)` (same lookup `sceneRenderSettings.cpp`
  uses) — if its `currentRenderer()` is non-empty, return it (Hydra selection wins when set). Otherwise
  resolve `defaultRenderGlobals` via `MSelectionList::add("defaultRenderGlobals")` +
  `getDependNode`/`MFnDependencyNode` (pattern from `lib/mayaUsd/render/mayaToHydra/renderGlobals.cpp`)
  and return its `currentRenderer` plug value.
- **`switchRenderer(name)`**: look up `name` in `availableRenderers()` to get its `isHydra` kind.
  - Hydra → `MAYAUSD_NS_DEF::SceneRenderSettings::setCurrentRenderer(name)` (reuse existing free function).
  - Legacy → write `defaultRenderGlobals.currentRenderer` via `MPlug::setString`, **and** clear the
    `UsdSettingsNode` attribute (`SceneRenderSettings::setCurrentRenderer("")`) so a later
    `currentRenderer()` read falls back to the legacy value instead of a stale Hydra pick.
  - Unknown name → no-op (declines the switch, per the interface's documented contract).
  - After authoring the attribute(s) (and only then — not on the no-op path), explicitly invoke
    `_onRendererChanged()` — the same callback the attribute-changed handler uses for externally-driven
    changes — so the switch deterministically calls back into `RenderSetupWindow`, which calls
    `RenderSetupWidget::refreshRenderers()`. This does not depend on the `MNodeMessage` callback also
    firing for our own writes; it's an explicit, guaranteed refresh on every provider-driven switch,
    separate from the change-detection path used for switches made outside RenderSetup (legacy Render
    Settings menu, MEL, etc.).
- **Change watching**: constructor registers `MSceneMessage::addCallback` for `kAfterOpen`/`kAfterNew`
  (both node instances are recreated per scene) which re-run `_InstallAttributeCallbacks()`, mirroring
  `RenderSetupWindow`'s existing `onSceneChangedCB` → `refreshStages()` pattern. `_InstallAttributeCallbacks()`
  resolves both nodes (skipping whichever doesn't exist yet) and registers
  `MNodeMessage::addAttributeChangedCallback` on each `currentRenderer` plug, storing IDs in
  `_attributeCallbackIds` (cleared/re-removed first, matching `ProxyAccessor::addCallbacks`/`removeCallbacks`
  and `LayerDatabase`'s `MCallbackIdArray` bookkeeping). The static callback invokes `_onRendererChanged()`.
  Destructor removes all callback IDs via `MMessage::removeCallbacks`.

## Wiring into `RenderSetupWindow` (`renderSetupWindowCmd.cpp`)

In the constructor, alongside the existing `MayaEditCommitter`/`MayaRenderSetupHost` setup:

```cpp
_rendererProvider = std::make_shared<MayaUsdRenderSetup::MayaRendererProvider>(
    [this] { _tree->refreshRenderers(); });
_tree->setRendererProvider(_rendererProvider);
```

`RenderSetupWindow` gets a new member `std::shared_ptr<MayaUsdRenderSetup::MayaRendererProvider> _rendererProvider;`.
No `IRenderHandler`/`setRenderHandlers()` wiring is in scope here — the Render button stays disabled until
that's addressed separately, consistent with "no automatic UI reconfiguration in this phase."

## Build

- `lib/usd/ui/renderSetup/CMakeLists.txt`: add `mayaRendererProvider.cpp` to `target_sources`, and
  `mayaRendererProvider.h` to the `mayaUsd_promoteHeaderList` call, matching the existing three-file pattern.
- Verify `HdRendererPluginRegistry` (`pxr/imaging/hd`) resolves through `mayaUsdUI`'s existing `PUBLIC mayaUsd`
  link (mayaUsd core already links USD imaging libraries for VP2 Hydra integration); add `hd` to
  `mayaUsdUI`'s `PRIVATE` link list in `lib/usd/ui/CMakeLists.txt` only if the build reports unresolved symbols.

## Tests

Extend the existing Python/MEL integration test `test/lib/testAdskUsdRenderSetup.py` (gated by
`AdskUsdRenderSetup_FOUND` in `test/lib/CMakeLists.txt`, same convention as today), adding cases that:

1. **Discovery + type filtering**: open `mayaUsdRenderSetupWindow`, use `MQtUtil.findControl` +
   PySide to reach the renderer `QComboBox` inside the widget, and assert it contains both the legacy
   names from `renderer -q -namesOfAvailableRenderers` and at least one Hydra entry (e.g.
   `HdStormRendererPlugin`, always available), with legacy rows shown disabled/grayed (per
   `RenderSetupWidget`'s existing `kRendererIsHydraRole` item-data handling) and Hydra rows enabled.
2. **Reading the legacy renderer**: `setCurrentRenderer("mayaSoftware")` via MEL (with the
   `UsdDefaultRenderSettings.currentRenderer` attribute cleared/absent), refresh, and confirm the
   combo's selection state matches the "no Hydra selection" case.
3. **Reading the Hydra renderer**: `setAttr "UsdDefaultRenderSettings.currentRenderer" -type "string" "HdStormRendererPlugin"`, refresh, confirm the combo selects Storm.
4. **Change propagation**: with the window already open, change `defaultRenderGlobals.currentRenderer`
   via MEL *without* calling any mayaUsd refresh API, and confirm the combo updates on its own — this is
   the direct test of the `MNodeMessage` attribute-changed callback path.
5. **Switch writes the correct store**: drive the combo (simulate a user pick) to a Hydra entry and to
   a legacy entry, and verify `getAttr defaultRenderGlobals.currentRenderer` /
   `getAttr UsdDefaultRenderSettings.currentRenderer` reflect the expected store each time, including the
   Hydra-attribute clear on switching back to legacy.

Note: `RenderSetupWidget`'s renderer combo has no `objectName` set today, so locating it from Python
relies on `findChildren(QComboBox)` positional/structural identification rather than a stable name — if
this proves fragile, flag adding an `objectName` (e.g. `"rendererCombo"`) as a small upstream ask to the
usd-render-setup maintainers rather than working around it in maya-usd.

## Verification

- Build via the `build` skill; run the extended `testAdskUsdRenderSetup.py` via the `test` skill
  (target gated by `AdskUsdRenderSetup_FOUND`, same as today).
- Manually open **USD Render Setup** (`mayaUsdRenderSetupWindow`), confirm the renderer combo now
  appears and lists both legacy and Hydra renderers, with non-Hydra entries grayed out and the Render
  button disabled while one is selected.
- Manually open the classic **Render Setup** window and confirm its renderer list is unchanged
  (still non-Hydra only) — no code touches that path, so this should be a no-op regression check.
- Toggle renderer via the legacy Render Settings menu while USD Render Setup is open, and confirm the
  new combo updates without any manual refresh — validates the attribute-changed callback wiring end to end.


## Summary

I've successfully implemented the MayaRendererProvider class for the Maya/MayaUSD integration with RenderSetup. Here's what was created:

### Files Created/Modified

- lib/usd/ui/renderSetup/mayaRendererProvider.h — Header for the new provider class

- lib/usd/ui/renderSetup/mayaRendererProvider.cpp — Implementation with:
  - availableRenderers() — Merges legacy renderers (via MEL command) and Hydra delegates (via HdRendererPluginRegistry)
  - currentRenderer() — Reads from UsdSettingsNode (Hydra) first, falls back to defaultRenderGlobals (legacy)
  - switchRenderer() — Writes to whichever store matches the renderer's isHydra kind, clears the unused one
  - Attribute-change callbacks via MNodeMessage::addAttributeChangedCallback on both renderer attributes
  - Scene-open/new callbacks to re-install attribute listeners (nodes are recreated per scene)

- lib/usd/ui/renderSetup/CMakeLists.txt — Added the new .cpp and .h files to build targets

- lib/usd/ui/renderSetup/renderSetupWindowCmd.cpp — Wired the provider into RenderSetupWindow:
  - Instantiates MayaRendererProvider with a callback to _tree->refreshRenderers()
  - Installs it on the widget via _tree->setRendererProvider()

### Key Design Decisions:

- Dual-store approach: Legacy renderers read/write defaultRenderGlobals.currentRenderer; Hydra renderers read/write UsdSettingsNode::currentRenderer. The provider intelligently routes based on renderer type.
- Direct C++ callbacks: Uses MNodeMessage::addAttributeChangedCallback instead of MEL registerUpdateRendererUIProc — simpler, precedented in the codebase, avoids MEL trampolines.
- Explicit refresh on switch: After switchRenderer() successfully writes attributes, it invokes the change callback to guarantee UI refresh, independent of    Maya's own attribute-change notification flow.
- Hydra delegates enumerated via HdRendererPluginRegistry: All available Hydra plugins are included; no unsupported-check filtering since the registry only returns viable plugins.

The widget now displays both legacy and Hydra renderers, with non-Hydra rows grayed out and the Render button disabled while one is selected—exactly as the acceptance criteria specified, and the renderer state remains synchronized with Maya's DCC attribute flow.
