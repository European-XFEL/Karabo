#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the main window of the application
   and includes all relevant panels and the main toolbar.
"""

__all__ = ["MainWindow"]

import os.path
from sys import platform
import qrc_icons # hopefully this goes one day
import icons

from docktabwindow import DockTabWindow
import globals
from enums import AccessLevel
from network import Network

from panels.configurationpanel import ConfigurationPanel
from panels.custommiddlepanel import CustomMiddlePanel
from panels.loggingpanel import LoggingPanel
from panels.navigationpanel import NavigationPanel
from panels.notificationpanel import NotificationPanel
from panels.placeholderpanel import PlaceholderPanel
from panels.projectpanel import ProjectPanel
from panels.scriptingpanel import ScriptingPanel

from PyQt4.QtCore import pyqtSignal, Qt
from PyQt4.QtGui import (QAction, QActionGroup, qApp, QKeySequence,
                         QMainWindow, QMenu, QMessageBox, QSplitter, QToolButton)


class MainWindow(QMainWindow):
    signalQuitApplication = pyqtSignal()
    signalGlobalAccessLevelChanged = pyqtSignal()

    def __init__(self):
        super(MainWindow, self).__init__()

        # Create projects folder, if not existent yet
        if not os.path.exists(globals.KARABO_PROJECT_FOLDER):
            os.mkdir(globals.KARABO_PROJECT_FOLDER)

        self.karaboVersion = self._getVersion()

        self._setupActions()
        self._setupMenuBar()
        self._setupToolBar()
        self._setupStatusBar()
        
        self._setupPanels()

        self.setWindowTitle("European XFEL - Karabo GUI " + self.karaboVersion)
        self.resize(1200,800)
        self.show()


### initializations ###
    def _getVersion(self):
        if "win" in platform:
            filePath = os.path.join(os.environ['USERPROFILE'], "karabo", "karaboFramework")
        else:
            filePath = os.path.join(globals.HIDDEN_KARABO_FOLDER, "karaboFramework")
        
        try:
            with open(filePath, 'r') as file:
                karaboVersionPath = os.path.join(file.readline().rstrip(), "VERSION")
        except IOError, e:
            print e
            return ""

        try:
            with open(karaboVersionPath, 'r') as file:
                return file.readline()
        except IOError:
            return ""


    def _setupActions(self):
        text = "Change access level"
        self.tbAccessLevel = QToolButton(self)
        self.tbAccessLevel.setIcon(icons.lock)
        self.tbAccessLevel.setToolTip(text)
        self.tbAccessLevel.setStatusTip(text)
        self.tbAccessLevel.setPopupMode(QToolButton.InstantPopup)
        self.tbAccessLevel.setEnabled(False)
        
        text = "Admin"
        self.acAdmin = QAction(text, self)
        self.acAdmin.setStatusTip(text)
        self.acAdmin.setToolTip(text)
        self.acAdmin.setCheckable(True)
        
        text = "Expert"
        self.acExpert = QAction(text, self)
        self.acExpert.setStatusTip(text)
        self.acExpert.setToolTip(text)
        self.acExpert.setCheckable(True)

        text = "Operator"
        self.acOperator = QAction(text, self)
        self.acOperator.setStatusTip(text)
        self.acOperator.setToolTip(text)
        self.acOperator.setCheckable(True)

        text = "User"
        self.acUser = QAction(text, self)
        self.acUser.setStatusTip(text)
        self.acUser.setToolTip(text)
        self.acUser.setCheckable(True)

        text = "Observer"
        self.acObserver = QAction(text, self)
        self.acObserver.setStatusTip(text)
        self.acObserver.setToolTip(text)
        self.acObserver.setCheckable(True)
        
        self.agAccessLevel = QActionGroup(self)
        self.agAccessLevel.addAction(self.acAdmin)
        self.agAccessLevel.addAction(self.acExpert)
        self.agAccessLevel.addAction(self.acOperator)
        self.agAccessLevel.addAction(self.acUser)
        self.agAccessLevel.addAction(self.acObserver)
        self.agAccessLevel.triggered.connect(self.onChangeAccessLevel)
        
        self.mAccessLevel = QMenu()
        self.onUpdateAccessLevel()
        self.tbAccessLevel.setMenu(self.mAccessLevel)
        
        text = "Connect to server"
        self.acServerConnect = QAction(icons.remote,
                                         "&Connect to server", self)
        self.acServerConnect.setStatusTip(text)
        self.acServerConnect.setToolTip(text)
        self.acServerConnect.setCheckable(True)
        self.acServerConnect.triggered.connect(Network().onServerConnection)

        text = "Exit application"
        self.acExit = QAction(icons.exit, '&Exit', self)
        self.acExit.setStatusTip(text)
        self.acExit.setToolTip(text)
        self.acExit.setShortcut('Ctrl+Q')
        self.acExit.triggered.connect(self.onExit)

        self.acHelpAbout = QAction("About", self);
        self.acHelpAbout.triggered.connect(self.onHelpAbout)
        
        self.acHelpAboutQt = QAction("About Qt", self);
        self.acHelpAboutQt.triggered.connect(qApp.aboutQt)


    def _setupMenuBar(self):

        mFileMenu = self.menuBar().addMenu("&File")
        mFileMenu.addAction(self.acExit)

        mHelpMenu = self.menuBar().addMenu("&Help")
        mHelpMenu.addAction(self.acHelpAbout)
        mHelpMenu.addAction(self.acHelpAboutQt)


    def _setupToolBar(self):

        toolbar = self.addToolBar('Standard')
        toolbar.addAction(self.acExit)

        toolbar.addSeparator()
        toolbar.addAction(self.acServerConnect)
        
        toolbar.addWidget(self.tbAccessLevel)


    def _setupStatusBar(self):

        self.statusBar().showMessage('Ready...')


    def _setupPanels(self):

        mainSplitter = QSplitter(Qt.Horizontal)
        mainSplitter.setContentsMargins(5,5,5,5)
        
        self.navigationPanel = NavigationPanel()
        leftArea = QSplitter(Qt.Vertical, mainSplitter)
        self.navigationTab = DockTabWindow("Navigation", leftArea)
        self.navigationTab.addDockableTab(self.navigationPanel, "Navigation")
        self.signalGlobalAccessLevelChanged.connect(self.navigationPanel.onGlobalAccessLevelChanged)
        leftArea.setStretchFactor(0,2)

        self.projectPanel = ProjectPanel()
        self.projectPanel.signalAddScene.connect(self.onAddScene)
        self.projectPanel.signalRemoveScene.connect(self.onRemoveScene)
        self.projectTab = DockTabWindow("Projects", leftArea)
        self.projectTab.addDockableTab(self.projectPanel, "Projects")
        leftArea.setStretchFactor(1,1)

        middleArea = QSplitter(Qt.Vertical, mainSplitter)
        self.middleTab = DockTabWindow("Custom view", middleArea)
        self.placeholderPanel = None
        self._showStartPage(True)
        middleArea.setStretchFactor(0, 6)

        self.loggingPanel = LoggingPanel()
        self.scriptingPanel = ScriptingPanel()
        self.notificationPanel = NotificationPanel()
        self.outputTab = DockTabWindow("Output", middleArea)
        self.outputTab.addDockableTab(self.loggingPanel, "Log")
        self.outputTab.addDockableTab(self.scriptingPanel, "Console")
        self.outputTab.addDockableTab(self.notificationPanel, "Notifications")
        middleArea.setStretchFactor(1,1)

        self.configurationPanel = ConfigurationPanel()
        rightArea = QSplitter(Qt.Vertical, mainSplitter)
        self.configurationTab = DockTabWindow("Configuration", rightArea)
        self.configurationTab.addDockableTab(self.configurationPanel, "Configurator")
        self.signalGlobalAccessLevelChanged.connect(self.configurationPanel.onGlobalAccessLevelChanged)
        
        mainSplitter.setStretchFactor(0,1)
        mainSplitter.setStretchFactor(1,2)
        mainSplitter.setStretchFactor(2,3)

        self.setCentralWidget(mainSplitter)


    def _createCustomMiddlePanel(self, scene):
        """
        This function creates a new CustomMiddlePanel and returns it.
        """
        customViewPanel = CustomMiddlePanel(scene, self.acServerConnect.isChecked())
        return customViewPanel


    def _quit(self):
        self.signalQuitApplication.emit()


    def _showStartPage(self, show):
        if show:
            if self.placeholderPanel is not None:
                return
            
            # Close all projects
            self.projectPanel.closeAllProjects()
            # Add startup page
            self.placeholderPanel = PlaceholderPanel()
            self.middleTab.addDockableTab(self.placeholderPanel, "Start Page")
        else:
            # Remove startup page
            self.middleTab.removeDockableTab(self.placeholderPanel)
            self.placeholderPanel = None
            # Setup default project
            self.projectPanel.setupDefaultProject()


### virtual functions ###
    def closeEvent(self, event):
        reply = QMessageBox.question(self, 'Message',
            "Are you sure to quit?", QMessageBox.Yes |
            QMessageBox.No, QMessageBox.No)

        if reply == QMessageBox.Yes:
            self._quit()
            event.accept()
        else:
            event.ignore()
            return
        QMainWindow.closeEvent(self, event)


    def onExit(self):
        self._quit()
        qApp.quit()

    
    def onHelpAbout(self):
        # TODO: add about dialog for karabo including version etc.
        print "onHelpAbout"


    def onAddScene(self, scene):
        customView = self._createCustomMiddlePanel(scene)
        self.middleTab.addDockableTab(customView, scene.filename)
        if self.middleTab.count()-1 > 0:
            self.middleTab.updateTabsClosable()


    def onRemoveScene(self, scene):
        for i in xrange(self.middleTab.count()):
            divWidget = self.middleTab.widget(i)
            if divWidget.dockableWidget.scene == scene:
                self.middleTab.removeDockableTab(divWidget.dockableWidget)
                break


    def onChangeAccessLevel(self, action):
        if action is self.acObserver:
            globals.GLOBAL_ACCESS_LEVEL = AccessLevel.OBSERVER
        elif action is self.acUser:
            globals.GLOBAL_ACCESS_LEVEL = AccessLevel.USER
        elif action is self.acOperator:
            globals.GLOBAL_ACCESS_LEVEL = AccessLevel.OPERATOR
        elif action is self.acExpert:
            globals.GLOBAL_ACCESS_LEVEL = AccessLevel.EXPERT
        elif action is self.acAdmin:
            globals.GLOBAL_ACCESS_LEVEL = AccessLevel.ADMIN
        
        self.signalGlobalAccessLevelChanged.emit()


    def onServerConnectionChanged(self, isConnected):
        if isConnected:
            text = "Disconnect from server"
        else:
            text = "Connect to server"
        
        self.acServerConnect.setStatusTip(text)
        self.acServerConnect.setToolTip(text)
        
        self.acServerConnect.blockSignals(True)
        self.acServerConnect.setChecked(isConnected)
        self.acServerConnect.blockSignals(False)
        
        self.tbAccessLevel.setEnabled(isConnected)

        # Adapt middle panel
        self._showStartPage(not isConnected)


    def onUpdateAccessLevel(self):
        self.mAccessLevel.clear()
        if globals.GLOBAL_ACCESS_LEVEL > AccessLevel.EXPERT:
            self.mAccessLevel.addAction(self.acAdmin)
        if globals.GLOBAL_ACCESS_LEVEL > AccessLevel.OPERATOR:
            self.mAccessLevel.addAction(self.acExpert)
        if globals.GLOBAL_ACCESS_LEVEL > AccessLevel.USER:
            self.mAccessLevel.addAction(self.acOperator)
        if globals.GLOBAL_ACCESS_LEVEL > AccessLevel.OBSERVER:
            self.mAccessLevel.addAction(self.acUser)
        self.mAccessLevel.addAction(self.acObserver)
        
        if globals.GLOBAL_ACCESS_LEVEL == AccessLevel.ADMIN:
            self.acAdmin.setChecked(True)
        elif globals.GLOBAL_ACCESS_LEVEL == AccessLevel.EXPERT:
            self.acExpert.setChecked(True)
        elif globals.GLOBAL_ACCESS_LEVEL == AccessLevel.OPERATOR:
            self.acOperator.setChecked(True)
        elif globals.GLOBAL_ACCESS_LEVEL == AccessLevel.USER:
            self.acUser.setChecked(True)
        elif globals.GLOBAL_ACCESS_LEVEL == AccessLevel.OBSERVER:
            self.acObserver.setChecked(True)
        else:
            self.acAdmin.setChecked(False)
            self.acExpert.setChecked(False)
            self.acOperator.setChecked(False)
            self.acUser.setChecked(False)
            self.acObserver.setChecked(False)

