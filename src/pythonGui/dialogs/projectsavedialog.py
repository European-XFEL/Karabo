#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on January 9, 2015
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


import globals

from PyQt4 import uic
from PyQt4.QtCore import (QDir, Qt)
from PyQt4.QtGui import (QDialog, QDialogButtonBox, QFileSystemModel,
                         QMessageBox, QMovie, QPalette)

import os.path


class ProjectSaveDialog(QDialog):


    def __init__(self):
        QDialog.__init__(self)
        uic.loadUi(os.path.join(os.path.dirname(__file__), 'projectsavedialog.ui'), self)
        
        self.lwProjects.currentItemChanged.connect(self.onCloudProjectChanged)
        
        self.fileSystemModel = QFileSystemModel(self)
        self.fileSystemModel.setRootPath(QDir.homePath())
        self.twLocal.setModel(self.fileSystemModel)
        
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
        self.buttonBox.accepted.connect(self.onAccepted)


    @property
    def filename(self):
        return self.leFilename.text()


    def fillCloudProjects(self, projects):
        if self.lwProjects.count() > 0:
            self.lwProjects.clear()
        
        # Fill all projects from cloud into table view
        self.lwProjects.addItems(projects)
        if self.swSaveTo.currentIndex() == 2:
            self.swSaveTo.setCurrentIndex(0)
        
        if not self.leFilename.isEnabled():
            self.leFilename.setEnabled(True)


    def onAccepted(self):
        # Check if filename is already existing
        if self.cbSaveTo.currentIndex() == 0:
            for i in range(self.lwProjects.count()):
                item = self.lwProjects.item(i)
                if item.text() == self.filename:
                    # Project already exists
                    reply = QMessageBox.question(None, 'Project already exists',
                        "Another project with the same name <br> \"<b>{}</b>\""
                        "already exists.<br><br>Do you want to overwrite it?".format(self.filename),
                        QMessageBox.Yes | QMessageBox.No, QMessageBox.No)

                    break
        elif self.cbSaveTo.currentIndex() == 1:
            fi = self.fileSystemModel.fileInfo(self.twLocal.currentIndex())
            print("absolutePath", fi.absolutePath())
        
        self.accept()


    def onChanged(self, text):
        self.buttonBox.button(QDialogButtonBox.Ok).setEnabled(len(text) > 0)


    def onCloudProjectChanged(self, item): # previousItem
        self.leFilename.setText(item.text())

