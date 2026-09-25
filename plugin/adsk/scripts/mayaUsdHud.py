#
# Copyright 2026 Sony Interactive Entertainment
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

# USD Details heads-up display & Options.

from maya import cmds
import maya.mel as mel

from mayaUSDRegisterStrings import getMayaUsdString

_blocks = []
_typeBlocks = []
_cache = {}
_selCache = {}
_dirty = True
_selDirty = True
_visible = False
_jobs = []
_shapeJobs = []
_listeners = []
_repaintScheduled = False
_countedStages = []
_menuItems = []
_menuRetries = 0
_defaultsSeeded = False
_lib = None
_libError = ''

# Screen region 0-9
_section = 0
_padding = 45

_OPTIONVAR_PREFIX = 'mayaUsd_UsdDetails'
_VISIBLE_OPTIONVAR = _OPTIONVAR_PREFIX + 'Visible'
_SECTION_OPTIONVAR = _OPTIONVAR_PREFIX + 'Section'
_PADDING_OPTIONVAR = _OPTIONVAR_PREFIX + 'Padding'
_MAX_TYPES_OPTIONVAR = _OPTIONVAR_PREFIX + 'MaxPrimTypes'

_MENU_ITEM = 'usdDetailsHudItem'
_OPTION_BOX_ITEM = 'usdDetailsHudOptionBoxItem'
_RUNTIME_COMMAND = 'ToggleUsdDetailsHud'

_BLOCK_PREFIX = 'usdDetails_'
_TYPE_BLOCK_PREFIX = _BLOCK_PREFIX + 'type'
_TYPES_ROW_KEY = 'primTypes'
_PROXY_SHAPE_TYPE = 'mayaUsdProxyShapeBase'

_SHAPE_ATTRS = ('drawRenderPurpose', 'drawProxyPurpose', 'drawGuidePurpose',
                'excludePrimPaths')

_MAX_MENU_RETRIES = 20
_DEFAULT_MAX_TYPES = 8
_MAX_TYPES_LIMIT = 24
_BLANK_LABEL = ' '
_COLUMN_CHARS = 12
_DATA_WIDTH = 165

_ROWS = (
    ('prims',     'kHudTotalPrims',     'RowPrims',     True),
    ('meshes',    'kHudTotalPrimMesh',  'RowMeshes',    True),
    ('vertices',  'kHudVerts',          'RowVerts',     True),
    ('triangles', 'kHudTris',           'RowTris',      True),
    ('faces',     'kHudFaces',          'RowFaces',     True),
    ('instances', 'kHudInstances',      'RowInstances', False),
    ('primTypes', 'kHudTotalPrimTypes', 'RowPrimTypes', True),
)

_COUNT_OPTIONS = (
    ('InstanceProxies', 'instanceProxies', True,  'kHudOptInstanceProxies'),
    ('IncludeInactive', 'includeInactive', False, 'kHudOptIncludeInactive'),
    ('IncludeClasses',  'includeClasses',  False, 'kHudOptIncludeClasses'),
    ('IncludeOvers',    'includeOvers',    False, 'kHudOptIncludeOvers'),
)

_OPTION_DEFAULTS = tuple([(row[2], row[3]) for row in _ROWS]
                         + [(opt[0], opt[2]) for opt in _COUNT_OPTIONS])

def _ensureDefaults(factory=False):
    global _defaultsSeeded

    if _defaultsSeeded and not factory:
        return

    for name, default in ([(_OPTIONVAR_PREFIX + suffix, default)
                           for suffix, default in _OPTION_DEFAULTS]
                          + [(_MAX_TYPES_OPTIONVAR, _DEFAULT_MAX_TYPES)]):
        if factory or not cmds.optionVar(exists=name):
            cmds.optionVar(intValue=(name, int(default)))

    if not cmds.optionVar(exists=_VISIBLE_OPTIONVAR):
        cmds.optionVar(intValue=(_VISIBLE_OPTIONVAR, 0))

    _defaultsSeeded = True


def _option(suffix):
    _ensureDefaults()
    return bool(cmds.optionVar(query=_OPTIONVAR_PREFIX + suffix))


def _setOption(suffix, value):
    cmds.optionVar(intValue=(_OPTIONVAR_PREFIX + suffix, int(bool(value))))


def _enabledRows():
    return [row for row in _ROWS if _option(row[2])]


def _maxTypes():
    if not _option('RowPrimTypes'):
        return 0
    try:
        value = int(cmds.optionVar(query=_MAX_TYPES_OPTIONVAR))
    except (TypeError, ValueError):
        value = _DEFAULT_MAX_TYPES
    return max(0, min(_MAX_TYPES_LIMIT, value))


def _mayaUsdLib():
    global _lib, _libError

    if _lib is not None:
        return _lib

    try:
        import mayaUsd.lib as mayaUsdLib
        if not hasattr(mayaUsdLib, 'ComputeUsdDetails'):
            raise AttributeError('mayaUsd.lib has no ComputeUsdDetails')
        _lib = mayaUsdLib
        _libError = ''
    except Exception as exc:
        _libError = str(exc)

    return _lib


def _statsArgs():
    args = dict((name, _option(suffix))
                for suffix, name, _default, _label in _COUNT_OPTIONS)
    args['byType'] = _option('RowPrimTypes')
    return args


def _group(digits):
    try:
        return format(int(digits), ',')
    except (TypeError, ValueError):
        return str(digits)


def _columns(total, selected):
    return _group(total) + _group(selected).rjust(_COLUMN_CHARS)


def _allStageObjects():
    try:
        return cmds.ls(type=_PROXY_SHAPE_TYPE, long=True) or []
    except Exception:
        return []


def _holdsStage(dagPath):
    try:
        return bool(cmds.ls(dagPath, type=_PROXY_SHAPE_TYPE)
                    or cmds.listRelatives(dagPath, allDescendents=True,
                                          type=_PROXY_SHAPE_TYPE))
    except Exception:
        return False


def _selectedObjects():
    try:
        import ufe
        selection = ufe.GlobalSelection.get()
    except Exception:
        return []

    objects = []
    for item in selection:
        try:
            path = item.path()
            name = ufe.PathString.string(path)
            # More than one segment means the item lives inside a stage.
            if path.nbSegments() > 1 or _holdsStage(name):
                objects.append(name)
        except Exception:
            continue
    return objects


def _computeDetails(objects):
    try:
        return _mayaUsdLib().ComputeUsdDetails(objects=objects,
                                               **_statsArgs()) or {}
    except Exception:
        return {}


def _countScene():
    global _cache, _dirty, _countedStages

    _dirty = False
    if _mayaUsdLib() is None:
        _cache = {}
        return

    _countedStages = _allStageObjects()
    _cache = _computeDetails(_countedStages)


def _countSelection():
    global _selCache, _selDirty

    _selDirty = False
    _selCache = {}
    if _mayaUsdLib() is None:
        return

    objects = _selectedObjects()
    if objects:
        _selCache = _computeDetails(objects)


def _ensureCounts():
    if _dirty:
        _countScene()
    if _selDirty:
        _countSelection()


def _primTypeCounts(cache):
    counts = []
    for name, count in (cache.get('types') or {}).items():
        try:
            counts.append((name, int(count)))
        except (TypeError, ValueError):
            continue
    counts.sort(key=lambda pair: (-pair[1], pair[0]))
    return counts


def _primTypeRows():
    limit = _maxTypes()
    if limit <= 0:
        return []

    counts = _primTypeCounts(_cache)
    selected = dict(_primTypeCounts(_selCache))
    split = limit if len(counts) <= limit else limit - 1
    head, folded = counts[:split], counts[split:]

    rows = [(name, count, selected.get(name, 0)) for name, count in head]
    if folded:
        rows.append((getMayaUsdString('kHudOtherTypes'),
                     sum(count for _name, count in folded),
                     sum(selected.get(name, 0) for name, _count in folded)))
    return rows


def typeValue(index):
    _ensureCounts()
    rows = _primTypeRows()
    return _columns(rows[index][1], rows[index][2]) if index < len(rows) else ''


def _applyTypeLabels():
    rows = _primTypeRows()
    for index, name in enumerate(_typeBlocks):
        try:
            if not cmds.headsUpDisplay(name, exists=True):
                continue
            used = index < len(rows)
            cmds.headsUpDisplay(
                name, edit=True,
                label=(rows[index][0] + ':') if used else _BLANK_LABEL,
                # A type block with no type to name would draw a bare number.
                visible=_visible and used)
        except Exception:
            continue


def value(key):
    _ensureCounts()

    if _mayaUsdLib() is None:
        return 'n/a'
    if key == _TYPES_ROW_KEY:
        return _columns(len(_primTypeCounts(_cache)),
                        len(_primTypeCounts(_selCache)))
    if key not in _cache:
        return '--'
    return _columns(_cache[key], _selCache.get(key, 0))


def refresh(selectionOnly=False):
    global _dirty, _selDirty

    _selDirty = True
    if not selectionOnly:
        _dirty = True
    if _visible:
        _repaint()


def _repaint():
    _ensureCounts()
    _applyTypeLabels()

    for block in _blocks:
        if cmds.headsUpDisplay(block, exists=True):
            cmds.headsUpDisplay(block, refresh=True)


def _refreshSelection():
    stages = _allStageObjects()
    if stages != _countedStages:
        _installShapeJobs()
    refresh(selectionOnly=(stages == _countedStages))


def _markStale(*_args):
    global _dirty, _selDirty, _repaintScheduled

    _dirty = True
    _selDirty = True
    if not _visible or _repaintScheduled:
        return

    _repaintScheduled = True
    try:
        cmds.evalDeferred(_repaintDeferred, lowestPriority=True)
    except Exception:
        _repaintScheduled = False


def _repaintDeferred():
    global _repaintScheduled

    _repaintScheduled = False
    if _visible:
        _repaint()


def _firstFreeRun(section, count):
    taken = set()
    for name in cmds.headsUpDisplay(query=True, listHeadsUpDisplays=True) or []:
        try:
            if cmds.headsUpDisplay(name, query=True, section=True) == section:
                taken.add(cmds.headsUpDisplay(name, query=True, block=True))
        except Exception:
            continue

    start = 0
    while any(start + offset in taken for offset in range(count)):
        start += 1
    return start


def _createBlocks():
    wanted = [(_BLOCK_PREFIX + key,
               getMayaUsdString(labelKey),
               "__import__('mayaUsdHud').value('{}')".format(key),
               False)
              for key, labelKey, _suffix, _default in _enabledRows()]
    wanted += [(_TYPE_BLOCK_PREFIX + str(index),
                _BLANK_LABEL,
                "__import__('mayaUsdHud').typeValue({})".format(index),
                True)
               for index in range(_maxTypes())]

    cursor = _firstFreeRun(_section, len(wanted))

    for name, label, command, isType in wanted:
        for block in range(cursor, cursor + 16):
            try:
                cmds.headsUpDisplay(
                    name, section=_section, block=block, blockSize='small',
                    blockAlignment='right', label=label,
                    labelFontSize='small', labelWidth=115,
                    dataFontSize='small', dataWidth=_DATA_WIDTH,
                    dataAlignment='right', padding=_padding,
                    allowOverlap=True, command=command,
                    event='SelectionChanged')
            except Exception:
                continue
            cursor = block + 1
            _blocks.append(name)
            if isType:
                _typeBlocks.append(name)
            cmds.headsUpDisplay(name, edit=True, visible=False)
            break
        else:
            cursor += 1


def _deleteBlocks():
    global _cache, _selCache, _dirty, _selDirty, _countedStages

    _removeJobs()
    for block in _blocks:
        if cmds.headsUpDisplay(block, exists=True):
            cmds.headsUpDisplay(block, remove=True)

    del _blocks[:]
    del _typeBlocks[:]
    _cache = {}
    _selCache = {}
    _dirty = True
    _selDirty = True
    _countedStages = []


def _rebuildBlocks():
    wasVisible = _visible

    _deleteBlocks()
    _createBlocks()
    setVisible(wasVisible)


def _installJobs():
    if _jobs or _listeners:
        return

    events = (('SelectionChanged', _refreshSelection),
              ('timeChanged', refresh),
              ('SceneOpened', refresh),
              ('NewSceneOpened', refresh),
              ('Undo', refresh),
              ('Redo', refresh))

    available = cmds.scriptJob(listEvents=True) or []
    for event, handler in events:
        if event in available:
            _jobs.append(cmds.scriptJob(event=[event, handler]))

    try:
        from pxr import Tf, Usd
        _listeners.append(Tf.Notice.RegisterGlobally(Usd.Notice.ObjectsChanged, _markStale))
    except Exception:
        pass

    _installShapeJobs()


def _installShapeJobs():
    _killJobs(_shapeJobs)

    for shape in _allStageObjects():
        for attr in _SHAPE_ATTRS:
            plug = '{}.{}'.format(shape, attr)
            try:
                if cmds.objExists(plug):
                    _shapeJobs.append(
                        cmds.scriptJob(attributeChange=[plug, _markStale]))
            except Exception:
                continue


def _killJobs(jobs):
    for job in jobs:
        try:
            if cmds.scriptJob(exists=job):
                cmds.scriptJob(kill=job, force=True)
        except Exception:
            pass
    del jobs[:]


def _removeJobs():
    _killJobs(_jobs)
    _killJobs(_shapeJobs)

    for listener in _listeners:
        try:
            listener.Revoke()
        except Exception:
            pass
    del _listeners[:]


def _headsUpDisplayMenu():
    candidates = ['HeadsUpDisplayMenu']
    try:
        candidates.insert(0, mel.eval(
            'global string $gHeadsUpDisplayMenu;'
            ' $mayaUsdTmpHudMenu = $gHeadsUpDisplayMenu'))
    except Exception:
        pass

    for candidate in candidates:
        try:
            if candidate and (cmds.menuItem(candidate, exists=True)
                              or cmds.menu(candidate, exists=True)):
                return candidate
        except Exception:
            continue
    return None


def _createRuntimeCommand():
    try:
        if not cmds.runTimeCommand(_RUNTIME_COMMAND, exists=True):
            cmds.runTimeCommand(
                _RUNTIME_COMMAND,
                default=False,
                annotation=getMayaUsdString('kHudMenuAnn'),
                category='Menu items.Display',
                commandLanguage='python',
                command='import mayaUsdHud; mayaUsdHud.toggle()')
    except Exception:
        pass


def _deleteRuntimeCommand():
    try:
        if cmds.runTimeCommand(_RUNTIME_COMMAND, exists=True):
            cmds.runTimeCommand(_RUNTIME_COMMAND, edit=True, delete=True)
    except Exception:
        pass


def _createMenu():
    global _menuRetries

    if _menuItems:
        return True

    headsUpMenu = _headsUpDisplayMenu()
    if not headsUpMenu:
        if _menuRetries < _MAX_MENU_RETRIES:
            _menuRetries += 1
            try:
                cmds.evalDeferred(_createMenu, lowestPriority=True)
            except Exception:
                pass
        return False
    _menuRetries = 0

    cmds.setParent(headsUpMenu, menu=True)

    kwargs = {'label': getMayaUsdString('kHudMenuLabel'),
              'annotation': getMayaUsdString('kHudMenuAnn'),
              'checkBox': isVisible(),
              'command': lambda *_args: toggle()}

    if cmds.menuItem('frameRateItem', exists=True):
        kwargs['insertAfter'] = 'frameRateItem'

    item = cmds.menuItem(_MENU_ITEM, **kwargs)
    _menuItems.append(item)

    _menuItems.append(cmds.menuItem(
        _OPTION_BOX_ITEM, optionBox=True, enableCommandRepeat=False,
        annotation=getMayaUsdString('kHudOptionsAnn'), insertAfter=item,
        command=lambda *_args: showOptions()))

    return True


def _syncMenu():
    try:
        if _menuItems and cmds.menuItem(_menuItems[0], exists=True):
            cmds.menuItem(_menuItems[0], edit=True, checkBox=isVisible())
    except Exception:
        pass


def _deleteMenu():
    for item in reversed(_menuItems):
        try:
            if cmds.menuItem(item, exists=True):
                cmds.deleteUI(item, menuItem=True)
        except Exception:
            pass
    del _menuItems[:]


def _rowControl(suffix):
    return 'usdDetailsOpt' + suffix


def _optionFrames():
    rows = [(suffix,
             getMayaUsdString(labelKey).rstrip(':：').strip(),
             getMayaUsdString('kHudOptPrimTypesAnn')
             if suffix == 'RowPrimTypes' else '')
            for _key, labelKey, suffix, _default in _ROWS]
    counts = [(suffix, getMayaUsdString(labelKey),
               getMayaUsdString(labelKey + 'Ann'))
              for suffix, _name, _default, labelKey in _COUNT_OPTIONS]
    return (('kHudOptionsDisplayFrame', rows),
            ('kHudOptionsCountingFrame', counts))


def _optionsSetup(factory=False):
    _ensureDefaults(factory)
    for suffix, _default in _OPTION_DEFAULTS:
        cmds.checkBoxGrp(_rowControl(suffix), edit=True, value1=_option(suffix))


def optionsApply():
    for suffix, _default in _OPTION_DEFAULTS:
        _setOption(suffix, cmds.checkBoxGrp(_rowControl(suffix),
                                            query=True, value1=True))

    if not _enabledRows():
        cmds.warning(getMayaUsdString('kHudNoRowsWarn'))
    _rebuildBlocks()


def optionsReset():
    _optionsSetup(factory=True)


def _wireOptionBoxButtons():
    def apply_(*_args):
        optionsApply()

    def applyAndClose(*_args):
        optionsApply()
        mel.eval('hideOptionBox')

    def reset(*_args):
        optionsReset()

    def close(*_args):
        mel.eval('hideOptionBox')

    for getter, handler in (('getOptionBoxApplyAndCloseBtn', applyAndClose),
                            ('getOptionBoxApplyBtn', apply_),
                            ('getOptionBoxSaveBtn', apply_),
                            ('getOptionBoxResetBtn', reset),
                            ('getOptionBoxCloseBtn', close)):
        cmds.button(mel.eval(getter + '()'), edit=True, command=handler)

    for getter, handler in (('getOptionBoxEditMenuSaveItem', apply_),
                            ('getOptionBoxEditMenuResetItem', reset)):
        cmds.menuItem(mel.eval(getter + '()'), edit=True, command=handler)


def showOptions():
    _ensureDefaults()

    cmds.setParent(mel.eval('getOptionBox()'))
    mel.eval('setOptionBoxCommandName("usdDetails")')

    cmds.setUITemplate('DefaultTemplate', pushTemplate=True)
    try:
        cmds.tabLayout(tabsVisible=False, scrollable=True)
        cmds.columnLayout(adjustableColumn=True)

        for frameKey, controls in _optionFrames():
            cmds.frameLayout(label=getMayaUsdString(frameKey),
                             collapsable=False)
            cmds.columnLayout(adjustableColumn=True)
            for suffix, label, annotation in controls:
                cmds.checkBoxGrp(_rowControl(suffix), numberOfCheckBoxes=1,
                                 label='', label1=label, annotation=annotation)
            cmds.setParent('..')
            cmds.setParent('..')

        cmds.setParent('..')
    finally:
        cmds.setUITemplate(popTemplate=True)

    _wireOptionBoxButtons()

    title = getMayaUsdString('kHudOptionsLabel')
    title = title.replace('\\', '\\\\').replace('"', '\\"')
    mel.eval('setOptionBoxTitle("{}")'.format(title))

    _optionsSetup()
    mel.eval('showOptionBox()')


def isVisible():
    return bool(_blocks) and _visible


def setVisible(state):
    global _visible

    _visible = bool(state) and bool(_blocks)

    for block in _blocks:
        if block not in _typeBlocks and cmds.headsUpDisplay(block, exists=True):
            cmds.headsUpDisplay(block, edit=True, visible=_visible)

    if _visible:
        _installJobs()
        refresh()
        if _mayaUsdLib() is None:
            cmds.warning(
                getMayaUsdString('kHudUnavailableWarn').format(_libError))
    else:
        _removeJobs()
        _applyTypeLabels()

    cmds.optionVar(intValue=(_VISIBLE_OPTIONVAR, int(_visible)))
    _syncMenu()


def toggle():
    setVisible(not isVisible())

# Called from mayaUsd_pluginUICreation on plugin load
def loadui():
    global _section, _padding, _menuRetries, _visible

    unloadui()

    _ensureDefaults()
    _menuRetries = 0
    _visible = False

    # Restore the position chosen in a previous session.
    if cmds.optionVar(exists=_SECTION_OPTIONVAR):
        saved = cmds.optionVar(q=_SECTION_OPTIONVAR)
        if 0 <= saved <= 9:
            _section = saved
    if cmds.optionVar(exists=_PADDING_OPTIONVAR):
        saved = cmds.optionVar(q=_PADDING_OPTIONVAR)
        if saved >= 0:
            _padding = saved

    _createBlocks()
    _createRuntimeCommand()
    _createMenu()

    if cmds.optionVar(q=_VISIBLE_OPTIONVAR):
        setVisible(True)


# Called from mayaUsd_pluginUIDeletion on plugin unload.
def unloadui():
    global _visible

    _visible = False
    _deleteBlocks()
    _deleteMenu()
    _deleteRuntimeCommand()