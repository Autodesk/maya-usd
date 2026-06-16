---
name: port-maya-to-max
description: Looks at a PR(s) or commit sha1 from the maya-usd repo and ports the changes to 3dsmax-component-usd, keeping the logic but integrating it with 3dsMax. Use when user asks to port a maya feature or pull request to 3dsmax or if the user mentions "port from maya", "port-to-max", "max integration".
argument-hint: [pr-link-or-sha]
allowed-tools: bash, grep, find, read, gh pr:view, gh pr:diff  
---

Arguments: $pr-link-or-sha

# Port Maya to Max

## Instructions
1. If $pr-link-or-sha is a link to a PR, verify it is from `https://github.com/Autodesk/maya-usd`. Before making any network calls, check for a local maya-usd clone by searching common sibling directories (e.g. `../maya-usd`, `../../maya-usd`, or any path matching `**/maya-usd` near the current repo root). If a local clone exists, use `git -C <local-clone> fetch origin` then `git -C <local-clone> show` or `git -C <local-clone> diff` to read the changes without network calls. Only fall back to `gh pr view $pr-link-or-sha` and `gh pr diff $pr-link-or-sha` if no local clone is found. If $pr-link-or-sha is instead a commit sha (40 hex characters), first check whether the commit exists in a local maya-usd clone via `git -C <local-clone> show $pr-link-or-sha`; only fall back to WebFetch at `https://github.com/Autodesk/maya-usd/commit/$pr-link-or-sha.diff` if no local clone has the commit.
2. Read `.claude/context/maya-to-max-mapping.md` for architectural differences and key file locations.
3. Create a plan for implementing it in 3dsmax-component-usd. You may search through both the maya-usd repo and the 3dsmax-component-usd repo.
4. Implement the changes necessary in 3dsMax to replicate the Maya pull request. Mirror the Maya PR's design as closely as possible (class hierarchy, inheritance relationships, member visibility (public/protected/private), and use of shared utilities (e.g. UsdUndoableItem, UsdUndoBlock)). Only deviate when the 3dsMax architecture strictly requires it. If the Maya PR makes a base class member protected, do the same in 3dsMax. If the Maya PR uses a particular undo mechanism, use the equivalent in 3dsMax. Do not invent alternative designs. If in doubt, follow the Maya PR.
5. Apply the code changes.
6. Implement all of the integration tests from the Maya pull request into 3dsMax.
   - First, list every new test method added in the Maya PR diff (e.g. `testFoo`, `testBar`, …) and count them.
   - Then port each one, checking them off as you go.
   - Before moving on, verify your count matches. Do not proceed until every test from the PR has been ported.
7. Build the entire project, using `python build-scripts\build-solution.py`, ensuring that it builds without error. Build both Release and Hybrid.
8. Run the integration tests, ensuring no integration tests fail.
9. Clean up logs and run_tests.py file(s).

## Running Tests

Tests require launching 3dsMax. Create `run_tests.py` in the repo root:
Before writing the file, glob for `artifacts_*.xml` in the repo root and extract all year numbers. List them highest-first in max_candidates so the newest installed version is preferred.

```python
import subprocess, sys, os, tempfile

repo_root = os.path.dirname(os.path.abspath(__file__))
script = os.path.join(repo_root, "src", "Tests", "run_integration_tests.py")
logpath = os.path.join(tempfile.gettempdir(), "test_log.txt")

# Find 3dsmax.exe - check common install locations or MAXPATH env var.
# Entries below are generated from all artifacts_*.xml found in the repo root, highest year first.
max_candidates = [
  os.environ.get("MAXPATH", ""),
  # r"C:\Program Files\Autodesk\3ds Max <YEAR_N>\3dsmax.exe",
  # r"C:\Program Files\Autodesk\3ds Max <YEAR_N-1>\3dsmax.exe",
  # ... one entry per artifacts_YEAR.xml, descending
]

maxpath = next((p for p in max_candidates if p and os.path.exists(p)), None)
if not maxpath:
  print("ERROR: 3dsmax.exe not found. Set MAXPATH env var or add your install path to max_candidates.")
  sys.exit(1)

result = subprocess.run(
  [sys.executable, script, "--maxpath", maxpath,
   "--tests", "<test_file>.ms", "--logpath", logpath],
  capture_output=True, text=True, timeout=900
)
print("Return code:", result.returncode)
print("Log:", logpath)
```

Run with `python run_tests.py`, then read the log at the path you specified.

**Notes:**
- 3dsMax takes a minute to start, use at least 900s timeout
- Return code `4294967295` (0xFFFFFFFF) is normal; actual pass/fail is in the log file
- Check git log on the test file to identify pre-existing known failures before assuming a failure is yours
- `test_layer_editor_selection` in `usd-layer-editor/test/layer_editor_test.py` (line ~1152) fails with `AssertionError: 0 != 1`. The test itself has a comment: `# KNOWN ISSUE: due to QTimer::singleShot in modelrebuild being called async`. When running integration tests and seeing only this failure, it is not a regression. Ignore it and consider the test run successful for porting purposes.
- UsdUfe::UsdUndoBlock + UsdUndoManager::trackLayerStates captures most USD layer edits (attribute sets, prim creation/removal, sublayer list edits, etc.) and should be used when mirroring Maya. However, SdfLayer::TransferContent is not captured by this mechanism. For commands that rely on TransferContent, manual layer backup (e.g., CreateAnonymous + TransferContent for snapshot, then restore on undo) is required instead of or in addition to UsdUndoableItem. Do not blindly follow Maya's UsdUndoableItem usage if the operation involves TransferContent — verify that the undo mechanism actually captures the changes.
This prevents two failure modes:
    1. Using manual snapshots when UsdUndoBlock would work (over-engineering, diverges from Maya's design).
    2. Using UsdUndoBlock alone for TransferContent-based operations and silently having broken undo.
The distinction is: tracked layer state changes (edits through the SDF change notification system) are captured; bulk buffer copies like TransferContent bypass that tracking entirely.
