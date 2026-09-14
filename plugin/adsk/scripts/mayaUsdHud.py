#
# Copyright 2026 Autodesk
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# USD stage statistics heads-up display.

from maya import cmds

from mayaUSDRegisterStrings import getMayaUsdString

_blocks = []
_tokens = []
_dirty = True
_jobs = []

_section = 4
_padding = 45

_VISIBLE_OPTIONVAR = 'mayaUsd_StageStatsHUDVisible'
_SECTION_OPTIONVAR = 'mayaUsd_StageStatsHUDSection'
_PADDING_OPTIONVAR = 'mayaUsd_StageStatsHUDPadding'

_ROWS = (
    ('prims', 'kHudTotalPrims'),
    ('meshes', 'kHudTotalPrimMesh'),
    ('vertices', 'kHudVerts'),
    ('triangles', 'kHudTris'),
    ('faces', 'kHudFaces'),
    ('normals', 'kHudNormals'),
)

# format digits
def _group(digits):
    try:
        return format(int(digits), ',')
    except ValueError:
        return digits


# The command every block runs.
def value(key):
    global _tokens, _dirty

    if _dirty:
        try:
            _tokens = cmds.mayaUsdStageStats(q=True, allCounts=True) or []
        except Exception:
            _tokens = []
        _dirty = False

    prefix = key + '='
    for token in _tokens:
        if token.startswith(prefix):
            return _group(token[len(prefix):])
    return '--'


# Mark the cached tokens stale and repaint every block.
def refresh():
    global _dirty

    _dirty = True
    for block in _blocks:
        if cmds.headsUpDisplay(block, exists=True):
            cmds.headsUpDisplay(block, refresh=True)


# First block in a section nothing else has claimed.
def _firstFreeBlock(section):
    try:
        last = cmds.headsUpDisplay(q=True, lastOccupiedBlock=section)
    except Exception:
        last = -1
    if last is None or last < -1:
        last = -1
    return last + 1


# Register one block at or above startBlock.
def _addBlock(section, startBlock, name, label, key):
    command = "__import__('mayaUsdHud').value('{}')".format(key)

    block = startBlock

    for _attempt in range(16):
        try:
            cmds.headsUpDisplay(
                name,
                section=section,
                block=block,
                blockSize='small',
                blockAlignment='right',
                label=label,
                labelFontSize='small',
                labelWidth=115,
                dataFontSize='small',
                dataWidth=80,
                dataAlignment='right',
                padding=_padding,
                allowOverlap=True,
                command=command,
                event='SelectionChanged')
            return block
        except Exception:
            block += 1

    return -1


# scriptJobs that repaint the HUD. Installed only while it is visible.
def _installJobs():
    if _jobs:
        return

    events = ('SelectionChanged', 'timeChanged', 'SceneOpened',
              'NewSceneOpened', 'Undo', 'Redo')

    available = cmds.scriptJob(listEvents=True) or []
    for event in events:
        # Event names vary by Maya version; only register advertised ones.
        if event not in available:
            continue
        _jobs.append(cmds.scriptJob(event=[event, refresh]))


def _removeJobs():
    for job in _jobs:
        if cmds.scriptJob(exists=job):
            cmds.scriptJob(kill=job, force=True)
    del _jobs[:]


def isVisible():
    if not _blocks:
        return False
    if not cmds.headsUpDisplay(_blocks[0], exists=True):
        return False
    return bool(cmds.headsUpDisplay(_blocks[0], q=True, visible=True))


def setVisible(state):
    state = bool(state)

    for block in _blocks:
        if cmds.headsUpDisplay(block, exists=True):
            cmds.headsUpDisplay(block, edit=True, visible=state)

    if state:
        _installJobs()
        refresh()
    else:
        _removeJobs()

    cmds.optionVar(intValue=(_VISIBLE_OPTIONVAR, int(state)))


def toggle():
    setVisible(not isVisible())


# Move the HUD to one of the ten screen regions: 0-4 across the top,
# 5-9 across the bottom. Section 4 is top right, 0 is top left.
def setSection(section):
    if section < 0 or section > 9:
        cmds.warning('mayaUsdHud: section must be 0 to 9 (0-4 top row, 5-9 bottom row).')
        return

    cmds.optionVar(intValue=(_SECTION_OPTIONVAR, section))
    loadui()


def setPadding(padding):
    if padding < 0:
        cmds.warning('mayaUsdHud: padding cannot be negative.')
        return

    cmds.optionVar(intValue=(_PADDING_OPTIONVAR, padding))
    loadui()


def position():
    return (_section, _padding)


# Called from mayaUsd_pluginUICreation on plugin load.
def loadui():
    global _section, _padding

    unloadui()

    # Restore the position chosen in a previous session.
    if cmds.optionVar(exists=_SECTION_OPTIONVAR):
        savedSection = cmds.optionVar(q=_SECTION_OPTIONVAR)
        if 0 <= savedSection <= 9:
            _section = savedSection
    if cmds.optionVar(exists=_PADDING_OPTIONVAR):
        savedPadding = cmds.optionVar(q=_PADDING_OPTIONVAR)
        if savedPadding >= 0:
            _padding = savedPadding

    cursor = _firstFreeBlock(_section)

    for key, labelKey in _ROWS:
        name = 'mayaUsdHud_' + key
        block = _addBlock(_section, cursor, name, getMayaUsdString(labelKey), key)

        if block < 0:
            cursor += 1
            continue

        cursor = block + 1
        _blocks.append(name)
        cmds.headsUpDisplay(name, edit=True, visible=False)

    if not cmds.runTimeCommand('mayaUsdToggleStageStatsHUD', exists=True):
        cmds.runTimeCommand(
            'mayaUsdToggleStageStatsHUD',
            default=True,
            label=getMayaUsdString('kHudToggleLabel'),
            annotation=getMayaUsdString('kHudToggleAnn'),
            category='Menu items.Display',
            command="__import__('mayaUsdHud').toggle()",
            commandLanguage='python')

    if cmds.optionVar(exists=_VISIBLE_OPTIONVAR):
        if cmds.optionVar(q=_VISIBLE_OPTIONVAR):
            setVisible(True)


# Called from mayaUsd_pluginUIDeletion on plugin unload.
def unloadui():
    global _tokens

    _removeJobs()

    for block in _blocks:
        if cmds.headsUpDisplay(block, exists=True):
            cmds.headsUpDisplay(block, remove=True)

    del _blocks[:]
    _tokens = []