from .theme import Theme
from .persistentStorage import PersistentStorage
from typing import Union
try:
    from PySide6.QtCore import Qt, Signal, QRect
    from PySide6.QtWidgets import QWidget, QStackedLayout, QVBoxLayout, QSizePolicy
except ImportError:
    from PySide2.QtCore import Qt, Signal, QRect  # type: ignore
    from PySide2.QtWidgets import QWidget, QStackedLayout, QVBoxLayout, QSizePolicy  # type: ignore


class Resizable(QWidget):

    __slots__ = [
        "_contentLayout",
        "_minContentSize",
        "_maxContentSize",
        "_persistentStorageGroup",
        "_persistentStorageKey",
        "_contentSize",
        "_dragStartContentSize",
        "_widget",
        "_overlay",
    ]

    class _Overlay(QWidget):

        dragged = Signal(int)
        dragging = Signal(bool)

        __slots__ = ["_active", "_mousePressGlobalPosY", "_maskRect"]

        def __init__(self, parent=None):
            super(Resizable._Overlay, self).__init__(parent)

            self._active:bool = False
            self._mousePressGlobalPosY: Union[int, None] = None
            self._maskRect: QRect = None

            self.setWindowFlags(Qt.FramelessWindowHint)
            self.setAttribute(Qt.WA_TranslucentBackground)
            self.setAttribute(Qt.WA_NoSystemBackground)
            self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
            s: int = Theme.instance().resizableActiveAreaSize()
            self.setMinimumSize(s, s)
            self.setMouseTracking(True)
            self.setFocusPolicy(Qt.NoFocus)

        def paintEvent(self, _):
            if self._active:
                Theme.instance().paintResizableOverlay(self)

        def resizeEvent(self, event):
            super().resizeEvent(event)
            s: int = Theme.instance().resizableActiveAreaSize()
            self._maskRect = QRect(0, event.size().height() - s, event.size().width(), s)
            if not self._active:
                self.setMask(self._maskRect)

        def mouseMoveEvent(self, event):
            if self._mousePressGlobalPosY is not None:
                diff = event.globalPos().y() - self._mousePressGlobalPosY
                # self._mousePressGlobalPosY = event.globalPos().y()
                self.dragged.emit(diff)
                event.accept()
            else:
                _overActiveArea: bool = event.pos().y() >= self.height() - Theme.instance().resizableActiveAreaSize()
                if _overActiveArea != self._active:
                    self._active = _overActiveArea
                    if self._active:
                        self.clearMask()
                    else:
                        self.setMask(self._maskRect)
                    self.update()
                    self.setCursor(Qt.SizeVerCursor if self._active else Qt.ArrowCursor)
                event.ignore()

        def mousePressEvent(self, event):
            if self._active:
                self.clearMask()
                self._mousePressGlobalPosY = event.globalPos().y()
                self.dragging.emit(True)
                event.accept()
            else:
                event.ignore()

        def mouseReleaseEvent(self, event):
            if self._active:
                self._mousePressGlobalPosY = None
                self.dragging.emit(False)
                event.accept()
            else:
                event.ignore()

        def enterEvent(self, event):
            self._active = event.pos().y() >= self.height() - Theme.instance().resizableActiveAreaSize()
            if self._active:
                event.accept()
                self._active = False
                self.update()
                self.setCursor(Qt.SizeVerCursor)
            else:
                event.ignore()

        def leaveEvent(self, event):
            if self._active:
                self._active = False
                self.update()
                self.setCursor(Qt.ArrowCursor)
                event.accept()
            else:
                event.ignore()

    def __init__(
        self,
        w: QWidget = None,
        persistentStorageGroup: str = None,
        persistentStorageKey: str = None,
        parent: QWidget = None,
    ):
        super(Resizable, self).__init__(parent)

        stackedLayout = QStackedLayout()
        stackedLayout.setContentsMargins(0, 0, 0, 0)
        stackedLayout.setSpacing(0)
        stackedLayout.setStackingMode(QStackedLayout.StackAll)

        contentWidget = QWidget(self)
        self._contentLayout = QVBoxLayout(contentWidget)
        self._contentLayout.setContentsMargins(0, 0, 0, Theme.instance().resizableContentMargin())
        self._contentLayout.setSpacing(0)

        self._minContentSize: int = 0
        self._maxContentSize: int = 500

        self._persistentStorageGroup: str = persistentStorageGroup
        self._persistentStorageKey: str = persistentStorageKey

        self._contentSize: int = -1
        self._dragStartContentSize: int = -1
        self._widget: QWidget = None

        self.loadPersistentStorage()

        if w is not None:
            self.widget = w

        self._overlay = Resizable._Overlay(self)
        stackedLayout.addWidget(contentWidget)
        stackedLayout.addWidget(self._overlay)
        stackedLayout.setCurrentIndex(1)

        self._overlay.dragged.connect(self.onResizeHandleDragged)
        self._overlay.dragging.connect(self.onResizeHandleDragging)

        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(0, 0, 0, 0)
        mainLayout.setSpacing(0)
        mainLayout.addLayout(stackedLayout, 1)

    def onResizeHandleDragging(self, dragging: bool):
        if dragging:
            self._dragStartContentSize = self._contentSize
        else:
            self.savePersistentStorage()

    def onResizeHandleDragged(self, dy):
        height = self._dragStartContentSize + dy
        self.contentSize = height

    @property
    def widget(self):
        return self._widget

    @widget.setter
    def widget(self, w: QWidget):
        if self._widget != w:
            if self._widget is not None:
                self._contentLayout.removeWidget(self._widget)
                self._widget.hide()
            self._widget = w
            if self._widget is not None:
                self._contentLayout.insertWidget(0, self._widget, 1)
                self._widget.show()
                if self._contentSize == -1:
                    self.contentSize = self._widget.height()
                else:
                    self.contentSize = self._contentSize

    @property
    def contentSize(self):
        return self._contentSize

    @contentSize.setter
    def contentSize(self, s: int):
        self._contentSize = max(self._minContentSize, min(self._maxContentSize, s))
        if self._widget is not None:
            self._widget.setFixedHeight(self._contentSize)

    @property
    def minContentSize(self):
        return self._minContentSize

    @minContentSize.setter
    def minContentSize(self, s: int):
        self._minContentSize = max(0, s)
        if self._contentSize != -1 and self._minContentSize > self._contentSize:
            self.contentSize = self._minContentSize

    @property
    def maxContentSize(self) -> int:
        return self._maxContentSize

    @maxContentSize.setter
    def maxContentSize(self, s: int):
        self._maxContentSize = max(0, s)
        if self._contentSize != -1 and self._maxContentSize < self._contentSize:
            self.contentSize = self._maxContentSize

    def setPersistentStorage(self, group: str, key: str):
        self._persistentStorageGroup = group
        self._persistentStorageKey = key

    def savePersistentStorage(self):
        if self._contentSize != -1 and self._persistentStorageGroup is not None and self._persistentStorageKey is not None:
            s: float = Theme.instance().uiUnScaled(float(self._contentSize))
            PersistentStorage.instance().set(self._persistentStorageGroup, self._persistentStorageKey, s)

    def loadPersistentStorage(self):
        if self._persistentStorageGroup is not None and self._persistentStorageKey is not None:
            s: float = float(PersistentStorage.instance().get(self._persistentStorageGroup, self._persistentStorageKey, -1.0))
            if s != -1.0:
                self.contentSize = round(Theme.instance().uiScaled(s))
