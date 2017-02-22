#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
"""This module contains a class which represents the main window of the
    application and includes all relevant panels and the main toolbar.
"""
from collections import OrderedDict
from enum import Enum
import os.path

from PyQt4.QtCore import Qt, pyqtSignal, pyqtSlot
from PyQt4.QtGui import (QAction, QActionGroup, QMainWindow, QMenu, QSplitter,
                         QToolButton, qApp)

import karabo_gui.icons as icons
from karabo.middlelayer import AccessLevel
import karabo_gui.globals as krb_globals
from karabo_gui.events import (KaraboBroadcastEvent, KaraboEventSender,
                               register_for_broadcasts)
from karabo_gui.messagebox import MessageBox
from karabo_gui.panels.configurationpanel import ConfigurationPanel
from karabo_gui.panels.container import PanelContainer
from karabo_gui.panels.loggingpanel import LoggingPanel
from karabo_gui.panels.navigationpanel import NavigationPanel
from karabo_gui.panels.notificationpanel import NotificationPanel
from karabo_gui.panels.projectpanel import ProjectPanel
from karabo_gui.panels.scriptingpanel import ScriptingPanel
from karabo_gui.singletons.api import get_db_conn, get_network

ACCESS_LEVELS = OrderedDict()
ACCESS_LEVELS['Admin'] = AccessLevel.ADMIN
ACCESS_LEVELS['Expert'] = AccessLevel.EXPERT
ACCESS_LEVELS['Operator'] = AccessLevel.OPERATOR
ACCESS_LEVELS['User'] = AccessLevel.USER
ACCESS_LEVELS['Observer'] = AccessLevel.OBSERVER


class PanelAreaEnum(Enum):
    """The different panel areas in the main window
    """
    LeftBottom = 0
    LeftTop = 1
    MiddleBottom = 2
    MiddleTop = 3
    Right = 4


class MainWindow(QMainWindow):
    signalQuitApplication = pyqtSignal()
    signalGlobalAccessLevelChanged = pyqtSignal()

    def __init__(self):
        super(MainWindow, self).__init__()

        # Create projects folder, if not exists
        if not os.path.exists(krb_globals.KARABO_PROJECT_FOLDER):
            os.makedirs(krb_globals.KARABO_PROJECT_FOLDER, exist_ok=True)

        self._setupActions()
        self._setupMenuBar()
        self._setupToolBar()
        self._setupStatusBar()

        self._panel_areas = {}
        self._setupPanelAreas()
        self._addFixedPanels()

        title = "European XFEL - Karabo GUI " + krb_globals.GUI_VERSION_LONG
        self.setWindowTitle(title)
        self.resize(1200, 800)

        # Register to KaraboBroadcastEvent, Note: unregister_from_broadcasts is
        # not necessary for self due to the fact that the singleton mediator
        # object and `self` are being destroyed when the GUI exists
        register_for_broadcasts(self)

    # --------------------------------------
    # Qt virtual methods

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
            elif sender is KaraboEventSender.MaximizePanel:
                self._panelContainerMaximized(data.get('container'))
            elif sender is KaraboEventSender.MinimizePanel:
                self._panelContainerMinimized(data.get('container'))
            return False
        return super(MainWindow, self).eventFilter(obj, event)

    # --------------------------------------
    # public methods

    def addPanel(self, panel, area_enum):
        """Add a panel to an area's container in this window
        """
        container = self._panel_areas[area_enum]
        container.addPanel(panel)
        container.setCurrentWidget(panel)

    def removePanel(self, panel, area_enum):
        """Remove a panel from a container in this window
        """
        container = self._panel_areas[area_enum]
        container.removePanel(panel)

    def selectPanel(self, panel, area_enum):
        """This method raises a panel to the top of the focus stack, regardless
        of its current docked/undocked status.
        """
        container = self._panel_areas[area_enum]
        index = container.indexOf(panel)
        if index > -1:
            container.setCurrentIndex(index)
        else:
            panel.activateWindow()
            panel.raise_()

    # --------------------------------------
    # private methods

    def _setupActions(self):
        network = get_network()

        text = "Change access level"
        self.tbAccessLevel = QToolButton(self)
        self.tbAccessLevel.setIcon(icons.lock)
        self.tbAccessLevel.setToolTip(text)
        self.tbAccessLevel.setStatusTip(text)
        self.tbAccessLevel.setPopupMode(QToolButton.InstantPopup)
        self.tbAccessLevel.setEnabled(False)

        self.agAccessLevel = QActionGroup(self)
        self.agAccessLevel.triggered.connect(self.onChangeAccessLevel)

        self.access_level_actions = {}
        for name, level in ACCESS_LEVELS.items():
            action = QAction(name, self)
            action.setStatusTip(text)
            action.setToolTip(text)
            action.setCheckable(True)
            action.setData(level)
            self.agAccessLevel.addAction(action)
            self.access_level_actions[level] = action

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

    def _addFixedPanels(self):
        """Add panels which only need to be created once
        """
        # Left
        navigation = NavigationPanel()
        self.signalGlobalAccessLevelChanged.connect(
            navigation.onGlobalAccessLevelChanged)
        self.addPanel(navigation, PanelAreaEnum.LeftTop)
        self.addPanel(ProjectPanel(), PanelAreaEnum.LeftBottom)

        # Middle
        self.addPanel(LoggingPanel(), PanelAreaEnum.MiddleBottom)
        self.addPanel(ScriptingPanel(), PanelAreaEnum.MiddleBottom)
        self.addPanel(NotificationPanel(), PanelAreaEnum.MiddleBottom)

        # Right
        configuration = ConfigurationPanel()
        self.signalGlobalAccessLevelChanged.connect(
            configuration.onGlobalAccessLevelChanged)
        self.addPanel(configuration, PanelAreaEnum.Right)

    def _setupPanelAreas(self):
        """Build the main splitter structure of the main window
        """
        # Create a main splitter with left, middle, and right areas
        mainSplitter = QSplitter(Qt.Horizontal)
        mainSplitter.setContentsMargins(5, 5, 5, 5)
        left_area = QSplitter(Qt.Vertical, mainSplitter)
        middle_area = QSplitter(Qt.Vertical, mainSplitter)
        right_area = QSplitter(Qt.Vertical, mainSplitter)
        mainSplitter.setStretchFactor(0, 2)
        mainSplitter.setStretchFactor(1, 2)
        mainSplitter.setStretchFactor(2, 2)
        self.setCentralWidget(mainSplitter)

        # Set up the left area
        left_top = PanelContainer("Navigation", left_area)
        left_bottom = PanelContainer("Projects", left_area)
        self._panel_areas[PanelAreaEnum.LeftTop] = left_top
        self._panel_areas[PanelAreaEnum.LeftBottom] = left_bottom
        left_area.setStretchFactor(0, 2)
        left_area.setStretchFactor(1, 1)

        # Set up the middle area
        middle_top = PanelContainer("Custom view", middle_area,
                                    allow_closing=True, handle_empty=True)
        middle_bottom = PanelContainer("Output", middle_area)
        self._panel_areas[PanelAreaEnum.MiddleTop] = middle_top
        self._panel_areas[PanelAreaEnum.MiddleBottom] = middle_bottom
        middle_area.setStretchFactor(0, 6)
        middle_area.setStretchFactor(1, 1)

        # Set up the right area
        right = PanelContainer("Configuration", right_area)
        self._panel_areas[PanelAreaEnum.Right] = right

    def _quit(self):
        # Make sure there are no pending writing things in the pipe
        if get_db_conn().is_writing():
            msg = ('There is currently data fetched from or sent to the <br>'
                   '<b>project database</b>. Please wait until this is done!')
            MessageBox.showWarning(msg, 'Database connection active')
            return False

        self.signalQuitApplication.emit()
        return True

    def _updateScenes(self):
        container = self._panel_areas[PanelAreaEnum.MiddleTop]
        for panel in container.panel_set:
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
        container = self._panel_areas[PanelAreaEnum.MiddleTop]
        for panel in container.panel_set:
            panel.setEnabled(enable)

        # Update configuration panels
        container = self._panel_areas[PanelAreaEnum.Right]
        for panel in container.panel_set:
            panel.setEnabled(enable)

    def _panelContainerMaximized(self, panel_container):
        """The given `panel_container` is about to be maximized.
        """
        for area_enum, area_container in self._panel_areas.items():
            if area_container is not panel_container:
                area_container.hide()

    def _panelContainerMinimized(self, panel_container):
        """The given `panel_container` is about to be minimized.
        """
        for area_enum, area_container in self._panel_areas.items():
            if area_container is not panel_container:
                area_container.show()

    # --------------------------------------
    # Qt slots

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
        level = action.data()
        assert isinstance(level, AccessLevel), 'Garbage access level value!'
        krb_globals.GLOBAL_ACCESS_LEVEL = level

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

    @pyqtSlot()
    def onUpdateAccessLevel(self):
        global_access_level = krb_globals.GLOBAL_ACCESS_LEVEL

        # Build the access level menu
        self.mAccessLevel.clear()
        for level in ACCESS_LEVELS.values():
            if global_access_level >= level:
                action = self.access_level_actions[level]
                self.mAccessLevel.addAction(action)

        # Show a check next to the current access level
        for action in self.access_level_actions.values():
            action.setChecked(False)

        checked_action = self.access_level_actions.get(global_access_level)
        if checked_action is not None:
            checked_action.setChecked(True)
