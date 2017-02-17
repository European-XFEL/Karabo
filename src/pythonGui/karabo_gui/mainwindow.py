#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
"""This module contains a class which represents the main window of the
    application and includes all relevant panels and the main toolbar.
"""
from functools import partial
import os.path

from PyQt4.QtCore import Qt, pyqtSignal, pyqtSlot
from PyQt4.QtGui import (QAction, QActionGroup, QColor, QMainWindow, QMenu,
                         QSplitter, QToolButton, qApp)

import karabo_gui.icons as icons
from karabo.common.scenemodel.api import SceneTargetWindow
from karabo.middlelayer import AccessLevel
from karabo_gui import globals
from karabo_gui.const import ALARM_COLOR
from karabo_gui.events import (KaraboBroadcastEvent, KaraboEventSender,
                               register_for_broadcasts)
from karabo_gui.panels.alarmpanel import AlarmPanel
from karabo_gui.panels.configurationpanel import ConfigurationPanel
from karabo_gui.panels.container import PanelContainer
from karabo_gui.panels.loggingpanel import LoggingPanel
from karabo_gui.panels.macropanel import MacroPanel
from karabo_gui.messagebox import MessageBox
from karabo_gui.panels.navigationpanel import NavigationPanel
from karabo_gui.panels.notificationpanel import NotificationPanel
from karabo_gui.panels.placeholderpanel import PlaceholderPanel
from karabo_gui.panels.projectpanel import ProjectPanel
from karabo_gui.panels.runconfigpanel import RunConfigPanel
from karabo_gui.panels.scenepanel import ScenePanel
from karabo_gui.panels.scriptingpanel import ScriptingPanel
from karabo_gui.sceneview.api import SceneView
from karabo_gui.singletons.api import (
    get_db_conn, get_network
)


class MainWindow(QMainWindow):
    signalQuitApplication = pyqtSignal()
    signalGlobalAccessLevelChanged = pyqtSignal()

    def __init__(self):
        super(MainWindow, self).__init__()

        # Create projects folder, if not exists
        if not os.path.exists(globals.KARABO_PROJECT_FOLDER):
            os.makedirs(globals.KARABO_PROJECT_FOLDER, exist_ok=True)

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

    def closeEvent(self, event):
        if not self._quit():
            event.ignore()
            return

        event.accept()
        QMainWindow.closeEvent(self, event)

    def eventFilter(self, obj, event):
        if isinstance(event, KaraboBroadcastEvent):
            sender = event.sender
            data = event.data
            if sender is KaraboEventSender.DeviceDataReceived:
                self._updateScenes()
            elif sender is KaraboEventSender.DatabaseIsBusy:
                self._database_is_processing(data.get('is_processing'))
            elif sender is KaraboEventSender.MaximizeTab:
                self._tabMaximized(data.get('tab'))
            elif sender is KaraboEventSender.MinimizeTab:
                self._tabMinimized(data.get('tab'))
            return False
        return super(MainWindow, self).eventFilter(obj, event)

    def _setupActions(self):
        network = get_network()

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
        self.acServerConnect.triggered.connect(network.onServerConnection)

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

        self.leftArea = QSplitter(Qt.Vertical, mainSplitter)
        self.navigationTab = PanelContainer("Navigation", self.leftArea)
        self.navigationPanel = self.navigationTab.addPanel(NavigationPanel,
                                                           "Navigation")
        self.signalGlobalAccessLevelChanged.connect(
            self.navigationPanel.onGlobalAccessLevelChanged)
        self.leftArea.setStretchFactor(0, 2)

        self.projectTab = PanelContainer("Projects", self.leftArea)
        self.projectPanel = self.projectTab.addPanel(ProjectPanel, "Projects")
        self.leftArea.setStretchFactor(1, 1)

        self.middleArea = QSplitter(Qt.Vertical, mainSplitter)
        self.middleTab = PanelContainer("Custom view", self.middleArea)
        self.placeholderPanel = None
        self.middleArea.setStretchFactor(0, 6)

        # Container for all current alarm panels
        self.alarmPanels = {}
        # Container for all current run configuration panels
        self.runConfigPanels = {}

        self.outputTab = PanelContainer("Output", self.middleArea)
        self.loggingPanel = self.outputTab.addPanel(LoggingPanel, "Log")
        self.scriptingPanel = self.outputTab.addPanel(ScriptingPanel,
                                                      "Console")
        self.notificationPanel = self.outputTab.addPanel(NotificationPanel,
                                                         "Notifications")
        self.middleArea.setStretchFactor(1, 1)

        self.rightArea = QSplitter(Qt.Vertical, mainSplitter)
        self.configurationTab = PanelContainer("Configuration", self.rightArea)
        self.configurationPanel = self.configurationTab.addPanel(
            ConfigurationPanel, "Configuration Editor")
        self.signalGlobalAccessLevelChanged.connect(
            self.configurationPanel.onGlobalAccessLevelChanged)

        mainSplitter.setStretchFactor(0, 2)
        mainSplitter.setStretchFactor(1, 2)
        mainSplitter.setStretchFactor(2, 2)

        self.setCentralWidget(mainSplitter)
        self._addPlaceholderMiddlePanel(False)

    def _quit(self):
        # Make sure there are no pending writing things in the pipe
        if get_db_conn().is_writing():
            msg = ('There is currently data fetched from or sent to the <br>'
                   '<b>project database</b>. Please wait until this is done!')
            MessageBox.showWarning(msg, 'Database connection active')
            return False
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

        # Remove all alarm panels
        if not connectedToServer:
            rm_keys = list(self.alarmPanels.keys())
            while rm_keys:
                self._removeAlarmPanel(rm_keys.pop())

    def _removePlaceholderMiddlePanel(self):
        """The placeholder for the middle panel is removed.
        """
        self.middleTab.removePanel(self.placeholderPanel)
        self.placeholderPanel = None

    def checkAndRemovePlaceholderMiddlePanel(self):
        """ Remove placeholder from middle panel in case it makes sense.
        """
        if self.middleTab.count() == 1 and self.placeholderPanel is not None:
            # Remove start up page
            self._removePlaceholderMiddlePanel()

    def _createPlaceholderMiddlePanel(self):
        self.placeholderPanel = self.middleTab.addPanel(PlaceholderPanel,
                                                        "Start Page")

    def _getPanel(self, panelContainer, child_type, model):
        """ The associated panel of the given ``panelContainer`` for the given
            ``model`` is returned, if available.
            ``child_type`` can be 'macro_model' or 'scene_model' or
            'alarm_model'.
        """
        for panel in panelContainer.panel_set:
            panel_model = getattr(panel, child_type, None)
            if panel_model is model:
                return panel

    def focusExistingPanel(self, panelContainer, child_type, model):
        """This method checks whether a given panel for the given ``model``
           already exists.
           ``child_type`` can be either 'macro_model' or 'scene_model' or
           'alarm_model'.

           The method returns ``True``, if panel already open else ``False``.
           Note that in case the panel is already there, it is pushed to the
           top of the focus stack.
        """
        panel = self._getPanel(panelContainer, child_type, model)
        if panel is not None:
            index = panelContainer.indexOf(panel)
            if index > -1:
                panelContainer.setCurrentIndex(index)
            else:
                panel.activateWindow()
                panel.raise_()
            return True

        return False

    def renameMiddlePanel(self, child_type, model):
        """ Adapt tab text of corresponding ``model``.
            ``child_type`` can be 'macro_model' or 'scene_model'.

            The title of the panel was already changed in the project panel
            and needs to be updated.
        """
        panel = self._getPanel(self.middleTab, child_type, model)
        if panel is not None:
            index = self.middleTab.indexOf(panel)
            # Update title - important for undocked widgets
            panel.updateTitle(model.simple_name)
            if index > -1:
                self.middleTab.setTabText(index, model.simple_name)

    def removeMiddlePanel(self, child_type, model):
        """ The middle panel for the given ``model`` is removed.
            ``child_type`` can be either 'macro_model' or scene_model'.
        """
        panel = self._getPanel(self.middleTab, child_type, model)
        if panel is not None:
            self.middleTab.removePanel(panel)
        # If tabwidget is empty - show start page instead
        if self.middleTab.count() < 1:
            self._createPlaceholderMiddlePanel()

    def selectPanel(self, panelContainer, panel):
        if panelContainer.count() > 1:
            panelContainer.updateTabsClosable()
        panelContainer.setCurrentWidget(panel)

    def _removeAlarmPanel(self, instanceId):
        if instanceId in self.alarmPanels:
            panel = self.alarmPanels[instanceId]
            # Call closeEvent to unregister from broadcast events
            panel.close()
            self.outputTab.removePanel(panel)
            del self.alarmPanels[instanceId]

    def showAlarmServicePanels(self, instanceIds):
        """ Show alarm panels for the given ``instanceIds``."""
        for instId in instanceIds:
            # Check whether there is already an alarm panel for
            # this ``instanceId``
            if instId not in self.alarmPanels:
                factory = partial(AlarmPanel, instId)
                title = "Alarms for {}".format(instId)
                panel = self.outputTab.addPanel(factory, title)
                self.selectPanel(self.outputTab, panel)
                tabBar = self.outputTab.tabBar()
                tabBar.setTabTextColor(
                    self.outputTab.count()-1, QColor(*ALARM_COLOR))
                self.alarmPanels[instId] = panel
            else:
                panel = self.alarmPanels[instId]
                self.focusExistingPanel(self.outputTab, 'alarm_model',
                                        panel.alarm_model)

    def removeAlarmServicePanels(self, instanceIds):
        """ Remove alarm panels for the given ``instanceIds``."""
        for instId in instanceIds:
            self._removeAlarmPanel(instId)

    def addRunConfigPanel(self, instanceIds):
        for instId in instanceIds:
            if instId not in self.runConfigPanels:
                factory = partial(RunConfigPanel, instId)
                title = "Run configuration at {}".format(instId)
                if len(self.runConfigPanels) == 0:
                    title = "RunConfig"
                panel = self.configurationTab.addPanel(factory, title)
                self.selectPanel(self.outputTab, panel)
                self.runConfigPanels[instId] = panel

    def removeRunConfigPanels(self, instanceIds):
        """ Remove run config panels for the given ``instanceIds``."""
        for instId in instanceIds:
            self._removeRunConfigPanel(instId)

    def _removeRunConfigPanel(self, instanceId):
        if instanceId in self.runConfigPanels:
            panel = self.runConfigPanels[instanceId]
            # Call closeEvent to unregister from broadcast events
            panel.close()
            self.configurationTab.removePanel(panel)

    def addSceneView(self, sceneModel, target_window):
        """ Add a scene view to show the content of the given `sceneModel in
            the GUI.
        """
        if sceneModel is None:
            return

        self.checkAndRemovePlaceholderMiddlePanel()
        if self.focusExistingPanel(self.middleTab, 'scene_model', sceneModel):
            # XXX: we need the panel
            panel = self._getPanel(self.middleTab, 'scene_model', sceneModel)
        else:
            # Add scene view to tab widget
            sceneView = SceneView(model=sceneModel, parent=self)
            server_connected = self.acServerConnect.isChecked()
            factory = partial(ScenePanel, sceneView, server_connected)
            panel = self.middleTab.addPanel(factory, sceneModel.simple_name)
            self.selectPanel(self.middleTab, panel)

        # If it's a dialog, make sure it's undocked
        if target_window == SceneTargetWindow.Dialog:
            panel.onUndock()

    def addMacro(self, macroModel):
        """ Add a macro pane to show the content of the given `macroModel in
            the GUI.
        """
        self.checkAndRemovePlaceholderMiddlePanel()
        if self.focusExistingPanel(self.middleTab, 'macro_model', macroModel):
            return

        factory = partial(MacroPanel, macroModel)
        panel = self.middleTab.addPanel(factory, macroModel.simple_name)
        self.selectPanel(self.middleTab, panel)

    def _updateScenes(self):
        for panel in self.middleTab.panel_set:
            scene_view = getattr(panel, 'scene_view', None)
            if scene_view is not None and scene_view.isVisible():
                scene_view.update()

    def _enable_toolbar(self, enable):
        self.acServerConnect.setEnabled(enable)
        self.tbAccessLevel.setEnabled(enable)

    def _database_is_processing(self, is_processing):
        """This method gets called whenever the database is switching its
        processing mode.
        """
        enable = not is_processing
        # Update toolbar
        self._enable_toolbar(enable)

        # Update all open scenes and macros
        for panel in self.middleTab.panel_set:
            panel.setEnabled(enable)

        # Update configuration panel
        for panel in self.configurationTab.panel_set:
            panel.setEnabled(enable)

    def _tabMaximized(self, tabWidget):
        """The given /tabWidget is about to be maximized.
        """
        areas = (self.rightArea, self.middleArea, self.leftArea)
        for area in areas:
            for i in range(area.count()):
                w = area.widget(i)
                if w != tabWidget:
                    w.hide()

    def _tabMinimized(self, tabWidget):
        """The given /tabWidget is about to be minimized.
        """
        areas = (self.rightArea, self.middleArea, self.leftArea)
        for area in areas:
            for i in range(area.count()):
                w = area.widget(i)
                if w != tabWidget:
                    w.show()

    @pyqtSlot()
    def onExit(self):
        if not self._quit():
            return
        qApp.quit()

    @pyqtSlot()
    def onHelpAbout(self):
        # TODO: add about dialog for karabo including version etc.
        print("onHelpAbout")

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
