#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on January 9, 2015
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


__all__ = ["ProjectSaveDialog", "ProjectLoadDialog"]

import globals
import network

from PyQt4 import uic
from PyQt4.QtCore import (QDir, Qt)
from PyQt4.QtGui import (QDialog, QDialogButtonBox, QFileSystemModel,
                         QItemSelectionModel, QMessageBox, QMovie, QPalette)

import os.path


class ProjectDialog(QDialog):

    CLOUD = 0
    LOCAL = 1

    def __init__(self):
        QDialog.__init__(self)
        uic.loadUi(os.path.join(os.path.dirname(__file__), 'projectdialog.ui'), self)
        
        # Request all available projects in cloud
        network.Network().onGetAvailableProjects()
        
        self.lwProjects.currentItemChanged.connect(self.onCloudProjectChanged)
        
        self.fileSystemModel = QFileSystemModel(self)
        self.fileSystemModel.setNameFilters(["*.krb"])
        self.fileSystemModel.setRootPath(QDir.homePath())
        self.twLocal.setModel(self.fileSystemModel)
        
        self.selectionModel = QItemSelectionModel(self.fileSystemModel, self)
        self.selectionModel.currentChanged.connect(self.onLocalProjectChanged)
        self.twLocal.setSelectionModel(self.selectionModel)
        
        rootDir = os.path.join(QDir.homePath(), globals.HIDDEN_KARABO_FOLDER)
        index = self.fileSystemModel.index(rootDir)
        self.twLocal.setCurrentIndex(index)
        self.twLocal.header().resizeSection(0, 300)
        
        # Cloud view is waiting for available projects - show waiting
        self.laWait.setAlignment(Qt.AlignHCenter | Qt.AlignVCenter)
        self.laWait.setAutoFillBackground(True)
        self.laWait.setBackgroundRole(QPalette.Base)
        
        movie = QMovie(os.path.join("icons", "wait"))
        self.laWait.setMovie(movie)
        movie.start()
        
        self.swSaveTo.setCurrentIndex(2)

        self.leFilename.textChanged.connect(self.onChanged)
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(False)


    @property
    def filename(self):
        """
        This property describes the filename including the project suffix.
        """
        f = self.leFilename.text()
        if not f.endswith(".krb"):
            return "{}.krb".format(f)
        else:
            return f


    @property
    def filepath(self):
        """
        This property describes the filepath including the project suffix.
        """
        if self.location == ProjectDialog.CLOUD:
            return self.filename
        elif self.location == ProjectDialog.LOCAL:
            return self.fileSystemModel.filePath(self.twLocal.currentIndex())


    @property
    def location(self):
        return self.cbSaveTo.currentIndex()


    def fillCloudProjects(self, projects):
        if self.lwProjects.count() > 0:
            self.lwProjects.clear()
        
        # Fill all projects from cloud into table view
        self.lwProjects.addItems(projects)
        if self.swSaveTo.currentIndex() == 2:
            self.swSaveTo.setCurrentIndex(ProjectDialog.CLOUD)
        
        if not self.leFilename.isEnabled():
            self.leFilename.setEnabled(True)
            self.leFilename.setFocus(Qt.OtherFocusReason)


    def onChanged(self, text):
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(len(text) > 0)


    def onCloudProjectChanged(self, item): # previousItem
        self.leFilename.setText(item.text())


    def onLocalProjectChanged(self, current): # previous
        if self.fileSystemModel.isDir(current):
            self.leFilename.setText("")
            return
        
        self.leFilename.setText(current.data())


class ProjectSaveDialog(ProjectDialog):


    def __init__(self):
        ProjectDialog.__init__(self)
        
        self.setWindowTitle("Save project")
        self.buttonBox.button(QDialogButtonBox.Ok).setText("Save")
        self.buttonBox.accepted.connect(self.onSaved)


    def onSaved(self):
        # Check if filename is already existing
        if self.cbSaveTo.currentIndex() == ProjectDialog.CLOUD:
            for i in range(self.lwProjects.count()):
                item = self.lwProjects.item(i)
                if item.text() == self.filename:
                    # Project already exists
                    reply = QMessageBox.question(None, 'Project already exists',
                        "Another project with the same name <br> \"<b>{}</b>\""
                        "already exists.<br><br>Do you want to overwrite it?".format(self.filename),
                        QMessageBox.Yes | QMessageBox.No, QMessageBox.No)
                    if reply == QMessageBox.No:
                        return
                    break
        elif self.cbSaveTo.currentIndex() == ProjectDialog.LOCAL:
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

