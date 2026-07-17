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

import maya.cmds as cmds

from pxr import Sdf, Usd

from AdskUsdComponentCreator import (
    Options,
    GenerateUniqueComponentName,
    GenerateVariantSetNameFromObjectName,
    GenerateVariantNameFromObjectName,
    TheHost,
    CreateFromStageCommand,
)
from usd_component_creator_plugin import (
    print_exceptions,
    MayaUndoChunk,
    execute_ufe_command,
    CreateProxyShapeForComponentCommand,
    AddComponentToManagerCommand,
    OpenComponentCommand,
    show_create_component_options,
)


def _build_options():
    """
    Create fresh, validated Options with a unique component name/folder
    and an initial variant set/variant.
    """
    options = Options()
    options.Validate()
    options.component_folder = cmds.workspace(expandName=cmds.internalVar(userTmpDir=True))
    options = GenerateUniqueComponentName(options)

    component_name = options.component_name
    options.component_variants = [
        (
            GenerateVariantSetNameFromObjectName(options, '', []),
            GenerateVariantNameFromObjectName(options, '', [], []),
        ),
    ]
    options.Validate()
    return options


@print_exceptions('Failed to create the USD component')
def createComponent():
    """
    Create a new, empty Autodesk USD Component from a temporary anonymous
    in-memory stage, add it to the component manager, and open it.
    """
    options = _build_options()

    # Include the whole (empty) stage root.
    options.included_paths = [Sdf.Path.absoluteRootPath]
    options.replace_variant_content = True

    stage = Usd.Stage.CreateInMemory()

    with MayaUndoChunk('Create USD Component'):
        # Single creation pass: no UnlockComponentContext needed.
        create_cmd = CreateFromStageCommand(None, stage, options)
        if not TheHost.GetHost().ExecuteWithUndo(create_cmd):
            print('Failed to create the empty USD component.')
            return

        proxy_cmd = CreateProxyShapeForComponentCommand(create_cmd, target_default_variant=True)
        execute_ufe_command(proxy_cmd)

        add_cmd = AddComponentToManagerCommand(create_cmd)
        execute_ufe_command(add_cmd)

        open_cmd = OpenComponentCommand(create_cmd)
        execute_ufe_command(open_cmd)


def createComponentOptions():
    """Open the existing Component Creator options dialog wired to createComponent."""
    show_create_component_options('Create USD Component', createComponent, show_export_options=False)
