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


class DummyLocatorWriter(mayaUsd.lib.PrimWriter):
    """
    Dummy locator PrimWriter.
    It is used to verify plugin discovery and loading, and writes noting.
    """
    @classmethod
    def CanExport(cls, exportArgs, exportObj=None):
        return mayaUsd.lib.PrimWriter.ContextSupport.Unsupported


def initializePlugin(mobject):
    maya.api.OpenMaya.MFnPlugin(mobject, 'Autodesk', '1.0', 'Any')
    mayaUsd.lib.PrimWriter.Register(DummyLocatorWriter, 'locator')


def uninitializePlugin(mobject):
    maya.api.OpenMaya.MFnPlugin(mobject, 'Autodesk', '1.0', 'Any')
