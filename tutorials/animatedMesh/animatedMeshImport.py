
import logging
logger = logging.getLogger(__name__)
from pxr import Usd, UsdGeom
from maya import cmds
from AL import usdmaya



class TranslateUSDMeshesToMaya(object):
    """
    Translate all child meshes from a given set of parent prims from the usd scene into maya
    """

    def __init__(self, proxyShapeName, parentPrims, onlyTranslateVisible=True, isAnimated=True, doPostTranslation=True):
        primsToTranslate = []
        for prim in parentPrims:
            primsToTranslate.extend(self.getPrimsToTranslate(prim, onlyTranslateVisible))
        
        if not primsToTranslate:
            logger.error('Failed to find any prims to translate to maya')
            return

        for prim in primsToTranslate:
            primPath = prim.GetPrimPath().pathString
            cmds.AL_usdmaya_TranslatePrim(ip=primPath, fi=True, proxy=proxyShapeName)
            # Selecting the mesh and applying static or animated mesh import is a temporary work around,
            # this will eventually be moved into the AL_usdmaya_TranslatePrim command as an argument
            if doPostTranslation:
                self.createMayaNodes(proxyShapeName, primPath, isAnimated)

    @classmethod
    def getPrimsToTranslate(cls, parentPrim, onlyTranslateVisible):
        prims = []
        for prim in Usd.PrimRange(parentPrim):
            if prim.IsInstance():
                for instancedPrim in prim.GetFilteredChildren(Usd.TraverseInstanceProxies()):
                    prims.extend(cls._getPrimsToTranslate(instancedPrim, onlyTranslateVisible))
            else:
                if prim.GetTypeName() != 'Mesh':
                    continue
                if onlyTranslateVisible and UsdGeom.Imageable(prim).ComputeVisibility() == 'invisible':
                    continue
                prims.extend(list(Usd.PrimRange(prim)))
        return prims

    @classmethod
    def createMayaNodes(cls, proxyShapeName,  primPath, isAnimated):
        dagPath = '{}{}'.format(proxyShapeName, primPath.replace('/', '|'))
        proxyShape = cmds.listConnections('{}.inStageData'.format(dagPath))[0]
        mesh = cmds.listRelatives(dagPath, c=True, type='mesh')[0]
        if cmds.listConnections('{}.inMesh'.format(mesh), s=True):
            return

        if isAnimated:
            usdMaya_meshNode = cmds.createNode('AL_usdmaya_MeshAnimCreator')
            usdMaya_deformerNode = cmds.createNode('AL_usdmaya_MeshAnimDeformer')
            cmds.connectAttr('time1.outTime', '{}.inTime'.format(usdMaya_deformerNode))
            cmds.connectAttr('{}.outMesh'.format(usdMaya_meshNode), '{}.inMesh'.format(usdMaya_deformerNode))
            cmds.connectAttr('{}.outMesh'.format(usdMaya_deformerNode), '{}.inMesh'.format(mesh))
            for node in [usdMaya_meshNode, usdMaya_deformerNode]:
                cmds.setAttr('{}.primPath'.format(node), primPath, type='string')
                cmds.connectAttr('{}.outStageData'.format(proxyShape), '{}.inStageData'.format(node))
        else:
            usdMaya_meshNode = cmds.createNode('AL_usdmaya_MeshAnimCreator')
            cmds.setAttr('{}.primPath'.format(usdMaya_meshNode), primPath, type='string')
            cmds.connectAttr('{}.outStageData'.format(proxyShape), '{}.inStageData'.format(usdMaya_meshNode))
            cmds.connectAttr('{}.outMesh'.format(usdMaya_meshNode), '{}.inMesh'.format(mesh))

