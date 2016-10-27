#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QWidget, QStackedLayout

from karabo_gui.docktabwindow import Dockable


class ProjectPanel(Dockable, QWidget):
    """ A Dockable panel which contains a view of the project
    """
    def __init__(self, project_view):
        super(ProjectPanel, self).__init__()

        title = "Projects"
        self.setWindowTitle(title)

        self.project_view = project_view
        layout = QStackedLayout(self)
        layout.addWidget(self.project_view)
        self.setLayout(layout)

    def onDock(self):
        pass

    def onUndock(self):
        pass

    def setupToolBars(self, toolbar, widget):
        pass
