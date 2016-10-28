#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on October 26, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QStackedLayout, QWidget

from .dialogs import LoadDialog, NewDialog, SaveDialog
import karabo_gui.icons as icons
from karabo_gui.docktabwindow import Dockable
from karabo_gui.actions import build_qaction, KaraboAction


class ProjectPanel(Dockable, QWidget):
    """ A dockable panel which contains a view of the project
    """
    def __init__(self, project_view):
        super(ProjectPanel, self).__init__()

        title = "Projects"
        self.setWindowTitle(title)

        self.project_view = project_view
        layout = QStackedLayout(self)
        layout.addWidget(self.project_view)
        self.setLayout(layout)

    def _create_actions(self):
        """ Create actions and return list of them"""
        new = KaraboAction(
            icon=icons.new,
            text="&New Project",
            tooltip="Create a New (Sub)project",
            triggered=project_new_handler,
        )
        load = KaraboAction(
            icon=icons.open,
            text="&Open Project",
            tooltip="Open an Existing Project",
            triggered=project_open_handler,
        )
        save = KaraboAction(
            icon=icons.save,
            text="&Save Project",
            tooltip="Save Project Snapshot",
            triggered=project_save_handler,
        )

        qactions = []
        for k_action in (new, load, save):
            q_ac = build_qaction(k_action, self)
            q_ac.triggered.connect(k_action.triggered)
            qactions.append(q_ac)
        return qactions

    def setupToolBars(self, toolbar, widget):
        """ Setup the project specific toolbar """
        qactions = self._create_actions()
        for ac in qactions:
            toolbar.addAction(ac)

    def onDock(self):
        pass

    def onUndock(self):
        pass


def project_new_handler():
    dialog = NewDialog()
    dialog.exec()


def project_open_handler():
    dialog = LoadDialog()
    dialog.exec()


def project_save_handler():
    dialog = SaveDialog()
    dialog.exec()
