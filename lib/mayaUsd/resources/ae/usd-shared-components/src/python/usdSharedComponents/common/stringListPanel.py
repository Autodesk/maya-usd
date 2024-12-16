from typing import Sequence
import textwrap
from .filteredStringListView import FilteredStringListView
from ..data.stringListData import StringListData

try:
    from PySide6.QtCore import QEvent, QObject, Qt  # type: ignore
    from PySide6.QtWidgets import (  # type: ignore
        QLabel,
        QVBoxLayout,
        QHBoxLayout,
        QWidget,
        QCheckBox,
    )
except:
    from PySide2.QtCore import QEvent, QObject, Qt  # type: ignore
    from PySide2.QtWidgets import QLabel, QVBoxLayout, QHBoxLayout, QWidget, QCheckBox  # type: ignore

# TODO: support I8N
kIncludeAllLabel = "Include all"
kIncludeAllTooltip = textwrap.dedent(
    """
    When enabled, all prims are illuminated. When disabled,
    only the prims in the Include list are illuminated.
    """).strip()

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
        LEFT_RIGHT_MARGINS = 2
        layout.setContentsMargins(LEFT_RIGHT_MARGINS, 0, LEFT_RIGHT_MARGINS, 0)

        headerWidget = QWidget(self)
        headerLayout = QHBoxLayout(headerWidget)

        titleLabel = QLabel(headerTitle, self)
        headerLayout.addWidget(titleLabel)
        headerLayout.addStretch(1)
        headerLayout.setContentsMargins(0, 0, 0, 0)

        # only add the check box on the header if there's a label
        if isInclude:
            self.cbIncludeAll = QCheckBox(kIncludeAllLabel, self)
            self.cbIncludeAll.setToolTip(kIncludeAllTooltip)
            self.cbIncludeAll.setCheckable(True)
            headerLayout.addWidget(self.cbIncludeAll)

        headerWidget.setLayout(headerLayout)

        layout.addWidget(headerWidget)
        layout.addWidget(self.list)
        self.setLayout(layout)

