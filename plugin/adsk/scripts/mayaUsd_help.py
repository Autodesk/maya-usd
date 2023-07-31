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

def showHelpMayaUSD(contentId):
    """
    Helper method to display help content.

    Note that this is a wrapper around Maya's showHelp() method and showHelpMayaUSD()
    should be used for all help contents in Maya USD.

    Example usage of this method:
    
    - In Python scripts:
    from mayaUsd_help import *
    showHelpMayaUSD("someContentId");

    - In MEL scripts:
    python(\"from mayaUsd_help import *; showHelpMayaUSD('someContentId');\")
    
    - In C++:
    MGlobal::executePythonCommand(
    "from mayaUsd_help import *; showHelpMayaUSD(\"someContentId\");");

    Input contentId refers to the contentId that is registered in helpTableMayaUSD
    file which is used to open help pages.
    """
    import os
    import maya.mel as mel
    # Finding the path to helpTableMayaUSD file.
    dirName = mel.eval('pluginInfo -q -p mayaUsdPlugin')
    dirName = os.path.dirname(dirName) + "/../helpTable/" + "helpTableMayaUSD"
    # Setting the default helpTable to helpTableMayaUSD
    mel.eval('showHelp -helpTable "%s"; showHelp "%s";'% (dirName,contentId))
    # Restoring Maya's default helpTable
    mel.eval('showHelp -helpTable "helpTable"')
