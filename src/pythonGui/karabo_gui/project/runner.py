#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import sys

from PyQt4.QtGui import QApplication

from karabo_gui.panels.container import PanelContainer
from karabo_gui.panels.projectpanel import ProjectPanel
from karabo_gui.singletons.api import get_manager, get_network


def load_panel():
    import karabo_gui.icons as icons

    icons.init()  # Very important!

    container = PanelContainer("Project", None)
    container.addPanel(ProjectPanel, "Project")
    return container


def main():
    app = QApplication(sys.argv)

    widget = load_panel()
    widget.show()
    widget.resize(300, 500)

    # XXX: Just a hack to connect to the GUI Server for testing
    get_manager()
    get_network().connectToServer()

    app.exec_()
