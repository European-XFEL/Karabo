#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on January 9, 2015
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


__all__ = ["ProjectSaveDialog", "ProjectLoadDialog"]

import globals
import icons
import network

from PyQt4 import uic
from PyQt4.QtCore import (QDir, Qt)
from PyQt4.QtGui import (QDialog, QDialogButtonBox, QFileSystemModel,
                         QItemSelectionModel, QMessageBox, QMovie, QPalette,
                         QTreeWidgetItem)

import os.path
import datetime


class ProjectDialog(QDialog):

    CLOUD = 0
    LOCAL = 1

    def __init__(self):
        QDialog.__init__(self)
        uic.loadUi(os.path.join(os.path.dirname(__file__), 'projectdialog.ui'), self)
        
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
        splitted = f.split(".")
        if len(splitted) > 1:
            f = splitted[0]
        return "{}.krb".format(f)


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
        if self.twProjects.topLevelItemCount() > 0:
            self.twProjects.clear()
 
        # Fill all projects from cloud into the view
        for k in projects.keys():
            checkedOut = projects.get("{}.checkedOut".format(k))
            
            item = QTreeWidgetItem(self.twProjects)
            item.setData(0, Qt.UserRole, checkedOut)
            item.setIcon(0, icons.lock if checkedOut else icons.folder)
            item.setText(0, k)
            
            item.setText(1, "True" if checkedOut else "False")
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
        
        if self.swSaveTo.currentIndex() == 2:
            self.swSaveTo.setCurrentIndex(ProjectDialog.CLOUD)
        
        if not self.leFilename.isEnabled():
            self.leFilename.setEnabled(True)
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


class ProjectSaveDialog(ProjectDialog):


    def __init__(self):
        ProjectDialog.__init__(self)
        
        self.setWindowTitle("Save project")
        self.buttonBox.button(QDialogButtonBox.Ok).setText("Save")
        self.buttonBox.accepted.connect(self.onSaved)

        self.twProjects.itemDoubleClicked.connect(self.onSaved)
        self.twLocal.doubleClicked.connect(self.onSaved)


    def onSaved(self):
        # Check if filename is already existing
        if self.cbSaveTo.currentIndex() == ProjectDialog.CLOUD:
            for i in range(self.twProjects.topLevelItemCount()):
                item = self.twProjects.topLevelItem(i)
                if item.text(0) == self.filename:
                    checkedOut = item.data(0, Qt.UserRole)
                    if checkedOut:
                        # Project is locked
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
        
        self.twProjects.itemDoubleClicked.connect(self.accept)
        self.twLocal.doubleClicked.connect(self.accept)

