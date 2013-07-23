#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the main window of the application
   and includes all relevant panels and the main toolbar.
"""

__all__ = ["MainWindow"]


import qrc_icons

from docktabwindow import DockTabWindow
import globals
from manager import Manager
from network import Network

from panels.configurationpanel import ConfigurationPanel
from panels.custommiddlepanel import CustomMiddlePanel
from panels.loggingpanel import LoggingPanel
from panels.navigationpanel import NavigationPanel
from panels.notificationpanel import NotificationPanel
from panels.projectpanel import ProjectPanel
from panels.scriptingpanel import ScriptingPanel

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class MainWindow(QMainWindow):

    def __init__(self):
        super(MainWindow, self).__init__()
        
        self._setupPanels()
        self._setupNetwork()
        
        self._setupActions()
        self._setupMenuBar()
        self._setupToolBar()
        self._setupStatusBar()

        self.setWindowTitle('European XFEL - Karabo GUI')
        self.resize(1200,800)
        self.show()


### initializations ###

    def _setupActions(self):
        text = "Change access level"
        self.__tbAccessLevel = QToolButton(self)
        self.__tbAccessLevel.setIcon(QIcon(":user"))
        self.__tbAccessLevel.setToolTip(text)
        self.__tbAccessLevel.setStatusTip(text)
        self.__tbAccessLevel.setPopupMode(QToolButton.InstantPopup)
        
        text = "Observer"
        self.__acObserver = QAction(text, self)
        self.__acObserver.setStatusTip(text)
        self.__acObserver.setToolTip(text)
        self.__acObserver.setCheckable(True)
        
        text = "User"
        self.__acUser = QAction(text, self)
        self.__acUser.setStatusTip(text)
        self.__acUser.setToolTip(text)
        self.__acUser.setCheckable(True)

        text = "Expert"
        self.__acExpert = QAction(text, self)
        self.__acExpert.setStatusTip(text)
        self.__acExpert.setToolTip(text)
        self.__acExpert.setCheckable(True)
        self.__acExpert.setChecked(True)
        
        text = "Admin"
        self.__acAdmin = QAction(text, self)
        self.__acAdmin.setStatusTip(text)
        self.__acAdmin.setToolTip(text)
        self.__acAdmin.setCheckable(True)
        
        self.__agAccessLevel = QActionGroup(self)
        self.__agAccessLevel.addAction(self.__acObserver)
        self.__agAccessLevel.addAction(self.__acUser)
        self.__agAccessLevel.addAction(self.__acExpert)
        self.__agAccessLevel.addAction(self.__acAdmin)
        self.__agAccessLevel.triggered.connect(self.onChangeAccessLevel)
        
        self.__mAccessLevel = QMenu()
        self.__mAccessLevel.addAction(self.__acObserver)
        self.__mAccessLevel.addAction(self.__acUser)
        self.__mAccessLevel.addAction(self.__acExpert)
        self.__mAccessLevel.addAction(self.__acAdmin)
        self.__tbAccessLevel.setMenu(self.__mAccessLevel)
        
        text = "Connect to server"
        self.__acRemote = QAction(QIcon(":remote"), "&Connect to server", self)
        self.__acRemote.setStatusTip(text)
        self.__acRemote.setToolTip(text)
        self.__acRemote.triggered.connect(self.__network.onStartConnection)

        text = "Exit application"
        self.__acExit = QAction(QIcon(':exit'), '&Exit', self)
        self.__acExit.setStatusTip(text)
        self.__acExit.setToolTip(text)
        self.__acExit.setShortcut('Ctrl+Q')
        self.__acExit.triggered.connect(self.onExit)

        self.__acEditUndo = QAction(QIcon(":undo"), "Undo", self)
        self.__acEditUndo.setShortcuts(QKeySequence.Undo)

        self.__acEditRedo = QAction(QIcon(":redo"), "Redo", self)
        self.__acEditRedo.setShortcuts(QKeySequence.Redo)

        self.__acEditCut = QAction(QIcon(":cut"), "Cut", self)
        self.__acEditCut.setShortcuts(QKeySequence.Cut)

        self.__acEditCopy = QAction(QIcon(":copy"), "Copy", self)
        self.__acEditCopy.setShortcuts(QKeySequence.Copy)

        self.__acEditPaste = QAction(QIcon(":paste"), "Paste", self)
        self.__acEditPaste.setShortcuts(QKeySequence.Cut)

        self.__acHelpAbout = QAction("About", self);
        self.__acHelpAbout.triggered.connect(self.onHelpAbout)
        
        self.__acHelpAboutQt = QAction("About Qt", self);


    def _setupMenuBar(self):

        mFileMenu = self.menuBar().addMenu("&File")
        mFileMenu.addAction(self.__acExit)

        mEditMenu = self.menuBar().addMenu("&Edit")
        mEditMenu.addAction(self.__acEditUndo)
        mEditMenu.addAction(self.__acEditRedo)
        mEditMenu.addSeparator()
        mEditMenu.addAction(self.__acEditCut)
        mEditMenu.addAction(self.__acEditCopy)
        mEditMenu.addAction(self.__acEditPaste)

        mHelpMenu = self.menuBar().addMenu("&Help")
        mHelpMenu.addAction(self.__acHelpAbout)
        mHelpMenu.addAction(self.__acHelpAboutQt)


    def _setupToolBar(self):

        toolbar = self.addToolBar('Standard')
        toolbar.addAction(self.__acExit)

        toolbar.addSeparator()
        toolbar.addAction(self.__acRemote)
        
        toolbar.addWidget(self.__tbAccessLevel)


    def _setupStatusBar(self):

        self.statusBar().showMessage('Ready...')


    def _setupPanels(self) :

        mainSplitter = QSplitter(Qt.Horizontal)
        mainSplitter.setContentsMargins(5,5,5,5)
        
        navigationPanel = NavigationPanel(Manager().treemodel)
        leftArea = QSplitter(Qt.Vertical, mainSplitter)
        self.__navigationTab = DockTabWindow("Navigation", leftArea)
        self.__navigationTab.addDockableTab(navigationPanel, "Navigation")
        leftArea.setStretchFactor(0,2)

        #projectPanel = ProjectPanel()
        #self.__projectTab = DockTabWindow("Projects", leftArea)
        #self.__projectTab.addDockableTab(projectPanel, "Projects")
        #leftArea.setStretchFactor(1,1)

        notificationPanel = NotificationPanel()
        self.__monitorTab = DockTabWindow("Notifications", leftArea)
        self.__monitorTab.addDockableTab(notificationPanel, "Notifications")
        leftArea.setStretchFactor(1,1)

        middleArea = QSplitter(Qt.Vertical, mainSplitter)
        customViewPanel = CustomMiddlePanel()
        self.__customTab = DockTabWindow("Custom view", middleArea)
        self.__customTab.addDockableTab(customViewPanel, "Custom view")
        # Add tab
        tbNewTab = QToolButton()
        tbNewTab.setIcon(QIcon(":add"))
        text = "Open a new tab"
        tbNewTab.setToolTip(text)
        tbNewTab.setStatusTip(text)
        self.__customTab.addCornerWidget(tbNewTab)
        tbNewTab.clicked.connect(self.onOpenNewCustomViewTab)
        middleArea.setStretchFactor(0, 10)

        loggingPanel = LoggingPanel()
        scriptingPanel = ScriptingPanel()
        self.__outputTab = DockTabWindow("Update", middleArea)
        self.__outputTab.addDockableTab(loggingPanel, "Log")
        self.__outputTab.addDockableTab(scriptingPanel, "Console")
        middleArea.setStretchFactor(1,1)

        configurationPanel = ConfigurationPanel(Manager().treemodel)
        rightArea = QSplitter(Qt.Vertical, mainSplitter)
        self.__configurationTab = DockTabWindow("Configurator", rightArea)
        self.__configurationTab.addDockableTab(configurationPanel, "Configurator")

        mainSplitter.setStretchFactor(0,1)
        mainSplitter.setStretchFactor(1,3)
        mainSplitter.setStretchFactor(2,1)

        self.setCentralWidget(mainSplitter)


    def _setupNetwork(self):
        self.__network = Network()


### virtual functions ###
    def closeEvent(self, event):
        reply = QMessageBox.question(self, 'Message',
            "Are you sure to quit?", QMessageBox.Yes |
            QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.Yes:
            self.__network.onEndConnection()
            event.accept()
        else:
            event.ignore()


### slots ###
    def onExit(self):
        self.__network.onEndConnection()
        qApp.quit()

    
    def onHelpAbout(self):
        print "onHelpAbout"


    def onOpenNewCustomViewTab(self):
        customViewPanel = CustomMiddlePanel()
        self.__customTab.addDockableTab(customViewPanel, "Custom view")
        self.__customTab.updateTabsClosable()


    def onChangeAccessLevel(self, action):
        if action is self.__acObserver:
            print "observer"
            globals.GLOBAL_ACCESS_LEVEL = 0
        elif action is self.__acUser:
            print "user"
            globals.GLOBAL_ACCESS_LEVEL = 1
        elif action is self.__acExpert:
            print "expert"
            globals.GLOBAL_ACCESS_LEVEL = 2
        elif action is self.__acAdmin:
            print "admin"
            globals.GLOBAL_ACCESS_LEVEL = 3

