import maya.api.OpenMaya as om
from pxr import Sdf, Usd, Vt
import mayaUsd
import ufe

# Adapted from testMayaUsdSchemasMayaReference.py.
def createMayaReferencePrim(ufePathStr, mayaReferencePath, mayaNamespace):
    '''Create a Maya reference prim parented to the argument path.'''

    # Append reference name to parent Sdf path.
    ufePath = ufe.PathString.path(ufePathStr)
    parentPath = ufePath.segments[1]
    primPath = Sdf.AssetPath(str(parentPath)+'/MayaReference')

    stage = mayaUsd.ufe.getStage(ufePathStr)
    prim = stage.DefinePrim(primPath.path, 'MayaReference')
    if not prim.IsValid():
        om.MGlobal.displayError("Could not create MayaReference prim under %s" % ufePathStr)
        return

    mayaReferenceAttr = prim.CreateAttribute('mayaReference', Sdf.ValueTypeNames.Asset)
    mayaReferenceAttr.Set(mayaReferencePath)

    mayaNamespaceAttr = prim.CreateAttribute('mayaNamespace', Sdf.ValueTypeNames.String)
    mayaNamespaceAttr.Set(mayaNamespace)
    
