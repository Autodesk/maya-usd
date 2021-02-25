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

import maya.cmds as cmds

def createStageWithNewLayer():
    """For use in aggregating USD (Assembly) and creating layer structures (Layout)
    users need to be able to create a new empty stage.

    Executing this command should produce the following:
    - Proxyshape
    - Stage
    - Session Layer
    - Anonymous Root Layer (this is set as the target layer)
    """

    # Simply create a proxy shape. Since it does not have a USD file associated
    # (in the .filePath attribute), the proxy shape base will create an empty
    # stage in memory. This will create the session and root layer as well.
    shapeNode = cmds.createNode('mayaUsdProxyShape', skipSelect=True, name='stageShape1')
    cmds.connectAttr('time1.outTime', shapeNode+'.time')
    cmds.select(shapeNode, replace=True)
    fullPath = cmds.ls(shapeNode, long=True)
    return fullPath[0]
