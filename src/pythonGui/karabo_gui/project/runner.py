#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import sys

from PyQt4.QtGui import QApplication, QFrame, QHBoxLayout, QVBoxLayout

from karabo_gui.singletons.api import get_manager, get_network
from .api import ProjectView
from .panel import ProjectPanel


def load_panel(project_view):
    import karabo_gui.icons as icons
    from karabo_gui.toolbar import ToolBar

    class _Frame(QFrame):
        def __init__(self, panel):
            super(_Frame, self).__init__()
            self.toolbar = ToolBar()
            self.dockableWidget = panel

            self.toolBarLayout = QHBoxLayout()
            self.toolBarLayout.setContentsMargins(0, 0, 0, 0)
            self.toolBarLayout.setSpacing(0)
            self.addToolBar(self.toolbar)

            vLayout = QVBoxLayout(self)
            vLayout.setContentsMargins(0, 0, 0, 0)
            vLayout.addLayout(self.toolBarLayout)
            vLayout.addWidget(self.dockableWidget)

            self.dockableWidget.setupToolBars(self.toolbar, self)

        def addToolBar(self, toolbar):
            self.toolBarLayout.addWidget(toolbar)

    icons.init()  # Very important!
    return _Frame(ProjectPanel(project_view))


def main():
    app = QApplication(sys.argv)

    project_view = ProjectView()
    widget = load_panel(project_view)
    widget.show()
    widget.resize(300, 500)

    # XXX: Just a hack to connect to the GUI Server for testing
    get_manager()
    get_network().connectToServer()

    app.exec_()
