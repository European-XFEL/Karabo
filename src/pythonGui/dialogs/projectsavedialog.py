#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on January 9, 2015
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


import globals

from PyQt4 import uic
from PyQt4.QtCore import (QDir, Qt)
from PyQt4.QtGui import (QDialog, QFileSystemModel, QLabel, QMovie, QPalette)

import os.path


class ProjectSaveDialog(QDialog):


    def __init__(self):
        QDialog.__init__(self)
        uic.loadUi(os.path.join(os.path.dirname(__file__), 'projectsavedialog.ui'), self)
        
        self.fileSystemModel = QFileSystemModel(self)
        self.fileSystemModel.setRootPath(QDir.homePath())
        #self.fileSystemModel.setFilter(QDir.AllDirs | QDir.NoDotAndDotDot)
        self.twLocal.setModel(self.fileSystemModel)
        
        rootDir = os.path.join(QDir.homePath(), globals.HIDDEN_KARABO_FOLDER)
        index = self.fileSystemModel.index(rootDir)
        self.twLocal.setCurrentIndex(index)
        self.twLocal.resizeColumnToContents(0)
        
        # Cloud view is waiting for available projects - show waiting
        self.laWait.setAlignment(Qt.AlignHCenter | Qt.AlignVCenter)
        self.laWait.setAutoFillBackground(True)
        self.laWait.setBackgroundRole(QPalette.Base)
        
        movie = QMovie(os.path.join("icons", "wait"))
        self.laWait.setMovie(movie)
        movie.start()
        
        self.swSaveTo.setCurrentIndex(2)


    def fillCloudProjects(self, projects):
        print("fillCloudProjects")
        # Remove waiting icon
        
        # Fill all projects from cloud into table view


    @property
    def filename(self):
        return self.leFilename.text()


    def setAvailableCloudProjects(self, projects):
        if self.lwProjects.count() > 0:
            self.lwProjects.clear()
        
        self.lwProjects.addItems(projects)
        if self.swSaveTo.currentIndex() == 2:
            self.swSaveTo.setCurrentIndex(0)
        
        if not self.leFilename.isEnabled():
            self.leFilename.setEnabled(True)

