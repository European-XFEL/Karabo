#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 11, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import QAction, QVBoxLayout, QWidget

from karabo_gui.docktabwindow import Dockable
import karabo_gui.icons as icons
from karabo_gui.mediator import (
    register_for_broadcasts, KaraboBroadcastEvent, KaraboEventSender)
from karabo_gui.projecttreeview import ProjectTreeView
from karabo_gui.singletons.api import get_manager


class ProjectPanel(Dockable, QWidget):
    def __init__(self):
        super(ProjectPanel, self).__init__()

        # Register for broadcast events.
        # This object lives as long as the app. No need to unregister.
        register_for_broadcasts(self)

        title = "Projects"
        self.setWindowTitle(title)

        self.twProject = ProjectTreeView(self)
        self.twProject.model().signalSelectionChanged.connect(
            self.onSelectionChanged)
        # Connect signal to get project available
        manager = get_manager()
        manager.signalAvailableProjects.connect(
            self.twProject.onAvailableProjects)
        manager.signalProjectLoaded.connect(self.twProject.onProjectLoaded)
        manager.signalProjectSaved.connect(self.twProject.onProjectSaved)

        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5, 5, 5, 5)
        mainLayout.addWidget(self.twProject)

        self.setupActions()

    def eventFilter(self, obj, event):
        """ Router for incoming broadcasts
        """
        if isinstance(event, KaraboBroadcastEvent):
            if event.sender is KaraboEventSender.NetworkConnectStatus:
                if not event.data['status']:
                    # Don't show projects when there's no server connection
                    self.closeAllProjects()
            return False
        return super(ProjectPanel, self).eventFilter(obj, event)

    def setupActions(self):
        text = "New project"
        self.acProjectNew = QAction(icons.new, "&New project", self)
        self.acProjectNew.setStatusTip(text)
        self.acProjectNew.setToolTip(text)
        self.acProjectNew.setEnabled(False)
        self.acProjectNew.triggered.connect(self.twProject.projectNew)

        text = "Open project"
        self.acProjectOpen = QAction(icons.load, "&Open project", self)
        self.acProjectOpen.setStatusTip(text)
        self.acProjectOpen.setToolTip(text)
        self.acProjectOpen.setEnabled(False)
        self.acProjectOpen.triggered.connect(self.twProject.projectOpen)

        text = "Save project"
        self.acProjectSave = QAction(icons.save, "&Save project", self)
        self.acProjectSave.setStatusTip(text)
        self.acProjectSave.setToolTip(text)
        self.acProjectSave.setEnabled(False)
        self.acProjectSave.triggered.connect(self.twProject.projectSave)

        text = "Save project as"
        self.acProjectSaveAs = QAction(icons.saveAs, "&Save project as", self)
        self.acProjectSaveAs.setStatusTip(text)
        self.acProjectSaveAs.setToolTip(text)
        self.acProjectSaveAs.setEnabled(False)
        self.acProjectSaveAs.triggered.connect(self.twProject.projectSaveAs)

    def setupToolBars(self, standardToolBar, parent):
        standardToolBar.addAction(self.acProjectNew)
        standardToolBar.addAction(self.acProjectOpen)
        standardToolBar.addAction(self.acProjectSave)
        standardToolBar.addAction(self.acProjectSaveAs)

    def closeAllProjects(self):
        return self.twProject.closeAllProjects()

    def enableToolBar(self, enabled):
        self.acProjectNew.setEnabled(enabled)
        self.acProjectOpen.setEnabled(enabled)

    def modifiedProjects(self):
        return self.twProject.modifiedProjects()

    def onSelectionChanged(self, selectedIndexes):
        self.acProjectSave.setEnabled(len(selectedIndexes) > 0)
        self.acProjectSaveAs.setEnabled(len(selectedIndexes) > 0)
