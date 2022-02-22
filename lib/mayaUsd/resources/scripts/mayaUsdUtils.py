from maya.api import OpenMaya as om

import mayaUsd

import ufe

def getPulledInfo(dagPath):
    """
    Retrieves the full DAG path, UFE hierarchy item, FE path of the USD pulled path and the USD prim.
    The USD pulled path and USD prim may be invalid if the pulled object is orphaned.
    """
    # The dagPath must have the pull information that brings us to the USD
    # prim.  If this prim is of type Maya reference, create a specific menu
    # item for it, otherwise return an empty string for the chain of
    # responsibility.

    # The dagPath may come in as a shortest unique name.  Expand it to full
    # name; ls -l would be an alternate implementation.
    sn = om.MSelectionList()
    sn.add(dagPath)
    fullDagPath = sn.getDagPath(0).fullPathName()
    mayaItem = ufe.Hierarchy.createItem(ufe.PathString.path(fullDagPath))
    pathMapper = ufe.PathMappingHandler.pathMappingHandler(mayaItem)
    pulledPath = pathMapper.fromHost(mayaItem.path())
    prim = mayaUsd.ufe.ufePathToPrim(ufe.PathString.string(pulledPath))
    return fullDagPath, mayaItem, pulledPath, prim


def isPulledMayaReference(dagPath):
    """
    Verifies if the DAG path refers to a pulled prim that is a Maya reference.
    """
    _, _, _, prim = getPulledInfo(dagPath)
    return prim and prim.GetTypeName() == 'MayaReference'
