#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import sys

from PyQt4.QtGui import QApplication

import karabo_gui.icons as icons
from karabo_gui.panels.projectpanel import ProjectPanel
from karabo_gui.singletons.api import (
    get_manager, get_network, get_panel_wrangler)


def main():
    app = QApplication(sys.argv)

    icons.init()  # Very important!

    widget = ProjectPanel()
    widget.show()
    widget.resize(300, 500)

    # XXX: Just a hack to connect to the GUI Server for testing
    get_manager()
    get_panel_wrangler()
    get_network().connectToServer()

    app.exec_()
