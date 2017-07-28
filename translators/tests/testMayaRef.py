import unittest

import maya.standalone
import maya.cmds as mc

from pxr import Tf

class testTranslatorsPlugin(unittest.TestCase):

	@classmethod
	def setUpClass(cls):
		maya.standalone.initialize()
		mc.loadPlugin('AL_USDMayaPlugin')

	@classmethod
	def tearDown(cls):
		maya.standalone.uninitialize()

	def testPluginIsFunctional(self):
		mc.AL_usdmaya_ProxyShapeImport(file='./testMayaRef.usda')
		self.assertTrue(Tf.Type.Unknown != Tf.Type.FindByName('AL::usdmaya::fileio::translators::MayaReference'))
		self.assertTrue(len(mc.ls('cube:pCube1')))

if __name__ == '__main__':
    unittest.main()
