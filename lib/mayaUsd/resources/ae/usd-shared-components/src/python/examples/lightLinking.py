from PySide6.QtCore import QRect, Qt
from PySide6.QtGui import QPainter, QColor, QPen, QPalette, QTextOption
from PySide6.QtWidgets import QWidget, QApplication, QMainWindow

import sys, os

sys.path.insert(
    0, os.path.realpath(os.path.dirname(os.path.abspath(__file__)) + "/..")
)

# make sure you have the USD python bindings installed - or adapt this path to your needs
if 'PXR_USD_WINDOWS_DLL_PATH' not in os.environ :
    os.environ['PXR_USD_WINDOWS_DLL_PATH'] = "C:/dev/3dsmax-component-usd_2/build/bin/x64/Hybrid/usd-component-2025/Contents/bin"
sys.path.append("C:/dev/3dsmax-component-usd_2/build/bin/x64/Hybrid/usd-component-2025/Contents/bin/python")


from usdSharedComponents.common.theme import Theme
from usdSharedComponents.collection.widget import CollectionWidget

class StandaloneTheme(Theme):

    def __init__(self):
        ### Standalone theme colors.
        self._colorBackground = 0x494949
        self._colorBackgroundList = 0x373737
        self._colorText = 0xeeeeee
        self._colorTextDisabled = 0x80eeeeee
        self._colorResizeBorderActive = 0x5285a6

        self._palette = self.Palette()
        ### we may e.g. Initialize the custom palette.
        # self._palette.colorResizeBorderActive = QColor(0x993322)
        pass

    def paintList(self, widget: QWidget, updateRect: QRect, state: Theme.State):
        painter = QPainter(widget)
        rect = widget.rect()

        color = QColor(self._colorBackgroundList)
        painter.fillRect(rect, color)

        if state & Theme.State.Empty:
            # TODO: Implement empty state
            pass

    def paintStringListEntry(self, painter: QPainter, rect: QRect, string: str):
        color = QColor(self._colorBackgroundList)
        painter.fillRect(rect, color)

        offset: int = self.uiScaled(6)
        textRect = rect.adjusted(offset, 0, -offset, 0)
        string = painter.fontMetrics().elidedText(string, Qt.TextElideMode.ElideMiddle, textRect.width())

        textColor = QColor(self._colorText)
        painter.setPen(textColor)
        painter.drawText(textRect, string, Qt.AlignVCenter | Qt.AlignLeft)


# Initialize the theme
Theme.injectInstance(StandaloneTheme())

app = QApplication(sys.argv)

# Let's try to make the Qt color scheme match the theme a bit more to not look
# completely out of place (to be continued...)
palette = QPalette()
palette.setColor(QPalette.Window, QColor(0x494949))
app.setPalette(palette)

mainWindow = QMainWindow()
mainWindow.setWindowTitle("Standalone USD Light-Linking Test")

lightLinking = CollectionWidget()
mainWindow.setCentralWidget(lightLinking)
mainWindow.show()

# Close the application when the main window is closed. This is a temporary
# workaround for a bug in 3dsMax's Qt/Python that prevents the last window from
# terminating the application.
mainWindow.closeEvent = lambda event: app.quit()

# Run the application
app.exec()
