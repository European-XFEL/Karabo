#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on February 9, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QFrame, QLabel

from karabo_gui import icons
from karabo_gui.widget import DisplayWidget
from .base import BaseWidgetContainer


class _DummyDisplayWidget(DisplayWidget):
    def __init__(self, box, parent):
        super(_DummyDisplayWidget, self).__init__(box)

        self.value = None
        self.widget = QLabel(parent)
        self.widget.setFrameShape(QFrame.Box)
        self.widget.setScaledContents(True)
        self.widget.setPixmap(icons.no.pixmap(100))


class VacuumWidgetPlaceholder(BaseWidgetContainer):
    """VacuumWidgets are DEPRECATED.

    This class gives us a visual representation for VacuumWidgetModel objects
    in scenes. That representation is suitably ugly, so as to convince the user
    to pick a more appropriate widget.
    """
    def _create_widget(self, boxes):
        return _DummyDisplayWidget(boxes[0], self)
