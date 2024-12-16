from typing import Sequence
from .filteredStringListView import FilteredStringListView

try:
    from PySide6.QtWidgets import (  # type: ignore
        QLabel,
        QVBoxLayout,
        QHBoxLayout,
        QWidget,
        QCheckBox,
    )
except:
    from PySide2.QtWidgets import QLabel, QVBoxLayout, QHBoxLayout, QWidget, QCheckBox  # type: ignore


class StringList(QWidget):

    def __init__(
        self,
        items: Sequence[str] = [],
        headerTitle: str = "",
        toggleTitle: str = "",
        parent=None,
    ):
        super(StringList, self).__init__(parent)
        self.list = FilteredStringListView(items, headerTitle, self)
        self.list.update_placeholder()

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
        if toggleTitle != None and toggleTitle != "":
            self.cbIncludeAll = QCheckBox(toggleTitle, self)
            self.cbIncludeAll.setCheckable(True)
            headerLayout.addWidget(self.cbIncludeAll)

        headerWidget.setLayout(headerLayout)

        layout.addWidget(headerWidget)
        layout.addWidget(self.list)
        self.setLayout(layout)
