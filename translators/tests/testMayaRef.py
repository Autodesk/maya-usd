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
		self.assertEqual(1, len(mc.ls('cube:pCube1')))
		self.assertEqual('mayaRefPrim', mc.listRelatives('cube:pCube1', parent=1)[0])
		self.assertEqual((0.0, 0.0, 0.0), cmds.getAttr('mayaRefPrim.translate')[0])
		self.assertFalse(mc.getAttr('mayaRefPrim.translate', lock=1))
		self.assertEqual(1, len(mc.ls('otherNS:pCube1')))
		self.assertEqual('otherCube', mc.listRelatives('otherNS:pCube1', parent=1)[0])
		self.assertEqual((3.0, 2.0, 1.0), cmds.getAttr('otherCube.translate')[0])
		self.assertFalse(mc.getAttr('otherCube.translate', lock=1))

if __name__ == '__main__':
    unittest.main()
