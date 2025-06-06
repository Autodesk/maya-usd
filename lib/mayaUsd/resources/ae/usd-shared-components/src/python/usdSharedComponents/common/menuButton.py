from .theme import Theme
try:
    from PySide6.QtCore import Qt # type: ignore
    from PySide6.QtWidgets import QSizePolicy, QToolButton, QWidget, QMenu # type: ignore
except ImportError:
    from PySide2.QtCore import Qt # type: ignore
    from PySide2.QtWidgets import QSizePolicy, QToolButton, QWidget, QMenu # type: ignore

class MenuButton(QToolButton):
    def __init__(self, menu: QMenu, parent: QWidget = None):
        super(MenuButton, self).__init__(parent)
        self.setIcon(Theme.instance().icon("menu"))
        self.setSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        self.setMaximumWidth(Theme.instance().uiScaled(16))
        self.setArrowType(Qt.NoArrow)
        self.setPopupMode(QToolButton.InstantPopup)
        Theme.instance().themeMenuButton(self, False)
        self.setMenu(menu)