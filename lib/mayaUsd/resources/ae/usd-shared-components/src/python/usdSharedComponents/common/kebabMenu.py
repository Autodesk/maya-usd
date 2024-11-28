from ..collection.expressionRulesMenu import ExpressionMenu
from ..resources import *
try:
    from PySide6.QtCore import Qt # type: ignore
    from PySide6.QtGui import QIcon  # type: ignore
    from PySide6.QtWidgets import QSizePolicy, QToolButton, QWidget # type: ignore
except ImportError:
    from PySide2.QtCore import Qt # type: ignore
    from PySide2.QtGui import QIcon  # type: ignore
    from PySide2.QtWidgets import QSizePolicy, QToolButton, QWidget # type: ignore

class KebabMenu(QToolButton):
    def __init__(self, menu: ExpressionMenu, parent: QWidget = None):
        super(KebabMenu, self).__init__(parent)
        self.setIcon(QIcon(":/icons/kebab.png"))
        self.setSizePolicy(QSizePolicy.Minimum, QSizePolicy.Minimum)
        self.setArrowType(Qt.NoArrow)
        self.setPopupMode(QToolButton.InstantPopup)
        self.setStyleSheet("""
            QToolButton::menu-indicator { width: 0px; }
        """)
        self.setMenu(menu)
