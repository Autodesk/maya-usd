import maya.api.OpenMaya as om

from pxr import Gf

def getMayaNodeTransform(transformName):
    '''Retrieve the transformation object (MFnTransform) of a Maya node.'''
    selectionList = om.MSelectionList()
    selectionList.add(transformName)
    node = selectionList.getDependNode(0)
    return om.MFnTransform(node)


def getMayaNodeMatrix(transformName):
    '''Retrieve the matrix (MMatrix) of a Maya node.'''
    mayaTransform = getMayaNodeTransform(transformName)
    transformation = mayaTransform.transformation()
    return transformation.asMatrix()


def getMayaMatrixRotation(matrix):
    '''Extract the three local rotation angles (XYZ) from a Maya matrix.'''
    return om.MTransformationMatrix(matrix).rotation()


def getMayaNodeRotation(transformName):
    '''Extract the three local rotation angles (XYZ) from a Maya node.'''
    return getMayaMatrixRotation(getMayaNodeMatrix(transformName))


def getMayaMatrixScaling(matrix):
    '''Extract the scaling from a Maya matrix.'''
    return om.MTransformationMatrix(matrix).scale(om.MSpace.kObject)


def getMayaNodeScaling(transformName):
    '''Extract the scaling from a Maya node.'''
    return getMayaMatrixScaling(getMayaNodeMatrix(transformName))


def assertPrimXforms(test, prim, xforms):
    '''
    Verify that the prim has the given xform in the roder given.
    The verifications are done by assertions on the given test.

    The xforms should be a list of pairs, each containing the
    xform op name and its value.

    For example:
        [
            ('xformOp:translate', (0., 0., 30.)),
            ('xformOp:scale', (10., 10., 10.))
        ]
    '''
    EPSILON = 1e-5
    xformOpOrder = prim.GetAttribute('xformOpOrder').Get()
    test.assertEqual(len(xformOpOrder), len(xforms), "%s != %s" % (xformOpOrder, xforms))
    for name, value in xforms:
        test.assertEqual(xformOpOrder[0], name)
        if value is not None:
            attr = prim.GetAttribute(name)
            test.assertIsNotNone(attr, "Attribute %s not found on prim %s" % (name, prim.GetPath()))
            attrValue = attr.Get()
            test.assertTrue(Gf.IsClose(attrValue, value, EPSILON), "%s does not match: %s != %s" % (name, attrValue, value))
        # Chop off the first xofrm op for the next loop.
        xformOpOrder = xformOpOrder[1:]


def assertStagePrimXforms(test, stage, path, xforms):
    '''
    Verify that the prim at the given path in the given stage
    has the given xform in the roder given. The verifications
    are done by assertions on the given test.
    
    The xforms should be a list of pairs, each containing the
    xform op name and its value.

    For example:
        [
            ('xformOp:translate', (0., 0., 30.)),
            ('xformOp:scale', (10., 10., 10.))
        ]
    '''
    prim = stage.GetPrimAtPath(path)
    return assertPrimXforms(test, prim, xforms)

