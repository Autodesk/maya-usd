import ufe
import mayaUsd.ufe
import maya.internal.ufeSupport.ufeCmdWrapper as ufeCmd

def GetDefaultPrimName(proxyShape):
    try:
        proxyStage = mayaUsd.ufe.getStage(proxyShape)
        if proxyStage:
            defPrim = proxyStage.GetDefaultPrim()
            if defPrim:
                return defPrim.GetName()
    except:
        pass
    return ''

def GetRootLayerName(proxyShape):
    try:
        proxyStage = mayaUsd.ufe.getStage(proxyShape)
        if proxyStage:
            rootLayer = proxyStage.GetRootLayer()
            if rootLayer:
                return rootLayer.GetDisplayName() if rootLayer.anonymous else rootLayer.realPath
    except:
        pass
    return ''

def ProxyShapePayloads(proxyShape, loadAll):
    try:
        proxyPath = ufe.PathString.path(proxyShape)
        ufeItem = ufe.Hierarchy.createItem(proxyPath) if proxyPath else None
        if ufeItem:
            contextOps = ufe.ContextOps.contextOps(ufeItem)
            if contextOps:
                cmdStr = 'Load with Descendants' if loadAll else 'Unload'
                cmd = contextOps.doOpCmd([cmdStr])
                ufeCmd.execute(cmd)
    except:
        pass
