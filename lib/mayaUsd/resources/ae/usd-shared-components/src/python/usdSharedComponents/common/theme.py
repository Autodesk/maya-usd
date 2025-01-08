try:
    from PySide6 import QtSvg
    from PySide6.QtCore import QRect, Qt  # type: ignore
    from PySide6.QtGui import QImage, QPixmap, QPalette, QPainter, QColor, QPen, QIcon  # type: ignore
    from PySide6.QtWidgets import QWidget  # type: ignore
except:
    from PySide2 import QtSvg # type: ignore
    from PySide2.QtCore import QRect, Qt  # type: ignore
    from PySide2.QtGui import QImage, QPixmap, QPalette, QPainter, QColor, QPen, QIcon  # type: ignore
    from PySide2.QtWidgets import QWidget  # type: ignore

from enum import Flag, auto
from typing import Union

class Theme(object):
    _instance = None

    def __init__(self):
        raise RuntimeError("Call instance() instead")

    @classmethod
    def instance(cls):
        if cls._instance is None:
            cls._instance = cls.__new__(cls)
            cls._instance._palette = None
            cls._instance._icons = {}
        return cls._instance

    @classmethod
    def injectInstance(cls, theme):
        cls._instance = theme

    @property
    def uiScaleFactor(self) -> float:
        ### Returns the UI scale factor.
        return 1.0

    def uiScaled(self, value: Union[float, int]):
        ### Returns the scaled value.
        if isinstance(value, float):
            return value * self.uiScaleFactor
        if isinstance(value, int):
            return int(round(float(value) * self.uiScaleFactor))
        raise ValueError("Value must be a float or an int")

    def uiUnScaled(self, value: Union[float, int]):
        ### Returns the scaled value.
        return float(value) / self.uiScaleFactor

    class Palette(object):
        def __init__(self):
            super(Theme.Palette, self)
            self.colorResizeBorderActive: QColor = QColor(0x5285a6)


            pal = QPalette()
            self.colorPlaceHolderText = pal.color(QPalette.ColorRole.WindowText)
            self.colorPlaceHolderText.setAlphaF(0.7)

            self.colorListBorder = pal.color(QPalette.ColorRole.Dark)

    def icon(self, name: str) -> QIcon:
        ### Returns the icon with the given name.

        if name in self._icons:
            return self._icons[name]

        result = self.themedIcon(name, "dark")

        if result.isNull():
            raise ValueError(f"Icon '{name}' not found")

        self._icons[name] = result
        return result

    def themedIcon(self, name: str, theme: str) -> QIcon:
        import os, fnmatch
        result = QIcon()

        iconFolder = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "icons", theme))
        if os.path.exists(iconFolder):
            icons = fnmatch.filter(os.listdir(iconFolder), f"{name}.svg")
            for icon in icons:
                svg_renderer = QtSvg.QSvgRenderer(os.path.join(iconFolder, icon))
                image = QImage(64, 64, QImage.Format_ARGB32)
                image.fill(0x00000000)
                svg_renderer.render(QPainter(image))
                pixmap = QPixmap.fromImage(image)

                result.addPixmap(pixmap)
                # take the first SVG and run!
                return result

            icons = fnmatch.filter(os.listdir(iconFolder), f"{name}*.png")
            for icon in icons:
                svg_renderer = QtSvg.QSvgRenderer(os.path.join(iconFolder, icon))
                image = QImage(64, 64, QImage.Format_ARGB32)
                image.fill(0x00000000)
                svg_renderer.render(QPainter(image))
                pixmap = QPixmap.fromImage(image)

                result.addPixmap(pixmap)

        return result

    @property
    def palette(self) -> Palette:
        if self._palette is None:
            self._palette = self.Palette()
        return self._palette

    class State(Flag):
        Normal = auto()
        Hover = auto()
        Pressed = auto()
        Disabled = auto()
        Empty = auto()

    def paintResizableOverlay(self, widget: QWidget):
        """ Paints the overlay of a Resizable.

        Args:
            widget (QWidget): the widget to paint the overlay on.
        """

        painter = QPainter(widget)
        painter.setRenderHint(QPainter.Antialiasing, False)
        rect = widget.rect()
        rect.adjust(0, 0, 0, - self.resizableContentMargin())
        lineWidth: int = self.uiScaled(2)
        if lineWidth == 1:
            rect.adjust(1, 1, -1, -1)
        elif lineWidth > 1:
            halfLineWidth = round(lineWidth / 2.0)
            rect.adjust(halfLineWidth, halfLineWidth, -halfLineWidth, -halfLineWidth)

        painter.setPen(QPen(self.palette.colorResizeBorderActive, lineWidth, Qt.SolidLine, Qt.SquareCap, Qt.MiterJoin))
        painter.setBrush(Qt.NoBrush)
        painter.drawRect(rect)

    def resizableActiveAreaSize(self) -> int:
        """Returns the size of the active area at the bottom of a Resizable."""

        return self.uiScaled(6)

    def resizableContentMargin(self) -> int:
        """Returns the bottom margin of the content of a Resizable.

        The content margin is the the part of the active area extending the
        content widget area to make the active area of the Resizable easier to
        grab without overlapping too much of the content.
        """

        return self.uiScaled(2)

    def paintList(self, widget: QWidget, updateRect: QRect, state: State):
        raise RuntimeError("Needs to be implemented in derived class")

    def paintStringListEntry(self, painter: QPainter, rect: QRect, string: str):
        raise RuntimeError("Needs to be implemented in derived class")
