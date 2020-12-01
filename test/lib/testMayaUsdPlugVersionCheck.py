#!/usr/bin/env python

#
# Copyright 2020 Autodesk
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

import unittest

import mayaUtils
from pxr import Plug, Tf

class MayaUsdPlugVersionCheckTestCase(unittest.TestCase):
    """
    Verify that only plugins passing version checks are loaded
    """
    
    def testVersionCheck(self):
        # Useful for debugging accessor
        Tf.Debug.SetDebugSymbolsByName("USDMAYA_PLUG_INFO_VERSION", 1)
        
        # Loading the plugin will trigger registration of plugins from MAYA_PXR_PLUGINPATH_NAME
        # path
        mayaUtils.loadPlugin("mayaUsdPlugin")

        # All test plugins derive from _TestPlugBase<1>
        base1Subclasses = Tf.Type.FindByName('_TestPlugBase<1>').GetAllDerivedTypes()
        
        # All checkes should pass for testPlugModule1,2,3 plugin paths based on
        # mayaUsdPluginInfo.json
        self.assertIn('TestPlugModule1.TestPlugPythonDerived1', base1Subclasses)
        self.assertIn('TestPlugModule2.TestPlugPythonDerived2', base1Subclasses)
        self.assertIn('TestPlugModule3.TestPlugPythonDerived3', base1Subclasses)
        # Following plugins should never get register since they shouldn't pass version check
        with self.assertRaises(TypeError):
            ppd4 = Plug.Registry().GetPluginForType('TestPlugModule4.TestPlugPythonDerived4')
        with self.assertRaises(TypeError):
            ppd5 = Plug.Registry().GetPluginForType('TestPlugModule5.TestPlugPythonDerived5')
        with self.assertRaises(TypeError):
            ppd6 = Plug.Registry().GetPluginForType('TestPlugModule6.TestPlugPythonDerived6')
        
        
