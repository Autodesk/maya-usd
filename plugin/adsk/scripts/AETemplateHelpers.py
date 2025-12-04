import os.path
import maya.cmds as cmds
import maya.api.OpenMaya as OpenMaya
import maya.internal.ufeSupport.ufeCmdWrapper as ufeCmd
import usdUfe
import mayaUsd
import mayaUsd.ufe
import mayaUsd.lib as mayaUsdLib
import re
from mayaUSDRegisterStrings import getMayaUsdString
from mayaUsdUtils import getUSDDialogFileFilters

def debugMessage(msg):
    DEBUG = False
    if DEBUG:
        print(msg)

__naturalOrderRE = re.compile(r'([0-9]+)')

def GetAllRootPrimNamesNaturalOrder(proxyShape):
    # Custom comparator key
    def natural_key(item):
        return [int(s) if s.isdigit() else s.lower() for s in __naturalOrderRE.split(item)]
    try:
        proxyStage = mayaUsd.ufe.getStage(proxyShape)
        primNames = []
        if proxyStage:
            for prim in proxyStage.TraverseAll():
                if (prim.GetPath().IsRootPrimPath()):
                    primNames.append(prim.GetName())
        # Sort the prim list in natural order
        primNames.sort(key=natural_key)
        return primNames
    except Exception as e:
        debugMessage('GetAllPrimNames() - Error: %s' % str(e))
        pass
    return ''

def GetDefaultPrimName(proxyShape):
    try:
        proxyStage = mayaUsd.ufe.getStage(proxyShape)
        if proxyStage:
            defPrim = proxyStage.GetDefaultPrim()
            if defPrim:
                return defPrim.GetName()
    except Exception as e:
        debugMessage('GetDefaultPrimName() - Error: %s' % str(e))
        pass
    return ''

def SetDefaultPrim(proxyShape, primName):
    try:
        proxyStage = mayaUsd.ufe.getStage(proxyShape)
        if not proxyStage:
            return False

        cmd = None
        if not primName:
            cmd = usdUfe.ClearDefaultPrimCommand(proxyStage)
        else:
            defaultPrim = proxyStage.GetPrimAtPath('/' + primName)
            if defaultPrim:
                cmd = usdUfe.SetDefaultPrimCommand(defaultPrim)

        if cmd is None:
            return False
        
        ufeCmd.execute(cmd)
        return True
    except Exception as e:
        # Note: we do want to tell the user why the set or clear failed.
        OpenMaya.MGlobal.displayError(str(e))
        return False
    
def GetRootLayerName(proxyShape):
    try:
        proxyStage = mayaUsd.ufe.getStage(proxyShape)
        if proxyStage:
            rootLayer = proxyStage.GetRootLayer()
            if rootLayer:
                return rootLayer.GetDisplayName() if rootLayer.anonymous else rootLayer.realPath
    except Exception as e:
        debugMessage('GetRootLayerName() - Error: %s' % str(e))
        pass
    return ''

def IsProxyShapeLayerStackDirty(proxyStage):
    # Helper method which returns True if any layer (skipping SessionLayer)
    # in the layer stack is dirty.
    if proxyStage:
        # Is any layer in the layerstack dirty?
        for layer in proxyStage.GetLayerStack(includeSessionLayers=False):
            debugMessage('  Processing layer: %s' % layer.GetDisplayName())
            if layer.dirty:
                debugMessage('    Found dirty layer')
                return True
    return False

def GetStageFromProxyShapeAttr(attr):
    # Helper method which returns the proxy shape name and Usd stage
    # from the input attribute on the proxy shape.

    # First get the stage name from the input attribute.
    stageName = attr.split('.')[0]

    # Convert that into a long Maya path so we can get the USD stage.
    res = cmds.ls(stageName, l=True)
    fullStageName = res[0]
    proxyStage = mayaUsd.ufe.getStage(fullStageName)

    return(stageName, proxyStage)

def GetFullStageNameFromProxyShapeAttr(attr):
    # Helper method which returns the full stage name
    # First get the stage name from the input attribute.
    stageName = attr.split('.')[0]
    # Convert that into a long Maya path so we can get the USD stage.
    res = cmds.ls(stageName, l=True)
    fullStageName = res[0]
    
    return(fullStageName)

def RequireUsdPathsRelativeToMayaSceneFile():
    opVarName = "mayaUsd_MakePathRelativeToSceneFile"
    return cmds.optionVar(exists=opVarName) and cmds.optionVar(query=opVarName)

def ProxyShapeFilePathChanged(filePathAttr, newFilePath=None):
    # Function called from the MayaUsd Proxy Shape template when the file path
    # text field of the file path attibute custom control is modified or interacted with.
    #
    # Arguments:
    # - filePathAttr: string representing the file path attribute.
    # - newFilePath : If None, the user clicked the file browser button.
    #                 - In this case a file dialog will be opened to allow user to pick USD file.
    #                 Otherwise when a string, the user entered a filename directly in the field.
    #                 - In this case simply try and load the file.
    #
    # Return Value:
    # - True:  The input file path attribute was updated with a new value.
    # - False: No update to file path attribute was done.
    #
    debugMessage('ProxyShapeFilePathChanged(%s, %s)' % (filePathAttr, newFilePath))
    try:
        stageName, proxyStage = GetStageFromProxyShapeAttr(filePathAttr)

        # Does the filepath attribute contain a valid file?
        currFilePath = cmds.getAttr(filePathAttr)
        dirtyStack = False
        openFileDialog = False
        if currFilePath:
            # We have a current file path, so check if any layer in the layerstack is dirty?
            dirtyStack = IsProxyShapeLayerStackDirty(proxyStage)
            if not dirtyStack:
                openFileDialog = True
        else:
            # No current file path, so do we just have an empty stage or was something added to
            # this anon layer?
            rootLayer = proxyStage.GetRootLayer()
            if rootLayer and not rootLayer.empty:
                dirtyStack = True
            elif newFilePath is None:
                debugMessage('  Empty stage, opening file dialog...')
                openFileDialog = True

        # If we have a dirty layer stack, display dialog to confirm new loading.
        if dirtyStack:
            kTitleFormat = getMayaUsdString("kDiscardStageEditsTitle")
            kMsgFormat = getMayaUsdString("kDiscardStageEditsLoadMsg")
            kYes = getMayaUsdString("kButtonYes")
            kNo = getMayaUsdString("kButtonNo")
            kTitle = cmds.format(kTitleFormat, stringArg=stageName)
            # Our title is a little long, and the confirmDialog command is not making the
            # dialog wide enough to show it all. So by telling the message string to not
            # word-wrap, the dialog is forced wider.
            kMsg = "<p style='white-space:pre'>" + cmds.format(kMsgFormat, stringArg=stageName)
            res = cmds.confirmDialog(title=kTitle,
                                     message=kMsg, messageAlign='left', 
                                     button=[kYes, kNo], defaultButton=kYes, cancelButton=kNo, dismissString=kNo,
                                     icon='warning')
            if res == kYes:
                debugMessage('  User confirmed load action, opening file dialog...')
                openFileDialog = True
            else:
                # User said NO to update, so don't proceed any farther.
                return False

        if openFileDialog and newFilePath is None:
            # Pop the file open dialog for user to load new usd file.
            title = getMayaUsdString("kLoadUSDFile")
            okCaption = getMayaUsdString("kLoad")
            fileFilter = getUSDDialogFileFilters()

            # Note: empty or missing attribute return None, but we want an empty string
            #       in that case.
            primPath = cmds.getAttr(stageName+'.primPath') or ''
            excludedPrimPaths = cmds.getAttr(stageName+'.excludePrimPaths') or ''
            loadPayloads = mayaUsdLib.isLoadingAllPaylaods(stageName)

            cmds.optionVar(stringValue=('stageFromFile_primPath', primPath))
            cmds.optionVar(stringValue=('stageFromFile_excludePrimPath', excludedPrimPaths))
            cmds.optionVar(intValue=('stageFromFile_loadPayloads', loadPayloads))

            startDir = ''
            if not startDir and currFilePath:
                startDir = os.path.dirname(currFilePath)
            if not startDir:
                if cmds.file(q=True, exists=True):
                    fullPath = cmds.file(q=True, loc=True)
                    startDir = os.path.dirname(fullPath)

            res = cmds.fileDialog2(caption=title, fileMode=1, ff=fileFilter, okc=okCaption,
                                   optionsUICreate='stageFromFile_UISetup',
                                   optionsUIInit='stageFromFile_UIInit',
                                   optionsUICommit='stageFromFile_UICommit',
                                   startingDirectory=startDir)
            if res and len(res) == 1:
                debugMessage('    User picked USD file, setting file path attribute')
                # Simply set the file path attribute. The proxy shape will load the file.
                usdFileToLoad = res[0]
                requireRelative = RequireUsdPathsRelativeToMayaSceneFile()
                if requireRelative:
                    usdFileToLoad = mayaUsdLib.Util.getPathRelativeToMayaSceneFile(usdFileToLoad)

                primPath = cmds.optionVar(query='stageFromFile_primPath')
                excludedPrimPaths = cmds.optionVar(query='stageFromFile_excludePrimPath')
                loadPayloads = cmds.optionVar(query='stageFromFile_loadPayloads')

                # Note: load rules must be the first thing set so the stage gets loaded in teh correct state right away.
                mayaUsdLib.setLoadRulesAttribute(stageName, loadPayloads)

                cmds.setAttr(filePathAttr, usdFileToLoad, type='string')
                cmds.setAttr(filePathAttr+"Relative", requireRelative)
                cmds.setAttr(stageName+'.primPath', primPath, type="string")
                cmds.setAttr(stageName+'.excludePrimPaths', excludedPrimPaths, type="string")

                return True
        elif newFilePath is not None:
            # Instead of opening a file dialog to get the USD file, simply
            # use the one provided as input.
            # Note: it is important to check "not None" since the new file path can be
            #       an empty string.
            debugMessage('  New file to load provided, setting filepath="%s"' % newFilePath)
            cmds.setAttr(filePathAttr, newFilePath, type='string')
            return True
    except Exception as e:
        debugMessage('ProxyShapeFilePathChanged() - Error: %s' % str(e))
        pass
    return False

def HandleComponentReload(proxyStage):
    try:
        print("Attempting to import component manager modules and check for component description...")
        from AdskUsdComponentCreator import ComponentDescription  # type: ignore
        from usd_component_creator_plugin import MayaComponentManager  # type: ignore
        comp_desc = ComponentDescription.CreateFromStageMetadata(proxyStage)
        print(f"ComponentDescription.CreateFromStageMetadata returned: {comp_desc}")
        if not comp_desc:
            return False
        # Check if the component was ever saved: 
        # If the root layer isn't dirty, it must have been saved at least once.
        root_layer = proxyStage.GetRootLayer()

        ever_saved = False

        if root_layer.dirty:
            print("Root layer is dirty, checking if it was ever saved...")
            from pxr import Sdf
            # Create a new in-memory layer and try to read the contents of the current root layer file
            in_memory_layer = Sdf.Layer.CreateAnonymous()
            print(f"Created in-memory anonymous layer: {in_memory_layer}")
            # Only perform this check if the root layer has a real file path
            root_layer_path = root_layer.realPath
            print(f"Root layer realPath: {root_layer_path}")
            ever_saved = True

            # Read the file into the in-memory layer.
            # Use Sdf.Layer.OpenAsAnonymous to open a copy of the root layer into memory (Python API equivalent)
            from pxr import Sdf
            print(f"Attempting to open disk version for layer path: {root_layer_path}")
            disk_version = Sdf.Layer.OpenAsAnonymous(root_layer_path)
            print(f"Disk version loaded: {disk_version}, IsEmpty: {disk_version.empty}")
            ever_saved = not disk_version.empty

        if ever_saved:
            print("ever_saved is True, proceeding with component reload logic.")
            manager = MayaComponentManager.GetInstance()
            print(f"Obtained MayaComponentManager instance: {manager}")
            to_save_ids = manager.GetSaveInfo(proxyStage) or []
            print(f"Manager.GetSaveInfo returned: {to_save_ids}")
            if to_save_ids:
                print("There are components pending save. Prompting user for reload confirmation.")
                kTitleFormat = getMayaUsdString("kDiscardStageEditsTitle")
                print(f"kTitleFormat: {kTitleFormat}")
                kMsgFormat = getMayaUsdString("kDiscardComponentEditsReloadMsg")
                print(f"kMsgFormat: {kMsgFormat}")
                kYes = getMayaUsdString("kButtonReload")
                print(f"kYes: {kYes}")
                kNo = getMayaUsdString("kButtonCancel")
                print(f"kNo: {kNo}")
                kTitle = cmds.format(kTitleFormat, stringArg=stageName)
                print(f"Formatted dialog title: {kTitle}")
                # Like the normal stage case, widen dialog by preventing message word-wrap.
                filepath = cmds.getAttr(filePathAttr)
                print(f"Retrieved file path from attr {filePathAttr}: {filepath}")
                if filepath:
                    filename = os.path.basename(filepath)
                    print(f"Extracted file name from path: {filename}")
                    #kMsg = "<p style='white-space:pre'>" + cmds.format(kMsgFormat, stringArg=(stageName))
                    kMsg = cmds.format(kMsgFormat, stringArg=(stageName))
                    print(f"Formatted dialog message: {kMsg}")
                    print("Opening confirmation dialog for reload...")
                    res = cmds.confirmDialog(title=kTitle,
                                            message=kMsg, messageAlign='left',
                                            button=[kYes, kNo], defaultButton=kYes, cancelButton=kNo, dismissString=kNo,
                                            icon='warning')
                    print(f"User responded to confirmDialog: {res}")
                    if res == kYes:
                        print('  Component reload confirmed, calling ComponentManager.ReloadComponent()')
                        manager.ReloadComponent(proxyStage)

                    else:
                        print('  Component reload canceled by user.')
                else:
                    print("No filepath found; skipping dialog and reload.")
            else:
                print('  Component has no pending save impact, reloading without confirmation')
                # Assumes ReloadComponent(stage) exists on the manager.
                manager.ReloadComponent(proxyStage)
        return True
    except Exception as e:
        return False

def ProxyShapeFilePathRefresh(filePathAttr):
    # Function called from the MayaUsd Proxy Shape template when the refresh
    # button of the file path attribute custom control is clicked.
    #
    # Arguments:
    # - filePathAttr: string representing the file path attribute.
    #
    # Return Value:
    # - None
    #
    debugMessage('ProxyShapeFilePathRefresh(%s)' % filePathAttr)
    try:
        stageName, proxyStage = GetStageFromProxyShapeAttr(filePathAttr)

        handled = TryHandleComponentRefresh(proxyStage)
        if not handled:
            # Is any layer in the layerstack dirty?
            # - If nothing is dirty, we simply reload.
            # - If any layer is dirty pop a confirmation dialog to reload.
            # Note: since we are file-backed, this will reload based on the file
            #       which could have changed on disk.
            if not IsProxyShapeLayerStackDirty(proxyStage):
                debugMessage('  No dirty layers, calling UsdStage.Reload()')
                proxyStage.Reload()
            else:
                kTitleFormat = getMayaUsdString("kDiscardStageEditsTitle")
                kMsgFormat = getMayaUsdString("kDiscardStageEditsReloadMsg")
                kYes = getMayaUsdString("kButtonYes")
                kNo = getMayaUsdString("kButtonNo")
                kTitle = cmds.format(kTitleFormat, stringArg=stageName)
                # Our title is a little long, and the confirmDialog command is not making the
                # dialog wide enough to show it all. So by telling the message string to not
                # word-wrap, the dialog is forced wider.
                filepath = cmds.getAttr(filePathAttr)
                if filepath:
                    filename = os.path.basename(filepath)
                    kMsg = "<p style='white-space:pre'>" + cmds.format(kMsgFormat, stringArg=(filename, stageName))
                    res = cmds.confirmDialog(title=kTitle,
                                                message=kMsg, messageAlign='left', 
                                                button=[kYes, kNo], defaultButton=kYes, cancelButton=kNo, dismissString=kNo,
                                                icon='warning')
                    if res == kYes:
                        debugMessage('  User confirmed reload action, calling UsdStage.Reload()')
                        proxyStage.Reload()
        
        # Refresh the system lock status of the stage and its sublayers
        stageFilePath = cmds.getAttr(filePathAttr)
        fullStageName = GetFullStageNameFromProxyShapeAttr(filePathAttr)
        cmds.mayaUsdLayerEditor(stageFilePath, edit=True, refreshSystemLock=(fullStageName, True))
        
    except Exception as e:
        debugMessage('ProxyShapeFilePathRefresh() - Error: %s' % str(e))
