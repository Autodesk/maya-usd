from typing import Sequence
from .theme import Theme
import textwrap
from .filteredStringListView import FilteredStringListView
from ..data.stringListData import StringListData

try:
    from PySide6.QtCore import QRect, Qt  # type: ignore
    from PySide6.QtGui import QPainter  # type: ignore
    from PySide6.QtWidgets import (  # type: ignore
        QCheckBox,
        QHBoxLayout,
        QStyle,
        QStyleOptionHeaderV2,
        QStylePainter,
        QVBoxLayout,
        QWidget
    )
except ImportError:
    from PySide2.QtCore import QRect, Qt  # type: ignore
    from PySide2.QtGui import QPainter  # type: ignore
    from PySide2.QtWidgets import QCheckBox, QHBoxLayout, QVBoxLayout, QStyle, QStyleOptionHeader as QStyleOptionHeaderV2, QStylePainter, QWidget  # type: ignore

# TODO: support I8N
kIncludeAllLabel = "Include all"
kIncludeAllTooltip = textwrap.dedent(
    """
    When enabled, all prims are illuminated. When disabled,
    only the prims in the Include list are illuminated.
    """).strip()

_LEFT_RIGHT_MARGINS: int = Theme.instance().uiScaled(2)

class StringListPanel(QWidget):
    def __init__(
        self,
        data: StringListData,
        isInclude: bool,
        headerTitle: str = "",
        parent=None,
    ):
        super(StringListPanel, self).__init__(parent)
        self.list = FilteredStringListView(data, headerTitle, self)

        layout = QVBoxLayout(self)
        layout.setSpacing(0)

        layout.setContentsMargins(_LEFT_RIGHT_MARGINS + 1, 0, _LEFT_RIGHT_MARGINS + 1, 0)

        # This needs to be here for some special styling of 3ds Max, it won't
        # affect other hosts
        self.setProperty("StyleLayer", 3)

        self.headerWidget = StringListPanel.HeaderWidget(headerTitle, self)
        headerLayout = QHBoxLayout(self.headerWidget)
        headerLayout.addStretch(1)
        headerLayout.setContentsMargins(0, 0, _LEFT_RIGHT_MARGINS, 0)

        # only add the check box on the header if there's a label
        if isInclude:
            self.cbIncludeAll = QCheckBox(kIncludeAllLabel, self)
            self.cbIncludeAll.setToolTip(kIncludeAllTooltip)
            self.cbIncludeAll.setStatusTip(kIncludeAllTooltip)
            self.cbIncludeAll.setCheckable(True)
            headerLayout.addWidget(self.cbIncludeAll)

        self.headerWidget.setLayout(headerLayout)

        layout.addWidget(self.headerWidget)
        layout.addWidget(self.list)
        self.setLayout(layout)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setPen(Theme.instance().palette.colorListBorder)
        r: QRect = self.rect()
        r.adjust(_LEFT_RIGHT_MARGINS, 0, -(_LEFT_RIGHT_MARGINS + 1), -1)
        painter.drawRect(r)

    @property
    def title(self):
        return self.headerWidget._headerTitle

    class HeaderWidget(QWidget):

        def __init__(self, headerTitle: str = "", parent=None):
            super(StringListPanel.HeaderWidget, self).__init__(parent)
            self._headerTitle = headerTitle
            self.setMinimumHeight(Theme.instance().uiScaled(24))

        def paintEvent(self, event):
            painter = QStylePainter(self)
            opt = QStyleOptionHeaderV2()
            opt.initFrom(self)
            opt.position = QStyleOptionHeaderV2.SectionPosition.Middle
            opt.orientation = Qt.Horizontal
            opt.state = QStyle.State_None | QStyle.State_Raised | QStyle.State_Horizontal
            opt.section = 0
            
            # adjust the rect to not draw the right border
            opt.rect.adjust(-1,0,1,0)

            if self.isEnabled():
                opt.state |= QStyle.State_Enabled
            if self.isActiveWindow():
                opt.state |= QStyle.State_Active
            opt.text = self._headerTitle
            painter.drawControl(QStyle.CE_Header, opt)
