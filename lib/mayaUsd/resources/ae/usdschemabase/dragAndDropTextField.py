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

import mayaUsdUtils


class DragAndDropTextField:
    '''
    Class to hold the UI elements for a text field supporting drag-and-drop.
    '''
    def __init__(self, uiTooltip):
        self.field = cmds.textField(annotation=uiTooltip, editable=False, enableKeyboardFocus=True)
        self.lastValue = ''

    def fillUI(self, text, placeholder=None, annotation=None, editable=True):
        '''
        Fill the text field with the given text, optional placeholder and optional annotation.
        '''
        try:
            self.lastValue = text
            args = {
                'editable': editable,
                'text': text
            }
            if placeholder:
                args['placeholderText'] = placeholder
            if annotation:
                args['annotation'] = annotation
            cmds.textField(self.field, edit=True, **args)
        except Exception as e:
            cmds.warning(f'Error filling text field values: {e}', noContext=True)

    def setChangeCallback(self, callback, undoLabel):
        '''
        Set the callback to be called when the text field value is modified by user and confirmed
        by either pressing Enter or losing focus. The callback receives the new value.
        '''
        @mayaUsdUtils.setUndoLabel(undoLabel)
        def changeCallback(value, *args, **kwargs):
            try:
                callback(value)
            except Exception as e:
                cmds.warning(f'Error in text field change callback: {e}', noContext=True)
        try:
            cmds.textField(self.field, edit=True, changeCommand=changeCallback)
        except Exception as e:
            cmds.warning(f'Error connecting text field change callback: {e}', noContext=True)

    def _validateDroppedValue(self, value, validation):
        '''
        Try to detect drag-and-drop.

        In Maya, when text is dropped, it is inserted in the middle of the existing text.
        What we want is replacement, not insertion. So we try to detect that and extract
        the dropped value from the new text. Unfortunately, there may be corner cases
        where we cannot be sure what is teh new text. For example, if the previous text
        can be found multiple times in the new text, we cannot be sure which one was replaced.
        We extract all possible new values and let the validation function decide which one is valid.
        '''
        # If the last value is the same as the new value, it is not a drop.
        if self.lastValue == value:
            return None
        # If the new value is shorter than the last value, it is not a drop.
        # If teh new value is just ne character longer, then we assume the user is typing and not dropping.
        if len(value) <= len(self.lastValue) + 1:
            return None

        newValues = []
        lenToExtract = len(value) - len(self.lastValue)
        for insertionPoint in range(len(value) - lenToExtract + 1):
            potentialOldValue = value[:insertionPoint] + value[insertionPoint + lenToExtract:]
            if potentialOldValue == self.lastValue:
                newValues.append(value[insertionPoint:insertionPoint + lenToExtract])

        potentialNewValue = validation(self.lastValue, newValues)
        if potentialNewValue is not None:
            return potentialNewValue
        
        return value

    def setImmediateChangeCallback(self, validation, callback, undoLabel):
        '''
        Set callback for immediate text field value change, such as when the user is typing or dropping text.
        The validation function is called to validate the new value and determine if it should be accepted.
        If the validation function returns a new value (i.e not None), it will be used; otherwise, the original
        new value will be used. Then the callback is called with the new value and a boolean indicating if it
        was a dropped value.

        The validation function receives the last value and a list of potential new values extracted from the
        current text field value.

        The callback function receives the new value and a boolean indicating if it was a dropped value.
        '''
        @mayaUsdUtils.setUndoLabel(undoLabel)
        def validationCallback(value, *args, **kwargs):
            try:
                droppedValue = self._validateDroppedValue(value, validation)
                newValue = droppedValue if droppedValue is not None else value
                self.lastValue = newValue
                callback(newValue, bool(droppedValue is not None))
            except Exception as e:
                cmds.warning(f'Error in text field validation: {e}', noContext=True)

        try:
            cmds.textField(self.field, edit=True, textChangedCommand=validationCallback)
        except Exception as e:
            cmds.warning(f'Error connecting text field change callback: {e}', noContext=True)
