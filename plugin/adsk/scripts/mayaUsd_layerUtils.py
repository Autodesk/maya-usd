from pxr import Sdf
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


def _getAllSessionLayerIds(stage):
    '''Returns a list of all session layer ids for the given stage.'''
    if not stage:
        return set()
    sessionLayer = stage.GetSessionLayer()
    todo = [sessionLayer]
    sessionLayerIds = set()
    while todo:
        layer = todo.pop()
        sessionLayerIds.add(layer.identifier)
        for subLayerId in layer.subLayerPaths:
            subLayer = Sdf.Layer.FindRelativeToLayer(layer, subLayerId)
            todo.append(subLayer)
    return sessionLayerIds


def isSessionInLayerEditorSelection(panelName):
    '''Returns true if the panel is valid and any of the selected layers in the layer editor are session layers.'''
    stage = getLayerEditorStage(panelName)
    if not stage:
        return False

    sessionLayers = _getAllSessionLayerIds(stage)

    layerIds = getlayerEditorLayerIds(panelName)
    for layerId in layerIds:
        if layerId in sessionLayers:
            return True
    return False


def isNonSessionInLayerEditorSelection(panelName):
    '''Returns true if the panel is valid and any of the selected layers in the layer editor are non-session layers.'''
    stage = getLayerEditorStage(panelName)
    if not stage:
        return False

    sessionLayers = _getAllSessionLayerIds(stage)

    layerIds = getlayerEditorLayerIds(panelName)
    for layerId in layerIds:
        if layerId not in sessionLayers:
            return True
    return False
