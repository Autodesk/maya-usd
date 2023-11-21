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
from pxr import Sdf, Usd, Vt
from mayaUsd import schemas as mayaUsdSchemas


class MayaUsdSchemasMayaReferenceTestCase(unittest.TestCase):
    """
    Verify MayaReference schema and attributes
    """

    def testMayaReferenceAttributes(self):
        layer = Sdf.Layer.CreateAnonymous()
        stageOut = Usd.Stage.Open(layer.identifier)

        primPath = Sdf.AssetPath('/MayaReference')
        mayaReferencePath = '/somewherenice/path.ma'
        mayaNamespace = 'nsp'

        primOut = stageOut.DefinePrim(primPath.path, 'MayaReference')
        self.assertTrue(primOut.IsValid())

        mayaReferenceOut = mayaUsdSchemas.MayaReference(primOut)
        self.assertTrue(mayaReferenceOut.GetPrim())
        typeName = Usd.SchemaRegistry().GetSchemaTypeName(mayaReferenceOut._GetStaticTfType())
        self.assertEqual(typeName, 'MayaReference')

        mayaReferenceAttr = primOut.CreateAttribute('mayaReference', Sdf.ValueTypeNames.Asset)
        mayaReferenceAttr.Set(mayaReferencePath)

        mayaNamespaceAttr = primOut.CreateAttribute('mayaNamespace', Sdf.ValueTypeNames.String)
        mayaNamespaceAttr.Set(mayaNamespace)

        stageIn = Usd.Stage.Open(stageOut.Flatten());
        primIn = stageIn.GetPrimAtPath(primPath.path)
        self.assertTrue(primIn.IsValid())

        mayaReferenceIn = mayaUsdSchemas.MayaReference(primIn)
        self.assertTrue(mayaReferenceIn.GetMayaReferenceAttr().Get(),mayaReferencePath)
        self.assertTrue(mayaReferenceIn.GetMayaNamespaceAttr().Get(),mayaNamespace)
