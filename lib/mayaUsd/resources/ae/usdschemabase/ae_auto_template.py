# Copyright 2023 Autodesk
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

class AEUITemplate:
    '''
    Helper class to push/pop the Attribute Editor Template. This makes
    sure that controls are aligned properly.
    '''

    def __enter__(self):
        cmds.setUITemplate('attributeEditorTemplate', pst=True)
        return self

    def __exit__(self, mytype, value, tb):
        cmds.setUITemplate(ppt=True)

