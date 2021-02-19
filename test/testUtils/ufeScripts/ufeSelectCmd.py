#!/usr/bin/env python

#
# Copyright 2019 Autodesk
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

#-
# ===========================================================================
#                       WARNING: PROTOTYPE CODE
#
# The code in this file is intended as an engineering prototype, to demonstrate
# UFE integration in Maya.  Its performance and stability may make it
# unsuitable for production use.
#
# Autodesk believes production quality would be better achieved with a C++
# version of this code.
#
# ===========================================================================
#+

"""Maya UFE selection command.

Operations in Maya are performed on the selection.  This imposes the
requirement that selection in Maya must be an undoable operation, so that
operations after an undo have the proper selection as input.

Maya selections done through non-UFE UIs already go through the undoable select
command.  Selections done through UFE UIs must use the undoable command in this
module.
"""

# Code in this module was initially in the mayaUfe module, which defines
# the initialization and finalization functions for the mayaUfe plugin.
# This confusingly caused two instances of each command class to exist (as
# demonstrated by different values of id() on the class), one used by the
# command creator, and the other by the rest of the Python code.
# Separating out the command code into this module fixed the issue.  PPT,
# 26-Jan-2018.

import maya.api.OpenMaya as OpenMaya

import maya.cmds as cmds

import ufe.PyUfe as PyUfe

# Messages should be localized.
kUfeSelectCmdPrivate = 'Private UFE select command %s arguments not set up.'

def append(item):
    return SelectAppendCmd.execute(item)

def remove(item):
    return SelectRemoveCmd.execute(item)

def clear():
    SelectClearCmd.execute()

def replaceWith(selection):
    SelectReplaceWithCmd.execute(selection)

#==============================================================================
# CLASS SelectCmdBase
#==============================================================================

class SelectCmdBase(OpenMaya.MPxCommand):
    """Base class for UFE selection commands.

    This command is intended as a base class for concrete UFE select commands.
    """

    def __init__(self):
        super(SelectCmdBase, self).__init__()
        self.globalSelection = PyUfe.GlobalSelection.get()

    def isUndoable(self):
        return True

    def validateArgs(self):
        return True

    def doIt(self, args):
        # Completely ignore the MArgList argument, as it's unnecessary:
        # arguments to the commands are passed in Python object form
        # directly to the command's constructor.

        if self.validateArgs() is False:
            self.displayWarning(kUfeSelectCmdPrivate % self.kCmdName)
        else:
            self.redoIt()

#==============================================================================
# CLASS SelectAppendCmd
#==============================================================================

class SelectAppendCmd(SelectCmdBase):
    """Append an item to the UFE selection.

    This command is a private implementation detail of this module and should
    not be called otherwise."""
    
    kCmdName = 'ufeSelectAppend'

    # Command data.  Must be set before creating an instance of the command
    # and executing it.
    item = None

    # Command return data.  Set by doIt().
    result = None

    @staticmethod
    def execute(item):
        """Append the item to the UFE selection, and add an entry to the
           undo queue."""

        # Would be nice to have a guard context to restore the class data
        # to its previous value (which is None).  Not obvious how to write
        # a Python context manager for this, as Python simply binds names
        # to objects in a scope.
        SelectAppendCmd.item = item
        cmds.ufeSelectAppend()
        result = SelectAppendCmd.result
        SelectAppendCmd.item = None
        SelectAppendCmd.result = None
        return result

    @staticmethod
    def creator():
        return SelectAppendCmd(SelectAppendCmd.item)

    def __init__(self, item):
        super(SelectAppendCmd, self).__init__()
        self.item = item
        self.result = None

    def validateArgs(self):
        return self.item is not None

    def doIt(self, args):
        super(SelectAppendCmd, self).doIt(args)
        # Save the result out as a class member.
        SelectAppendCmd.result = self.result

    def redoIt(self):
        self.result = self.globalSelection.append(self.item)

    def undoIt(self):
        if self.result:
            self.globalSelection.remove(self.item)

#==============================================================================
# CLASS SelectRemoveCmd
#==============================================================================

class SelectRemoveCmd(SelectCmdBase):
    """Append an item to the UFE selection.

    This command is a private implementation detail of this module and should
    not be called otherwise."""
    
    kCmdName = 'ufeSelectRemove'

    # Command data.  Must be set before creating an instance of the command
    # and executing it.
    item = None

    # Command return data.  Set by doIt().
    result = None

    @staticmethod
    def execute(item):
        """Remove the item from the UFE selection, and add an entry to the
           undo queue."""

        # See SelectAppendCmd.execute comments.
        SelectRemoveCmd.item = item
        cmds.ufeSelectRemove()
        result = SelectRemoveCmd.result
        SelectRemoveCmd.item = None
        SelectRemoveCmd.result = None
        return result

    @staticmethod
    def creator():
        return SelectRemoveCmd(SelectRemoveCmd.item)

    def __init__(self, item):
        super(SelectRemoveCmd, self).__init__()
        self.item = item
        self.result = None

    def validateArgs(self):
        return self.item is not None

    def doIt(self, args):
        super(SelectRemoveCmd, self).doIt(args)
        # Save the result out as a class member.
        SelectRemoveCmd.result = self.result

    def redoIt(self):
        self.result = self.globalSelection.remove(self.item)

    def undoIt(self):
        # This is not a true undo!  Selection.remove removes an item regardless
        # of its position in the list, but append places the item in the last
        # position.  Saving and restoring the complete list is O(n) for n
        # elements in the list; we want an O(1) solution.  Selection.remove
        # should return the list position of the removed element, for undo O(1)
        # re-insertion using a future Selection.insert().  In C++, this list
        # position would be an iterator; a matching Python binding would most
        # likely require custom pybind code.  PPT, 26-Jan-2018.
        if self.result:
            self.globalSelection.append(self.item)

#==============================================================================
# CLASS SelectClearCmd
#==============================================================================

class SelectClearCmd(SelectCmdBase):
    """Clear the UFE selection.

    This command is a private implementation detail of this module and should
    not be called otherwise."""
    
    kCmdName = 'ufeSelectClear'

    @staticmethod
    def execute():
        """Clear the UFE selection, and add an entry to the undo queue."""
        cmds.ufeSelectClear()

    @staticmethod
    def creator():
        return SelectClearCmd()

    def __init__(self):
        super(SelectClearCmd, self).__init__()
        self.savedSelection = None

    def redoIt(self):
        self.savedSelection = self.globalSelection
        self.globalSelection.clear()

    def undoIt(self):
        self.globalSelection.replaceWith(self.savedSelection)

#==============================================================================
# CLASS SelectReplaceWithCmd
#==============================================================================

class SelectReplaceWithCmd(SelectCmdBase):
    """Replace the UFE selection with a new selection.

    This command is a private implementation detail of this module and should
    not be called otherwise."""
    
    kCmdName = 'ufeSelectReplaceWith'

    # Command data.  Must be set before creating an instance of the command
    # and executing it.
    selection = None

    @staticmethod
    def execute(selection):
        """Replace the UFE selection with a new selection, and add an entry to
        the undo queue."""

        # See SelectAppendCmd.execute comments.
        SelectReplaceWithCmd.selection = selection
        cmds.ufeSelectReplaceWith()
        SelectReplaceWithCmd.selection = None

    @staticmethod
    def creator():
        return SelectReplaceWithCmd(SelectReplaceWithCmd.selection)

    def __init__(self, selection):
        super(SelectReplaceWithCmd, self).__init__()
        self.selection = selection
        self.savedSelection = None

    def redoIt(self):
        # No easy way to copy a Selection, so create a new one and call
        # replaceWith().  selection[:], copy.copy(selection), and
        # copy.deepcopy(selection) all raise Python exceptions.
        self.savedSelection = PyUfe.Selection()
        self.savedSelection.replaceWith(self.globalSelection)
        self.globalSelection.replaceWith(self.selection)

    def undoIt(self):
        self.globalSelection.replaceWith(self.savedSelection)
