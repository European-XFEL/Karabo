#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 11, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the project panel on the
   middle left of the MainWindow which is un/dockable.
"""

__all__ = ["ProjectPanel"]

import const

from enums import NavigationItemTypes
from manager import Manager

from PyQt4.QtGui import QAction, QIcon, QInputDialog, QLineEdit, QMessageBox, \
                        QTreeWidget, QTreeWidgetItem, QVBoxLayout, QWidget


class ProjectPanel(QWidget):
    ##########################################
    # Dockable widget class used in DivWidget
    # Requires following interface:
    # 
    #def setupActions(self):
    #    pass
    #def setupToolBars(self, standardToolBar, parent):
    #    pass
    #def onUndock(self):
    #    pass
    #def onDock(self):
    #    pass
    ##########################################
    
    def __init__(self):
        super(ProjectPanel, self).__init__()
        
        title = "Projects"
        self.setWindowTitle(title)
        
        self.__twProject = QTreeWidget(self)
        self.__twProject.setHeaderLabels(["Projects"])
        #self.__twProject.itemSelectionChanged.connect(self.projectItemSelectionChanged)
        
        mainLayout = QVBoxLayout(self)
        mainLayout.setContentsMargins(5,5,5,5)
        mainLayout.addWidget(self.__twProject)

        self.setupActions()
        #Manager().notifier.signalCreateNewProjectConfig.connect(self.onCreateNewProjectConfig)


    def setupActions(self):
        text = "New project"
        self.__acProjectNew = QAction(QIcon(":new"), "&New project", self)
        self.__acProjectNew.setStatusTip(text)
        self.__acProjectNew.setToolTip(text)
        self.__acProjectNew.triggered.connect(self.onProjectNew)

        text = "Open project"
        self.__acProjectOpen = QAction(QIcon(":open"), "&Open project", self)
        self.__acProjectOpen.setStatusTip(text)
        self.__acProjectOpen.setToolTip(text)
        self.__acProjectOpen.triggered.connect(self.onProjectOpen)

        text = "Save project"
        self.__acProjectSave = QAction(QIcon(":save"), "&Save project", self)
        self.__acProjectSave.setStatusTip(text)
        self.__acProjectSave.setToolTip(text)
        self.__acProjectSave.setEnabled(False)
        self.__acProjectSave.triggered.connect(self.onProjectSave)


    def setupToolBars(self, standardToolBar, parent):
        standardToolBar.addAction(self.__acProjectNew)
        standardToolBar.addAction(self.__acProjectOpen)
        standardToolBar.addAction(self.__acProjectSave)


    #def projectItemSelectionChanged(self):
    #    item = self.__twProject.currentItem()
    #    if not item: return
    #    Manager().notifier.signalProjectItemChanged.emit(dict(type=NavigationItemTypes.CLASS, key=item.data(0, const.INTERNAL_KEY)))


    def _createProject(self, projectName):
        # Check whether project already exists

        # Create items
        item = QTreeWidgetItem(self.__twProject)
        item.setText(0, projectName)
        item.setIcon(0, QIcon(":folder"))

        # Add child items
        childItem = QTreeWidgetItem(item, ["Devices"])
        childItem = QTreeWidgetItem(item, ["Scenes"])
        childItem = QTreeWidgetItem(item, ["Macros"])
        childItem = QTreeWidgetItem(item, ["Monitors"])
        childItem = QTreeWidgetItem(item, ["Resources"])


### Slots ###
    def onProjectNew(self):
        projectName = QInputDialog.getText(self, "New project", \
                                           "Enter project name:", QLineEdit.Normal, "")

        if not projectName[1]:
            return

        if len(projectName[0]) < 1:
            reply = QMessageBox.question(self, "Project name", "Please enter a name!",
                QMessageBox.Ok | QMessageBox.Cancel, QMessageBox.Ok)

            if reply == QMessageBox.Cancel:
                return
            
            self.onProjectNew()
            return

        self._createProject(projectName[0])


    def onProjectOpen(self):
        print "onProjectOpen"


    def onProjectSave(self):
        print "onProjectSave"


    def onCreateNewProjectConfig(self, customItem, path, configName):
        print path, configName
        
        item = QTreeWidgetItem(self.__twProject)
        item.setData(0, const.INTERNAL_KEY, path)
        item.setText(0, configName)
        
        customItem.signalValueChanged.connect(self.onAdditionalInfoChanged)
        
        for i in self.__twProject.selectedItems():
            i.setSelected(False)
        
        item.setSelected(True)


    def onAdditionalInfoChanged(self, key, deviceId):
        # When deviceId of customItem was changed
        for i in xrange(self.__twProject.topLevelItemCount()):
            item = self.__twProject.topLevelItem(i)
            if (item.data(0, const.INTERNAL_KEY) + ".configuration.deviceId") == key:
                oldText = item.text(0)
                splittedText = str(oldText).split("-<")
                item.setText(0, "{}-<{}>").format(splittedText[0], deviceId)


    # virtual function
    def onUndock(self):
        pass


    # virtual function
    def onDock(self):
        pass


