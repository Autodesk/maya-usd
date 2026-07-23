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

"""
Qt mouse input helpers for tests. Prefer QMouseEvent + QApplication.sendEvent
over QTest.mouse* so behavior matches direct event delivery.
"""

try:
    from PySide2 import QtCore
    from PySide2.QtGui import QMouseEvent
    from PySide2.QtWidgets import QApplication
except ImportError:
    from PySide6 import QtCore
    from PySide6.QtGui import QMouseEvent
    from PySide6.QtWidgets import QApplication


def _mouse_event_types():
    """Return (press, move, release) QEvent type values for PySide2 and PySide6."""
    qe = QtCore.QEvent
    if hasattr(qe, 'MouseButtonPress'):
        return (qe.MouseButtonPress, qe.MouseMove, qe.MouseButtonRelease)
    qe_type = qe.Type
    return (qe_type.MouseButtonPress, qe_type.MouseMove, qe_type.MouseButtonRelease)

_PRESS_TYPE, _MOVE_TYPE, _RELEASE_TYPE = _mouse_event_types()

def send_mouse_click(view_widget, local_pos, modifiers=None):
    """
    Deliver a left-button click at local_pos (QPoint) on view_widget.
    """
    if modifiers is None:
        modifiers = QtCore.Qt.KeyboardModifier.NoModifier
    global_pos = view_widget.mapToGlobal(local_pos)
    press_event = QMouseEvent(
        _PRESS_TYPE,
        local_pos,
        global_pos,
        QtCore.Qt.MouseButton.LeftButton,
        QtCore.Qt.MouseButton.LeftButton,
        modifiers,
    )
    release_event = QMouseEvent(
        _RELEASE_TYPE,
        local_pos,
        global_pos,
        QtCore.Qt.MouseButton.LeftButton,
        QtCore.Qt.MouseButton.NoButton,
        modifiers,
    )
    QApplication.sendEvent(view_widget, press_event)
    QApplication.sendEvent(view_widget, release_event)

def send_mouse_drag(view_widget, press_local_pos, release_local_pos, modifiers=None):
    """
    Deliver press -> move -> release for a left-button drag, matching
    QApplication::sendEvent(press); sendEvent(move); sendEvent(release).
    """
    if modifiers is None:
        modifiers = QtCore.Qt.KeyboardModifier.NoModifier
    press_global = view_widget.mapToGlobal(press_local_pos)
    release_global = view_widget.mapToGlobal(release_local_pos)
    press_event = QMouseEvent(
        _PRESS_TYPE,
        press_local_pos,
        press_global,
        QtCore.Qt.MouseButton.LeftButton,
        QtCore.Qt.MouseButton.LeftButton,
        modifiers,
    )
    move_event = QMouseEvent(
        _MOVE_TYPE,
        release_local_pos,
        release_global,
        QtCore.Qt.MouseButton.NoButton,
        QtCore.Qt.MouseButton.NoButton,
        modifiers,
    )
    release_event = QMouseEvent(
        _RELEASE_TYPE,
        release_local_pos,
        release_global,
        QtCore.Qt.MouseButton.LeftButton,
        QtCore.Qt.MouseButton.NoButton,
        modifiers,
    )
    QApplication.sendEvent(view_widget, press_event)
    QApplication.sendEvent(view_widget, move_event)
    QApplication.sendEvent(view_widget, release_event)
