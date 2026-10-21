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


def _evaluateSubLayerPath(path, variables):
    '''Returns the sub-layer path, evaluated if it is a variable expression. Returns an empty string if it cannot be evaluated.'''
    # Older USD versions have no variable expressions.
    if not hasattr(Sdf, 'VariableExpression'):
        return path
    if not Sdf.VariableExpression.IsExpression(path):
        return path
    value = Sdf.VariableExpression(path).Evaluate(variables).value
    return value if isinstance(value, str) else ''


def _getAllSessionLayerIds(stage):
    '''Returns a list of all session layer ids for the given stage.'''
    if not stage:
        return set()
    sessionLayer = stage.GetSessionLayer()
    # The session layer's variables override the root layer's. Older USD versions have no
    # expression variables.
    variables = dict(getattr(stage.GetRootLayer(), 'expressionVariables', {}))
    variables.update(getattr(sessionLayer, 'expressionVariables', {}))
    todo = [sessionLayer]
    sessionLayerIds = set()
    while todo:
        layer = todo.pop()
        sessionLayerIds.add(layer.identifier)
        for subLayerId in layer.subLayerPaths:
            subLayerPath = _evaluateSubLayerPath(subLayerId, variables)
            subLayer = Sdf.Layer.FindRelativeToLayer(layer, subLayerPath) if subLayerPath else None
            if subLayer:
                todo.append(subLayer)
    return sessionLayerIds
