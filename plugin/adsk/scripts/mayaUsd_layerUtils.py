from maya import cmds
import mayaUsd


def getlayerEditorLayerIds(panelName):
    '''Returns the list of layer ids for the selected layers in the layer editor. Returns an empty list if the panel name is invalid.'''
    if not panelName:
        return []
    return cmds.mayaUsdLayerEditorWindow(panelName, query=True, getSelectedLayers=True)


def getLayerEditorProxyShape(panelName):
    '''Returns the proxy shape path for the given layer editor panel. Returns an empty string if the panel name is invalid.'''
    if not panelName:
        return ""
    return cmds.mayaUsdLayerEditorWindow(panelName, query=True, proxyShape=True)


def getLayerEditorStage(panelName):
    '''Returns the stage for the given layer editor panel. Returns None if the panel name is invalid or the proxy shape does not have a stage.'''
    proxyShapePath = getLayerEditorProxyShape(panelName)
    if not proxyShapePath:
        return None
    stage = mayaUsd.lib.GetPrim(proxyShapePath).GetStage()
    return stage
