from pymel.core import *
import maya.cmds as cmds
import tempfile
import os

def main():
    tmpdir = tempfile.gettempdir()
    scn_dir = os.path.dirname(os.path.realpath(__file__))
    
    cmds.loadPlugin('mayaUsdPlugin')
    cmds.loadPlugin('pxrUsd')
    cmds.file('{}/UsdExportRootTest.ma'.format(scn_dir), type="mayaAscii", o=True)
   
    
    select(['pCone1', 'Cube1'], r=True)
    cmds.usdExport(file='{}/only_set1_to_top.usda'.format(tmpdir), sl=True, root="Top")
    cmds.usdExport(file='{}/only_set1_to_Grp1.usda'.format(tmpdir), sl=True, root="GrpRoot1")
    cmds.usdExport(file='{}/only_set1_noParents.usda'.format(tmpdir), sl=True, root=[str(Xf) for Xf in ls(sl=True)])

    select(['pCone1', 'Cube1', 'pCone2', 'Cube2'], r=True)
    cmds.usdExport(file='{}/set1n2_to_top.usda'.format(tmpdir), sl=True, root="Top")
    cmds.usdExport(file='{}/set1n2_noParents.usda'.format(tmpdir), sl=True, root=[str(Xf) for Xf in ls(sl=True)])
    
    select(['pCone1'], r=True)
    cmds.usdExport(file='{}/cone1_to_Mid_NoTrans.usda'.format(tmpdir), sl=True, root="Mid_NoTransformation")

    # no selection, only roots
    cmds.usdExport(file='{}/onlyRootsA.usda'.format(tmpdir), root=['Mid_Transformation', 'Mid_NoTransformation'])
    cmds.usdExport(file='{}/onlyRootsB.usda'.format(tmpdir), root=['Mid_1', 'Mid_Transformation'])

    # no root, only selection
    select(['Cube1', 'pSphere2', 'pCone2'], r=True)
    cmds.usdExport(file='{}/onlySelected.usda'.format(tmpdir), sl=True)
    cmds.usdExport(file='{}/onlySelectedOldBehavior.usda'.format(tmpdir), sl=True, root='|')
    cmds.usdExport(file='{}/mixedRootsAndSelRootsA.usda'.format(tmpdir), sl=True, root=['Mid_Transformation', 'Mid_NoTransformation'])
    cmds.usdExport(file='{}/mixedRootsAndSelRootsB.usda'.format(tmpdir), sl=True, root=['Mid_Transformation', 'Mid_1'])
    cmds.usdExport(file='{}/mixedRootsAndSelRootsC.usda'.format(tmpdir), sl=True, root=['Mid_NoTransformation', 'Mid_Transformation', 'Mid_1'])
    
    # neither root nor selected
    cmds.usdExport(file='{}/export_all.usda'.format(tmpdir))

    # select(['GrpRoot2|Conflicting1', 'GrpRoot2|Conflicting2', 'GrpRoot3|Conflicting1', 'GrpRoot3|Conflicting2'], r=True)
    # cmds.usdExport(file='{}/conflictingNames.usda'.format(tmpdir), sl=True, root=[str(Xf) for Xf in ls(sl=True)])
    # '''

main()
