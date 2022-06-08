#!/usr/bin/env mayapy
#
# Copyright 2022 Autodesk
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

from maya import cmds
import maya.OpenMayaUI as mui

import fixturesUtils

import unittest

import ufe
import mayaUsd
from pxr import Usd, Tf

from shiboken2 import wrapInstance
from PySide2.QtGui import QIcon

class UIIconsTestCase(unittest.TestCase):
    """
    Tests that all prims have the icon file we expect.
    - Either directly from the UIInfoHandler
    - Or from the ancestor node types
    """

    @classmethod
    def setUpClass(cls):
        inputPath = fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)

    def setUp(self):
        # Start with just empty stage
        cmds.file(force=True, new=True)
        import mayaUsd_createStageWithNewLayer
        self.proxyShapePathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        self.assertIsNotNone(self.proxyShapePathStr)
        self.stage = mayaUsd.lib.GetPrim(self.proxyShapePathStr).GetStage()
        self.assertIsNotNone(self.stage)

        rid = ufe.RunTimeMgr.instance().getId('USD')
        self.ufeUIInfo = ufe.UIInfoHandler.uiInfoHandler(rid)
        self.assertIsNotNone(self.ufeUIInfo)
        self.baseIconName = 'out_' + ufe.RunTimeMgr.instance().getName(rid) + '_';

    @classmethod
    def tearDownClass(cls):
        pass

    def _createIconFromFile(self, iconName):
        swigIcon = mui.MQtUtil.createIcon(iconName)
        if swigIcon:
            return wrapInstance(int(swigIcon), QIcon)
        return None

    def _getUIInfoIcon(self, primType):
        primPath = ufe.PathString.path('%s,/%s' % (self.proxyShapePathStr, primType))
        primItem = ufe.Hierarchy.createItem(primPath)
        self.assertIsNotNone(primItem, msg='for prim path "%s"' % primPath)
        ufeIcon = self.ufeUIInfo.treeViewIcon(primItem)
        return ufeIcon.baseIcon if ufeIcon.baseIcon else ''

    def _getAncestorNodeIcon(self, primType):
        primPath = ufe.PathString.path('%s,/%s' % (self.proxyShapePathStr, primType))
        primItem = ufe.Hierarchy.createItem(primPath)
        for ty in primItem.ancestorNodeTypes():
            iconName = self.baseIconName + ty + '.png'
            icon = self._createIconFromFile(iconName)
            if icon:
                return iconName
        return ''

    def testUIIcons(self):
        # All the prim types to test
        usdVer = Usd.GetVersion()
        primTypes = [
            # Prim Type                 # Icon file name
            ('ALMayaReference',         'out_USD_MayaReference.png'),
            ('Backdrop',                'out_USD_UsdTyped.png'),
            ('BasisCurves',             'out_USD_UsdGeomCurves.png'),
            ('BlendShape',              'out_USD_BlendShape.png'),
            ('Camera',                  'out_USD_Camera.png'),
            ('Capsule',                 'out_USD_Capsule.png'),
            ('Cone',                    'out_USD_Cone.png'),
            ('Cube',                    'out_USD_Cube.png'),
            ('Cylinder',                'out_USD_Cylinder.png'),
            ('CylinderLight',           'out_USD_UsdLuxBoundableLightBase.png' if usdVer >= (0, 21, 11) else 'out_USD_UsdLuxLight.png'),
            ('DiskLight',               'out_USD_UsdLuxBoundableLightBase.png' if usdVer >= (0, 21, 11) else 'out_USD_UsdLuxLight.png'),
            ('DistantLight',            'out_USD_UsdLuxNonboundableLightBase.png' if usdVer >= (0, 21, 11) else 'out_USD_UsdLuxLight.png'),
            ('DomeLight',               'out_USD_UsdLuxNonboundableLightBase.png' if usdVer >= (0, 21, 11) else 'out_USD_UsdLuxLight.png'),
            ('Field3DAsset',            'out_USD_UsdGeomXformable.png'),
            ('GeomSubset',              'out_USD_GeomSubset.png'),
            ('GeometryLight',           'out_USD_UsdLuxNonboundableLightBase.png' if usdVer >= (0, 21, 11) else 'out_USD_UsdLuxLight.png'),
            ('HermiteCurves',           'out_USD_UsdGeomCurves.png'),
            ('LightFilter',             'out_USD_LightFilter.png'),
            ('Material',                'out_USD_UsdTyped.png'),
            ('MayaReference',           'out_USD_MayaReference.png'),
            ('Mesh',                    'out_USD_Mesh.png'),
            ('NodeGraph',               'out_USD_UsdTyped.png'),
            ('NurbsCurves',             'out_USD_UsdGeomCurves.png'),
            ('NurbsPatch',              'out_USD_NurbsPatch.png'),
            ('OpenVDBAsset',            'out_USD_UsdGeomXformable.png'),
            ('PackedJointAnimation',    'out_USD_SkelAnimation.png'),
            ('PluginLight',             'out_USD_PluginLight.png' if usdVer >= (0, 21, 11) else 'out_USD_UsdLuxLight.png'),
            ('PluginLightFilter',       'out_USD_LightFilter.png'),
            ('PointInstancer',          'out_USD_PointInstancer.png'),
            ('Points',                  'out_USD_Points.png'),
            ('PortalLight',             'out_USD_UsdLuxBoundableLightBase.png' if usdVer >= (0, 21, 11) else 'out_USD_UsdLuxLight.png'),
            ('RectLight',               'out_USD_UsdLuxBoundableLightBase.png' if usdVer >= (0, 21, 11) else 'out_USD_UsdLuxLight.png'),
            ('RenderProduct',           'out_USD_UsdTyped.png'),
            ('RenderSettings',          'out_USD_UsdTyped.png'),
            ('RenderVar',               'out_USD_UsdTyped.png'),
            ('Scope',                   'out_USD_Scope.png'),
            ('Shader',                  'out_USD_UsdTyped.png'),
            ('SkelAnimation',           'out_USD_SkelAnimation.png'),
            ('SkelRoot',                'out_USD_SkelRoot.png'),
            ('Skeleton',                'out_USD_Skeleton.png'),
            ('SpatialAudio',            'out_USD_UsdGeomXformable.png'),
            ('Sphere',                  'out_USD_Sphere.png'),
            ('SphereLight',             'out_USD_UsdLuxBoundableLightBase.png' if usdVer >= (0, 21, 11) else 'out_USD_UsdLuxLight.png'),
            ('Volume',                  'out_USD_Volume.png'),
            ('Xform',                   'out_USD_UsdGeomXformable.png')
        ]
        if usdVer >= (0, 21, 11):
            primTypes.extend([
                ('PhysicsCollisionGroup',   'out_USD_UsdTyped.png'),
                ('PhysicsDistanceJoint',    'out_USD_UsdTyped.png'),
                ('PhysicsFixedJoint',       'out_USD_UsdTyped.png'),
                ('PhysicsJoint',            'out_USD_UsdTyped.png'),
                ('PhysicsPrismaticJoint',   'out_USD_UsdTyped.png'),
                ('PhysicsRevoluteJoint',    'out_USD_UsdTyped.png'),
                ('PhysicsScene',            'out_USD_UsdTyped.png'),
                ('PhysicsSphericalJoint',   'out_USD_UsdTyped.png')
            ])

        # Special case for node types which are in an AL schema.
        # They aren't available when compiling without the AL plugin.
        if Usd.SchemaRegistry.IsConcrete(Tf.Type.FindByName('AL_usd_FrameRange')):
            primTypes.extend([
                ('ALExamplePolyCubeNode',   'out_USD_UsdTyped.png'),
                ('ALFrameRange',            'out_USD_UsdTyped.png')
        ])

        for ty,iname in primTypes:
            prim = self.stage.DefinePrim('/%s' % ty, ty)
            ufeIcon = self._getUIInfoIcon(ty)
            if ufeIcon:
                self.assertEqual(ufeIcon, iname, msg='for prim type "%s"' % ty)
            else:
                iconFile = self._getAncestorNodeIcon(ty)
                self.assertEqual(iconFile, iname, msg='for prim type "%s"' % ty)

if __name__ == '__main__':
    fixturesUtils.runTests(globals())
