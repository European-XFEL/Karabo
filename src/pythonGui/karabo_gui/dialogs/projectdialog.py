#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on January 9, 2015
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


__all__ = ["ProjectSaveDialog", "ProjectLoadDialog"]

import datetime
import os
from os.path import abspath, dirname, exists, join

import karabo_gui.globals as globals
import karabo_gui.icons as icons
from karabo.middlelayer import ProjectAccess
from karabo_gui.messagebox import MessageBox
import karabo_gui.network as network

from PyQt4 import uic
from PyQt4.QtCore import (QDir, Qt)
from PyQt4.QtGui import (QDialog, QDialogButtonBox, QFileSystemModel,
                         QItemSelectionModel, QMessageBox, QMovie, QPalette,
                         QTreeWidgetItem)


class ProjectDialog(QDialog):


    def __init__(self):
        QDialog.__init__(self)
        uic.loadUi(join(abspath(dirname(__file__)), 'projectdialog.ui'), self)

        self.stackedWidgets = {
            'cloud': self.wCloud,
            'local': self.wLocal,
            'wait': self.wWait,
        }
        self.saveToComboItems = {
            ProjectAccess.LOCAL.value: 'local',
            ProjectAccess.CLOUD.value: 'cloud',
        }
        # A little sentinel to check for changes in the UI file
        assert len(self.saveToComboItems) == self.cbSaveTo.count()

        self.cbSaveTo.currentIndexChanged.connect(self.onSaveToChanged)
        # States whether the project server has already sent information about the CLOUD projects
        self.hasCloudProjects = False
        
        # Request all available projects in cloud
        network.Network().onGetAvailableProjects()
        
        self.twProjects.currentItemChanged.connect(self.onCloudProjectChanged)
        
        self.fileSystemModel = QFileSystemModel(self)
        self.fileSystemModel.setNameFilters(["*.krb"])
        self.fileSystemModel.setRootPath(QDir.homePath())
        self.twLocal.setModel(self.fileSystemModel)
        
        self.selectionModel = QItemSelectionModel(self.fileSystemModel, self)
        self.selectionModel.currentChanged.connect(self.onLocalProjectChanged)
        self.twLocal.setSelectionModel(self.selectionModel)
        
        rootDir = join(QDir.homePath(), globals.HIDDEN_KARABO_FOLDER)
        index = self.fileSystemModel.index(rootDir)
        self.twLocal.setCurrentIndex(index)
        self.twLocal.header().resizeSection(0, 300)
        
        # Cloud view is waiting for available projects - show waiting
        self.laWait.setAlignment(Qt.AlignHCenter | Qt.AlignVCenter)
        self.laWait.setAutoFillBackground(True)
        self.laWait.setBackgroundRole(QPalette.Base)
        
        movie = QMovie(join(abspath(dirname(icons.__file__)), "wait"))
        self.laWait.setMovie(movie)
        movie.start()
        
        self.swSaveTo.setCurrentWidget(self.stackedWidgets['wait'])

        self.leFilename.textChanged.connect(self.onChanged)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)


    @property
    def filename(self):
        """
        This property describes the filename including the project suffix.
        """
        return "{}.krb".format(self.basename)


    @property
    def basename(self):
        f = self.leFilename.text()
        splitted = f.split(".")
        return splitted[0]


    @property
    def location(self):
        return ProjectAccess.LOCAL \
               if self.cbSaveTo.currentIndex() == ProjectAccess.LOCAL.value \
               else ProjectAccess.CLOUD

    def _cloudPath(self):
        path = join(globals.KARABO_PROJECT_FOLDER, network.Network().username)
        if not exists(path):
            os.mkdir(path)
        return path

    def fillCloudProjects(self, projects):
        if self.twProjects.topLevelItemCount() > 0:
            self.twProjects.clear()
 
        # Fill all projects from cloud into the view
        for k in projects.keys():
            checkedOut = projects.get("{}.checkedOut".format(k))
            
            item = QTreeWidgetItem(self.twProjects)
            item.setData(0, Qt.UserRole, checkedOut)
            item.setIcon(0, icons.lock if checkedOut else icons.folder)
            item.setText(0, k)
            
            item.setText(1, "Yes" if checkedOut else "No")
            item.setText(2, str(projects.get("{}.checkedOutBy".format(k))))
            item.setData(0, Qt.UserRole, checkedOut)
            item.setText(3, str(projects.get("{}.author".format(k))))
            
            lastModified = projects.get("{}.lastModified".format(k))
            lastModified = str(datetime.datetime.fromtimestamp(lastModified))
            item.setText(4, lastModified)
            
            creationDate = projects.get("{}.creationDate".format(k))
            creationDate = str(datetime.datetime.fromtimestamp(creationDate))
            item.setText(5, creationDate)
            self.twProjects.addTopLevelItem(item)
        
        if self.swSaveTo.currentWidget() == self.stackedWidgets['wait']:
            self.swSaveTo.setCurrentWidget(self.stackedWidgets['cloud'])
        
        self.setFilenameEnabled(True)
        self.hasCloudProjects = True


    def setFilenameEnabled(self, enable):
        self.leFilename.setEnabled(enable)
        self.leFilename.setFocus(Qt.OtherFocusReason)


    def onChanged(self, text):
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(len(text) > 0)


    def onCloudProjectChanged(self, item): # previousItem
        self.leFilename.setText(item.text(0))


    def onLocalProjectChanged(self, current): # previous
        if self.fileSystemModel.isDir(current):
            self.leFilename.setText("")
            return
        
        self.leFilename.setText(current.data())


    def onSaveToChanged(self, index):
        if not self.hasCloudProjects and index == ProjectAccess.CLOUD.value:
            # Show waiting page
            self.swSaveTo.setCurrentWidget(self.stackedWidgets['wait'])
            self.setFilenameEnabled(False)
            return

        # Show the correct stacked widget for the combo box selection
        stackedWidgetKey = self.saveToComboItems[index]
        self.swSaveTo.setCurrentWidget(self.stackedWidgets[stackedWidgetKey])

        self.setFilenameEnabled(True)


class ProjectSaveDialog(ProjectDialog):


    def __init__(self, saveTo=ProjectAccess.CLOUD, title="Save project", btnText="Save"):
        ProjectDialog.__init__(self)
        
        if saveTo == ProjectAccess.LOCAL:
            self.cbSaveTo.setCurrentIndex(ProjectAccess.LOCAL.value)
        
        self.setWindowTitle(title)
        self.buttonBox.button(QDialogButtonBox.Ok).setText(btnText)
        self.buttonBox.accepted.connect(self.onSaved)

        self.twProjects.itemDoubleClicked.connect(self.onSaved)
        self.twLocal.doubleClicked.connect(self.onSaved)


    @property
    def filepath(self):
        """
        This property describes the filepath including the project suffix.
        """
        if self.location == ProjectAccess.CLOUD:
            return self._cloudPath()
        elif self.location == ProjectAccess.LOCAL:
            currentIndex = self.twLocal.currentIndex()
            if self.fileSystemModel.isDir(currentIndex):
                return self.fileSystemModel.filePath(currentIndex)
            else:
                return self.fileSystemModel.filePath(currentIndex.parent())


    def onSaved(self):
        # Check if filename is already existing
        if self.cbSaveTo.currentIndex() == ProjectAccess.CLOUD.value:
            for i in range(self.twProjects.topLevelItemCount()):
                item = self.twProjects.topLevelItem(i)
                if item.text(0) == self.basename:
                    checkedOut = item.data(0, Qt.UserRole)
                    checkedOutBy = item.text(2)
                    if checkedOut and (checkedOutBy != network.Network().username):
                        # Project is locked - can be overwritten by same user
                        QMessageBox.warning(None, 'Project already checked out',
                            "Another project with the same name \"<b>{}</b>\"<br>"
                            "is already checked out by the user \"<b>{}</b>\".<br><br>"
                            "Please, choose another name for the project."
                            .format(self.filename, item.text(2)))
                        return
                    # Project already exists
                    reply = QMessageBox.question(None, 'Project already exists',
                        "Another project with the same name <br> \"<b>{}</b>\" "
                        "already exists.<br><br>Do you want to overwrite it?".format(self.filename),
                        QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
                    if reply == QMessageBox.No:
                        return
                    break
        elif self.cbSaveTo.currentIndex() == ProjectAccess.LOCAL.value:
            if not self.leFilename.text():
                MessageBox.showInformation("Please set a project name.")
                return
            
            if self.fileSystemModel.isDir(self.twLocal.currentIndex()):
                fi = self.fileSystemModel.fileInfo(self.twLocal.currentIndex())
            else:
                fi = self.fileSystemModel.fileInfo(self.twLocal.currentIndex().parent())

            dir = QDir(fi.absoluteFilePath())
            if dir.exists(self.filename):
                reply = QMessageBox.question(None, 'Project already exists',
                    "Another project with the same name <br> \"<b>{}</b>\""
                    "already exists.<br><br>Do you want to overwrite it?".format(self.filename),
                    QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
                if reply == QMessageBox.No:
                    return
        
        self.accept()


class ProjectLoadDialog(ProjectDialog):


    def __init__(self):
        ProjectDialog.__init__(self)
        
        self.setWindowTitle("Load project")
        self.buttonBox.button(QDialogButtonBox.Ok).setText("Open")
        self.buttonBox.accepted.connect(self.accept)
        
        self.leFilename.setReadOnly(True)
        
        self.twProjects.itemDoubleClicked.connect(self.accept)
        self.twLocal.doubleClicked.connect(self.accept)


    @property
    def filepath(self):
        """
        This property describes the filepath including the project suffix.
        """
        if self.location == ProjectAccess.CLOUD:
            return self._cloudPath()
        elif self.location == ProjectAccess.LOCAL:
            return self.fileSystemModel.filePath(self.twLocal.currentIndex())

