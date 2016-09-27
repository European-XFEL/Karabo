#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
"""This module contains a class which represents the main window of the
    application and includes all relevant panels and the main toolbar.
"""
import os.path

from PyQt4.QtCore import Qt, pyqtSignal, pyqtSlot
from PyQt4.QtGui import (QAction, QActionGroup, QColor, QMainWindow, QMenu,
                         QMessageBox, QSplitter, QToolButton, qApp)

import karabo_gui.icons as icons
from karabo.common.scenemodel.api import BaseIconsModel, DisplayIconsetModel
from karabo.middlelayer import AccessLevel
from karabo_gui import globals
from karabo_gui.const import ALARM_COLOR
from karabo_gui.docktabwindow import DockTabWindow
from karabo_gui.mediator import (KaraboBroadcastEvent, KaraboEventSender,
                                 register_for_broadcasts)
from karabo_gui.network import Network
from karabo_gui.panels.alarmpanel import AlarmPanel
from karabo_gui.panels.configurationpanel import ConfigurationPanel
from karabo_gui.panels.loggingpanel import LoggingPanel
from karabo_gui.panels.macropanel import MacroPanel
from karabo_gui.panels.navigationpanel import NavigationPanel
from karabo_gui.panels.notificationpanel import NotificationPanel
from karabo_gui.panels.placeholderpanel import PlaceholderPanel
from karabo_gui.panels.projectpanel import ProjectPanel
from karabo_gui.panels.scenepanel import ScenePanel
from karabo_gui.panels.scriptingpanel import ScriptingPanel
from karabo_gui.sceneview.api import SceneView


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

        # Register to KaraboBroadcastEvent, Note: unregister_from_broadcasts is
        # not necessary for self due to the fact that the singleton mediator
        # object and `self` are being destroyed when the GUI exists
        register_for_broadcasts(self)

    def eventFilter(self, obj, event):
        if isinstance(event, KaraboBroadcastEvent):
            sender = event.sender
            if sender is KaraboEventSender.OpenSceneView:
                data = event.data
                self.addSceneView(data.get('model'), data.get('project'))
                return True
            elif sender is KaraboEventSender.RemoveSceneView:
                data = event.data
                self.removeMiddlePanel('scene_model', data.get('model'))
                return True
            elif sender is KaraboEventSender.RenameSceneView:
                data = event.data
                self.renameMiddlePanel('scene_model', data.get('model'))
                return True
            elif sender is KaraboEventSender.OpenMacro:
                data = event.data
                self.addMacro(data.get('model'), data.get('project'))
                return True
            elif sender is KaraboEventSender.RemoveMacro:
                data = event.data
                self.removeMiddlePanel('macro_model', data.get('model'))
                return True
            elif sender is KaraboEventSender.RenameMacro:
                data = event.data
                self.renameMiddlePanel('macro_model', data.get('model'))
                return True
            elif sender is KaraboEventSender.ShowAlarmServices:
                data = event.data
                self.showAlarmServicePanels(data.get('instanceIds'))
                return True
            elif sender is KaraboEventSender.RemoveAlarmServices:
                data = event.data
                self.removeAlarmServicePanels(data.get('instanceIds'))
                return True
        return super(MainWindow, self).eventFilter(obj, event)

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
        self.navigationTab.addDockableTab(self.navigationPanel,
                                          "Navigation", self)
        self.signalGlobalAccessLevelChanged.connect(
            self.navigationPanel.onGlobalAccessLevelChanged)
        self.leftArea.setStretchFactor(0, 2)

        self.projectPanel = ProjectPanel()
        self.projectTab = DockTabWindow("Projects", self.leftArea)
        self.projectTab.addDockableTab(self.projectPanel, "Projects", self)
        self.leftArea.setStretchFactor(1, 1)

        self.middleArea = QSplitter(Qt.Vertical, mainSplitter)
        self.middleTab = DockTabWindow("Custom view", self.middleArea)
        self.placeholderPanel = None
        self.middleArea.setStretchFactor(0, 6)

        # Container for all current alarm panels
        self.alarmPanels = {}

        self.loggingPanel = LoggingPanel()
        self.scriptingPanel = ScriptingPanel()
        self.notificationPanel = NotificationPanel()
        self.outputTab = DockTabWindow("Output", self.middleArea)
        self.outputTab.addDockableTab(self.loggingPanel, "Log", self)
        self.outputTab.addDockableTab(self.scriptingPanel, "Console", self)
        self.outputTab.addDockableTab(self.notificationPanel,
                                      "Notifications", self)
        self.middleArea.setStretchFactor(1, 1)

        self.configurationPanel = ConfigurationPanel()
        self.rightArea = QSplitter(Qt.Vertical, mainSplitter)
        self.configurationTab = DockTabWindow("Configuration", self.rightArea)
        self.configurationTab.addDockableTab(self.configurationPanel,
                                             "Configurator", self)
        self.signalGlobalAccessLevelChanged.connect(
            self.configurationPanel.onGlobalAccessLevelChanged)

        mainSplitter.setStretchFactor(0, 2)
        mainSplitter.setStretchFactor(1, 4)
        mainSplitter.setStretchFactor(2, 2)

        self.setCentralWidget(mainSplitter)
        self._addPlaceholderMiddlePanel(False)

    def _quit(self):
        # Check for project changes
        projects = self.projectPanel.modifiedProjects()
        if projects:
            msgBox = QMessageBox(self)
            msgBox.setWindowTitle("Save changes before closing")
            msgBox.setText("Do you want to save your modified projects "
                           "before closing?")
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

    def _addPlaceholderMiddlePanel(self, connectedToServer):
        """The placholder for the middle panel is added.

        ``connectedToServer`` states whether panels, which only work with an
        active GuiServer connection are enabled.
        """
        if self.placeholderPanel is None:
            # Add startup page
            self._createPlaceholderMiddlePanel()

        # Enable or disable toolbar of project panel
        self.projectPanel.enableToolBar(connectedToServer)
        # Remove all alarm panels
        if not connectedToServer:
            rm_keys = list(self.alarmPanels.keys())
            while rm_keys:
                self._removeAlarmPanel(rm_keys.pop())

    def _removePlaceholderMiddlePanel(self):
        """The placeholder for the middle panel is removed.
        """
        self.middleTab.removeDockableTab(self.placeholderPanel)
        self.placeholderPanel = None

    def _readIconDataFromProject(self, sceneModel, project):
        """ Go through the model tree to find existing icon models like
            `DigitIconsModel`, `SelectionIconsModel`, `TextIconsModel` or
            `DisplayIconsetModel` and use their `url` to load the actual image
            data which is currently stored in the projects resources and put
            them to the model data.
        """
        def update_icon_model(parent_model):
            for child in parent_model.children:
                if isinstance(child, BaseIconsModel):
                    for icon_data in child.values:
                        url = icon_data.image
                        icon_data.data = project.getURL(url)
                elif isinstance(child, DisplayIconsetModel):
                    url = child.image
                    child.data = project.getURL(url)
                else:
                    if hasattr(child, "children"):
                        update_icon_model(child)

        # Recursively set all icon model data
        update_icon_model(sceneModel)

    def checkAndRemovePlaceholderMiddlePanel(self):
        """ Remove placeholder from middle panel in case it makes sense.
        """
        if self.middleTab.count() == 1 and self.placeholderPanel is not None:
            # Remove start up page
            self._removePlaceholderMiddlePanel()

    def _createPlaceholderMiddlePanel(self):
        self.placeholderPanel = PlaceholderPanel()
        self.middleTab.addDockableTab(self.placeholderPanel,
                                      "Start Page", self)

    def _getDivWidget(self, child_type, model):
        """ The associated divWidget for the given ``model`` is returned,
            if available. ``child_type`` can be 'macro_model' or 'scene_model'.
        """
        for divWidget in self.middleTab.divWidgetList:
            panel_model = getattr(divWidget.dockableWidget, child_type, None)
            if panel_model is model:
                return divWidget

    def focusExistingMiddlePanel(self, child_type, model):
        """ This method checks whether a middle panel for the given ``model``
            already exists.
            ``child_type`` can be either 'macro_model' or ''scene_model'.

            The method returns ``True``, if panel already open else ``False``.
            Note that in case the panel is already there, it is pushed to the
            top of the focus stack.
        """
        self.checkAndRemovePlaceholderMiddlePanel()

        divWidget = self._getDivWidget(child_type, model)
        if divWidget is not None:
            index = self.middleTab.indexOf(divWidget)
            if index > -1:
                self.middleTab.setCurrentIndex(index)
            else:
                divWidget.activateWindow()
                divWidget.raise_()
            return True

        return False

    def renameMiddlePanel(self, child_type, model):
        """ Adapt tab text of corresponding ``model``.
            ``child_type`` can be 'macro_model' or 'scene_model'.

            The title of the panel was already changed in the project panel
            and needs to be updated.
        """
        divWidget = self._getDivWidget(child_type, model)
        if divWidget is not None:
            index = self.middleTab.indexOf(divWidget)
            # Update title - important for undocked widgets
            divWidget.updateTitle(model.title)
            if index > -1:
                self.middleTab.setTabText(index, model.title)

    def removeMiddlePanel(self, child_type, model):
        """ The middle panel for the given ``model`` is removed.
            ``child_type`` can be either 'macro_model' or scene_model'.
        """
        divWidget = self._getDivWidget(child_type, model)
        if divWidget is not None:
            self.middleTab.removeDockableTab(divWidget.dockableWidget)

        # If tabwidget is empty - show start page instead
        if self.middleTab.count() < 1:
            self._createPlaceholderMiddlePanel()

    def selectLastMiddlePanel(self):
        if self.middleTab.count() > 1:
            self.middleTab.updateTabsClosable()
        self.middleTab.setCurrentIndex(self.middleTab.count() - 1)

    def _removeAlarmPanel(self, instanceId):
        if instanceId in self.alarmPanels:
            panel = self.alarmPanels[instanceId]
            # Call closeEvent to unregister from broadcast events
            panel.close()
            self.outputTab.removeDockableTab(panel)
            del panel

    def showAlarmServicePanels(self, instanceIds):
        """ Show alarm panels for the given ``instanceIds``."""
        for instId in instanceIds:
            # Check whether there is already alarm panel for this ``instanceId``
            if instId not in self.alarmPanels:
                panel = AlarmPanel(instId)
                title = "Alarms for {}".format(instId)
                self.outputTab.addDockableTab(panel, title, self)
                tabBar = self.outputTab.tabBar()
                tabBar.setTabTextColor(
                    self.outputTab.count()-1, QColor(*ALARM_COLOR))
                self.alarmPanels[instId] = panel

    def removeAlarmServicePanels(self, instanceIds):
        """ Remove alarm panels for the given ``instanceIds``."""
        for instId in instanceIds:
            self._removeAlarmPanel(instId)

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

    def addSceneView(self, sceneModel, project):
        """ Add a scene view to show the content of the given `sceneModel in
            the GUI.
        """
        if self.focusExistingMiddlePanel('scene_model', sceneModel):
            return

        # XXX: Set icon data to model, if existent (temporary solution)
        self._readIconDataFromProject(sceneModel, project)
        sceneView = SceneView(model=sceneModel, project=project)

        # Add scene view to tab widget
        scenePanel = ScenePanel(sceneView, self.acServerConnect.isChecked())
        self.middleTab.addDockableTab(scenePanel,
                                      sceneModel.title,
                                      self)
        self.selectLastMiddlePanel()

    def addMacro(self, macroModel, project):
        if self.focusExistingMiddlePanel('macro_model', macroModel):
            return
    
        # Add the project name to the macro model because it is only needed
        # at instantiation time of the macro
        macroModel.project_name = project.name
        macroPanel = MacroPanel(macroModel)
        self.middleTab.addDockableTab(macroPanel, macroModel.title, self)

        self.selectLastMiddlePanel()

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
        for divWidget in self.middleTab.divWidgetList:
            scene_view = getattr(divWidget.dockableWidget, 'scene_view', None)
            if scene_view is not None and scene_view.isVisible():
                scene_view.update()

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
