#!/usr/bin/env python 

import fixturesUtils
from maya import cmds
from maya import standalone
import mayaUsd.ufe
import mayaUtils
import ufe
import unittest
import usdUtils

def filterUsdStr(usdSceneStr):
    '''Remove empty lines and lines starting with pound character.'''
    nonBlankLines = filter(None, [l.rstrip() for l in usdSceneStr.splitlines()])
    finalLines = [l for l in nonBlankLines if not l.startswith('#')]
    return '\n'.join(finalLines)

def getSessionLayer(context, routingData):
    prim = context.get('prim')
    if prim is None:
        print('Prim not in context')
        return
        
    routingData['layer'] = prim.GetStage().GetSessionLayer().identifier
    
def createUfePathSegment(usdPath):
    """
        Create an UFE path from a given usd path.
        Args:
            usdPath (str): The usd path to use
        Returns :
            PathSegment of the given usdPath
    """
    return ufe.PathSegment(usdPath, mayaUsd.ufe.getUsdRunTimeId(), '/')

class VisibilityCmdTestCase(unittest.TestCase):
     '''Verify the Maya Edit Router for visibility.'''
     pluginsLoaded = False
 
     @classmethod
     def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()
 
     @classmethod
     def tearDownClass(cls):
        standalone.uninitialize()
 
     def setUp(self):
        ''' Called initially to set up the Maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)
        cmds.select(clear=True)

     def testEditRouter(self):
        '''Test edit router functionality.'''

        cmds.file(new=True, force=True)
        import mayaUsd_createStageWithNewLayer

        # Create the following hierarchy:
        #
        # ps
        #  |_ A
        #  |_ B
        #

        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        stage.DefinePrim('/A', 'Xform')
        stage.DefinePrim('/B', 'Xform')

        psPath = ufe.PathString.path(psPathStr)
        psPathSegment = psPath.segments[0]
        aPath = ufe.Path([psPathSegment, usdUtils.createUfePathSegment('/A')])
        bPath = ufe.Path([psPathSegment, usdUtils.createUfePathSegment('/B')])
        a = ufe.Hierarchy.createItem(aPath)
        b = ufe.Hierarchy.createItem(bPath)

        # Select /A
        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(a)

        # Hide A
        cmds.hide()

        # Get the session layer
        prim = mayaUsd.ufe.ufePathToPrim("|stage1|stageShape1,/A")
        sessionLayer = prim.GetStage().GetSessionLayer()

        # Check that the session layer is empty
        self.assertTrue(sessionLayer.empty)

        # Register visibility
        mayaUsd.lib.registerEditRouter('visibility', getSessionLayer)

        # Select /B
        sn = ufe.GlobalSelection.get()
        sn.clear()
        sn.append(b)

        # Hide B
        cmds.hide()

        # Check that something was written to the session layer
        self.assertIsNotNone(sessionLayer)
        self.assertIsNotNone(sessionLayer.GetPrimAtPath('/B'))

        # Check that any visibility changes were written to the session layer
        self.assertIsNotNone(sessionLayer.GetAttributeAtPath('/B.visibility').default)

        # Check that correct visibility changes were written to the session layer
        self.assertEqual(filterUsdStr(sessionLayer.ExportToString()),
            'over "B"\n{\n    token visibility = "invisible"\n}')

if __name__ == '__main__':
 unittest.main(verbosity=2)
