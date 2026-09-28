#!/usr/bin/env python
#
# Copyright 2026 Autodesk
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

import maya.api.OpenMaya
import mayaUsd.lib


def maya_useNewAPI():
    pass


class DummyXformReader(mayaUsd.lib.PrimReader):
    """
    Dummy UsdGeomXform PrimReader.
    It is used to verify plugin discovery and loading, and reads noting.
    """
    @classmethod
    def CanImport(cls, importArgs, importPrim):
        return mayaUsd.lib.PrimReader.ContextSupport.Unsupported

    def Read(self, context):
        return False


def initializePlugin(mobject):
    maya.api.OpenMaya.MFnPlugin(mobject, 'Autodesk', '1.0', 'Any')
    mayaUsd.lib.PrimReader.Register(DummyXformReader, 'UsdGeomXform')


def uninitializePlugin(mobject):
    maya.api.OpenMaya.MFnPlugin(mobject, 'Autodesk', '1.0', 'Any')
