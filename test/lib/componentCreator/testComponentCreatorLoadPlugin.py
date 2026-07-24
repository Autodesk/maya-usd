import os
import unittest

import fixturesUtils
import mayaUtils
from maya import cmds

from testComponentCreatorUtils import _CC_AVAILABLE, _HAVE_CC_MAYA_PLUGIN

class LoadComponentCreatorPluginTestCase(unittest.TestCase):
    """
    Verify that the USD component creator plugin can be loaded.
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)

    def testPluginLoadable(self):
        forceTest = os.environ.get('MAYAUSD_FORCE_CC_TEST', '0') == '1'
        if not _CC_AVAILABLE:
            if forceTest:
                self.fail('AdskUsdComponentCreator was not found but MAYAUSD_FORCE_CC_TEST is set.')
            else:
                self.skipTest('Could not find the AdskUsdComponentCreator.')

        # Both cases rely on the MayaUsd plugin being loaded first.
        self.assertTrue(mayaUtils.loadPlugin('mayaUsdPlugin'))
        if _HAVE_CC_MAYA_PLUGIN:
            self.assertTrue(mayaUtils.loadPlugin('usd_component_creator'))
        else:
            # In Maya at startup the idle events are suspended during initialization to prevent
            # plugins from being loaded before the UI is initialized.
            #
            # In an interactive test the startup command (for the test) is called before Maya
            # resumes the idle event processing. So we need to force resume it here and flush
            # the idle queue to ensure that the MayaUsd plugin UI Creation script is called
            # which is where the Component Creator initialization is done.
            #
            cmds.flushIdleQueue(resume=True)
            cmds.flushIdleQueue()

            # The Component Creator Maya plugin was removed and replace with a direct CC
            # initialization. We make sure CC was init correctly.
            from usd_component_creator_plugin import is_initialized as is_initializedCC
            is_cc_init = is_initializedCC()
            self.assertTrue(is_cc_init, "Component Creator was not initialized but MAYAUSD_FORCE_CC_TEST is set.")




if __name__ == '__main__':
    fixturesUtils.runTests(globals())

