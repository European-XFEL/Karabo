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

from PyQt4.QtCore import pyqtSignal, pyqtSlot, Qt
from PyQt4.QtGui import (QAction, QActionGroup, qApp, QMainWindow, QMenu,
                         QMessageBox, QSplitter, QToolButton)

import karabo_gui.icons as icons

from karabo_gui import globals
from karabo_gui.docktabwindow import DockTabWindow
from karabo_gui.guiproject import Macro
from karabo_gui.network import Network
from karabo_gui.scene import Scene
from karabo_gui.sceneview.api import SceneView

from karabo_gui.panels.configurationpanel import ConfigurationPanel
from karabo_gui.panels.loggingpanel import LoggingPanel
from karabo_gui.panels.macropanel import MacroPanel
from karabo_gui.panels.navigationpanel import NavigationPanel
from karabo_gui.panels.notificationpanel import NotificationPanel
from karabo_gui.panels.placeholderpanel import PlaceholderPanel
from karabo_gui.panels.projectpanel import ProjectPanel
from karabo_gui.panels.scenepanel import ScenePanel
from karabo_gui.panels.scriptingpanel import ScriptingPanel

from karabo.middlelayer import AccessLevel


class MainWindow(QMainWindow):
    signalQuitApplication = pyqtSignal()
    signalGlobalAccessLevelChanged = pyqtSignal()

    def __init__(self):
        super(MainWindow, self).__init__()

        # Create projects folder, if not exists
        if not os.path.exists(globals.KARABO_PROJECT_FOLDER):
            os.mkdir(globals.KARABO_PROJECT_FOLDER)

        self._setupActions()
        self._setupMenuBar()
        self._setupToolBar()
        self._setupStatusBar()

        self._setupPanels()

        title = "European XFEL - Karabo GUI " + globals.GUI_VERSION_LONG
        self.setWindowTitle(title)
        self.resize(1200, 800)

### initializations ###
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
        self.acServerConnect = QAction(icons.remote, "&{}".format(text), self)
        self.acServerConnect.setStatusTip(text)
        self.acServerConnect.setToolTip(text)
        self.acServerConnect.setCheckable(True)
        self.acServerConnect.triggered.connect(Network().onServerConnection)

        text = "Exit"
        self.acExit = QAction(icons.exit, "&{}".format(text), self)
        self.acExit.setStatusTip(text)
        self.acExit.setToolTip(text)
        self.acExit.setShortcut('Ctrl+Q')
        self.acExit.triggered.connect(self.onExit)

        self.acHelpAbout = QAction("About", self)
        self.acHelpAbout.triggered.connect(self.onHelpAbout)

        self.acHelpAboutQt = QAction("About Qt", self)
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
        mainSplitter.setContentsMargins(5, 5, 5, 5)

        self.navigationPanel = NavigationPanel()
        self.leftArea = QSplitter(Qt.Vertical, mainSplitter)
        self.navigationTab = DockTabWindow("Navigation", self.leftArea)
        self.navigationTab.addDockableTab(self.navigationPanel, "Navigation", self)
        self.signalGlobalAccessLevelChanged.connect(self.navigationPanel.onGlobalAccessLevelChanged)
        self.leftArea.setStretchFactor(0, 2)

        self.projectPanel = ProjectPanel()
        self.projectPanel.signalAddSceneView.connect(self.addSceneView)
        self.projectPanel.signalRemoveSceneView.connect(self.removeSceneView)
        self.projectPanel.signalRenameSceneView.connect(self.renameSceneView)
        self.projectPanel.signalAddMacro.connect(self.onAddMacro)
        self.projectPanel.signalRemoveMacro.connect(self.onRemoveMacro)
        self.projectTab = DockTabWindow("Projects", self.leftArea)
        self.projectTab.addDockableTab(self.projectPanel, "Projects", self)
        self.leftArea.setStretchFactor(1, 1)

        self.middleArea = QSplitter(Qt.Vertical, mainSplitter)
        self.middleTab = DockTabWindow("Custom view", self.middleArea)
        self.placeholderPanel = None
        self._addPlaceholderMiddlePanel(False)
        # A set of opened scene models
        self._openedScenes = set()
        self.middleArea.setStretchFactor(0, 6)

        self.loggingPanel = LoggingPanel()
        self.scriptingPanel = ScriptingPanel()
        self.notificationPanel = NotificationPanel()
        self.outputTab = DockTabWindow("Output", self.middleArea)
        self.outputTab.addDockableTab(self.loggingPanel, "Log", self)
        self.outputTab.addDockableTab(self.scriptingPanel, "Console", self)
        self.outputTab.addDockableTab(self.notificationPanel, "Notifications", self)
        self.middleArea.setStretchFactor(1, 1)

        self.configurationPanel = ConfigurationPanel()
        self.rightArea = QSplitter(Qt.Vertical, mainSplitter)
        self.configurationTab = DockTabWindow("Configuration", self.rightArea)
        self.configurationTab.addDockableTab(self.configurationPanel, "Configurator", self)
        self.signalGlobalAccessLevelChanged.connect(self.configurationPanel.onGlobalAccessLevelChanged)

        mainSplitter.setStretchFactor(0, 2)
        mainSplitter.setStretchFactor(1, 6)
        mainSplitter.setStretchFactor(2, 1)

        self.setCentralWidget(mainSplitter)

    def _quit(self):
        # Check for project changes
        projects = self.projectPanel.modifiedProjects()
        if projects:
            msgBox = QMessageBox(self)
            msgBox.setWindowTitle("Save changes before closing")
            msgBox.setText("Do you want to save your modified projects before closing?")
            msgBox.setStandardButtons(QMessageBox.Save | QMessageBox.Discard |
                                      QMessageBox.Cancel)
            msgBox.setDefaultButton(QMessageBox.Save)

            reply = msgBox.exec_()
            if reply == QMessageBox.Cancel:
                return False

            if reply == QMessageBox.Discard:
                for p in projects:
                    p.setModified(False)

        self.projectPanel.closeAllProjects()
        self.signalQuitApplication.emit()

        return True

    def _addPlaceholderMiddlePanel(self, enableProjectPanel):
        """The placholder for the middle panel is added.

        ``enableProjectPanel`` states whether the toolbar of the project panel
        should be enabled.
        """
        if self.placeholderPanel is None:
            # Add startup page
            self._createPlaceholderMiddlePanel()

        # Enable or disable toolbar of project panel
        self.projectPanel.enableToolBar(enableProjectPanel)

    def _removePlaceholderMiddlePanel(self):
        """The placeholder for the middle panel is removed.
        """
        self.middleTab.removeDockableTab(self.placeholderPanel)
        self.placeholderPanel = None

    def checkAndRemovePlaceholderMiddlePanel(self):
        """ Remove placeholder from middle panel in case it makes sense.
        """
        if self.middleTab.count() == 1 and self.placeholderPanel is not None:
            # Remove start up page
            self._removePlaceholderMiddlePanel()

    def _createPlaceholderMiddlePanel(self):
        self.placeholderPanel = PlaceholderPanel()
        self.middleTab.addDockableTab(self.placeholderPanel, "Start Page", self)

    def _getSceneDivWidget(self, sceneModel):
        """ The associated divWidget for the given `sceneModel` is returned,
            if available.
        """
        for divWidget in self.middleTab.divWidgetList:
            if hasattr(divWidget.dockableWidget, "scene_view"):
                view = divWidget.dockableWidget.scene_view
                model = view.scene_model
                if model is sceneModel:
                    return divWidget

    def middlePanelExists(self, child_type, obj):
        """This method checks whether a middle panel for the given ``obj``
        already exists. Currently `child_type` can only be "macro".

        The method returns ``True``, if panel already open else ``False``
        """
        self.checkAndRemovePlaceholderMiddlePanel()

        # Check whether scene is already open
        for i in range(self.middleTab.count()):
            divWidget = self.middleTab.widget(i)
            if getattr(divWidget.dockableWidget, child_type, None) is obj:
                # Child already open
                self.middleTab.setCurrentIndex(i)
                return True

        return False

    def isMiddlePanelUndocked(self, obj):
        """This methods states whether an undocked panel for the given
        ``obj`` already exists.
        """
        panel = None
        parent = None
        if isinstance(obj, Macro):
            if obj.editor is not None:
                # The macro might be in its own window
                panel = obj.editor

        if panel is None:
            return False

        parent = panel.parentWidget()
        if parent is not None:
            panel.activateWindow()
            panel.raise_()
            return True
        return False

    def removeMiddlePanel(self, child_type, obj):
        """ The middle panel for the given `obj` is removed. Currently
            `child_type` can only be "macro".
        """
        for w in self.middleTab.divWidgetList:
            if getattr(w.dockableWidget, child_type, None) is obj:
                self.middleTab.removeDockableTab(w.dockableWidget)
                break

        self.onMiddlePanelRemoved()

    def selectLastMiddlePanel(self):
        if self.middleTab.count() > 1:
            self.middleTab.updateTabsClosable()
        self.middleTab.setCurrentIndex(self.middleTab.count() - 1)

### virtual functions ###
    def closeEvent(self, event):
        if not self._quit():
            event.ignore()
            return

        event.accept()
        QMainWindow.closeEvent(self, event)

    @pyqtSlot()
    def onExit(self):
        if not self._quit():
            return
        qApp.quit()

    @pyqtSlot()
    def onHelpAbout(self):
        # TODO: add about dialog for karabo including version etc.
        print("onHelpAbout")

    @pyqtSlot()
    def onMiddlePanelRemoved(self):
        # If tabwidget is empty - show start page instead
        if self.middleTab.count() < 1:
            self._createPlaceholderMiddlePanel()

    @pyqtSlot(object, object)
    def addSceneView(self, sceneModel, project):
        """ Add a scene view to show the content of the given `sceneModel in
            the GUI.
        """
        if sceneModel not in self._openedScenes:
            self.checkAndRemovePlaceholderMiddlePanel()

            sceneView = SceneView(model=sceneModel, project=project)
            # XXX: TODO scenelink
            #sceneView.signalSceneLinkTriggered.connect(self.openSceneLink)

            # Add scene view to tab widget
            scenePanel = ScenePanel(sceneView, self.acServerConnect.isChecked())
            scenePanel.signalClosed.connect(self.onMiddlePanelRemoved)
            self.middleTab.addDockableTab(scenePanel,
                                          sceneModel.filename,
                                          self)
            self._openedScenes.add(sceneModel)
            self.selectLastMiddlePanel()
        else:
            divWidget = self._getSceneDivWidget(sceneModel)
            if divWidget is not None:
                index = self.middleTab.indexOf(divWidget)
                if index > -1:
                    self.middleTab.setCurrentIndex(index)
                else:
                    divWidget.activateWindow()
                    divWidget.raise_()

    @pyqtSlot(object)
    def removeSceneView(self, sceneModel):
        """ Remove the tab which is associated to the given `sceneModel`.
        """
        if sceneModel in self._openedScenes:
            # XXX: TODO scenelink
            #sceneView.signalSceneLinkTriggered.disconnect(self.openSceneLink)

            divWidget = self._getSceneDivWidget(sceneModel)
            if divWidget is not None:
                self.middleTab.removeDockableTab(divWidget.dockableWidget)
                self.onMiddlePanelRemoved()
            self._openedScenes.remove(sceneModel)

    @pyqtSlot(object)
    def renameSceneView(self, sceneModel):
        """ Adapt tab text of corresponding `sceneModel`.

            The filename of the scene was already changed in the project panel
            and needs to be updated.
        """
        if sceneModel in self._openedScenes:
            divWidget = self._getSceneDivWidget(sceneModel)
            if divWidget is not None:
                index = self.middleTab.indexOf(divWidget)
                # Update title - important for undocked widgets
                divWidget.updateTitle(sceneModel.filename)
                if index > -1:
                    self.middleTab.setTabText(index, sceneModel.filename)

    @pyqtSlot(object)
    def onAddMacro(self, macro):
        if self.middlePanelExists("macro", macro):
            return

        if self.isMiddlePanelUndocked(macro):
            return

        macroView = MacroPanel(macro)
        self.middleTab.addDockableTab(macroView, macro.name, self)
        macro.editor = macroView

        self.selectLastMiddlePanel()

    @pyqtSlot(object)
    def onRemoveMacro(self, macro):
        self.removeMiddlePanel("macro", macro)

    @pyqtSlot(object)
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

    @pyqtSlot(bool)
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
        self._addPlaceholderMiddlePanel(isConnected)

    @pyqtSlot()
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

    @pyqtSlot()
    def onUpdateScenes(self):
        for i in range(self.middleTab.count()):
            divWidget = self.middleTab.widget(i)
            if hasattr(divWidget.dockableWidget, "scene"):
                scene = divWidget.dockableWidget.scene
                if scene.isVisible():
                    scene.update()

    @pyqtSlot(object)
    def onTabMaximized(self, tabWidget):
        """
        The given /tabWidget is about to be maximized.
        """
        areas = (self.rightArea, self.middleArea, self.leftArea)
        for area in areas:
            for i in range(area.count()):
                w = area.widget(i)
                if w != tabWidget:
                    w.hide()

    @pyqtSlot(object)
    def onTabMinimized(self, tabWidget):
        """
        The given /tabWidget is about to be minimized.
        """
        areas = (self.rightArea, self.middleArea, self.leftArea)
        for area in areas:
            for i in range(area.count()):
                w = area.widget(i)
                if w != tabWidget:
                    w.show()
