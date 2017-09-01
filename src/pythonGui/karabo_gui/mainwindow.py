#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from collections import OrderedDict
from enum import Enum
import os.path

from PyQt4.QtCore import QEvent, Qt, pyqtSlot
from PyQt4.QtGui import (
    QAction, QActionGroup, QLabel, QMainWindow, QMenu, QMessageBox,
    QSizePolicy, QSplitter, QToolButton, QWidget, qApp
)

import karabo_gui.icons as icons
from karabo.middlelayer import AccessLevel
import karabo_gui.globals as krb_globals
from karabo_gui.events import (
    KaraboEventSender, broadcast_event, register_for_broadcasts)
from karabo_gui.panels.configurationpanel import ConfigurationPanel
from karabo_gui.panels.container import PanelContainer
from karabo_gui.panels.loggingpanel import LoggingPanel
from karabo_gui.panels.navigationpanel import NavigationPanel
from karabo_gui.panels.notificationpanel import NotificationPanel
from karabo_gui.panels.projectpanel import ProjectPanel
from karabo_gui.panels.scriptingpanel import ScriptingPanel
from karabo_gui.singletons.api import (get_db_conn, get_network,
                                       get_project_model)

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


class _PatternMatcher(object):
    """A tiny state machine which watches for a single pattern.

    Useful for watching for specific key sequences...
    """
    def __init__(self, pattern):
        self.pattern = pattern
        self.index = 0

    def check(self, letter):
        if letter == self.pattern[self.index]:
            self.index += 1
            if self.index == len(self.pattern):
                self.index = 0
                return True
        else:
            self.index = 0

        return False


class MainWindow(QMainWindow):
    """The main window of the application which includes all relevant panels
    and the main toolbar.
    """
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

        # Connect to some important network signals
        network = get_network()
        network.signalServerConnectionChanged.connect(
            self.onServerConnectionChanged)

        # Listen for application events
        qApp.installEventFilter(self)
        self._net_monitor_matcher = _PatternMatcher('chooch')

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
        """Listen for key press events
        """
        if event.type() == QEvent.KeyPress:
            if self._net_monitor_matcher.check(event.text()):
                get_network().togglePerformanceMonitor()
                self.networkPerfDisplay.setText('')

        return super(MainWindow, self).eventFilter(obj, event)

    def karaboBroadcastEvent(self, event):
        sender = event.sender
        data = event.data
        if sender is KaraboEventSender.ProcessingDelay:
            self.networkPerfDisplay.setText('{:.3f}'.format(data['value']))
            return True  # Nobody else should handle this event!

        if sender is KaraboEventSender.DatabaseIsBusy:
            self._database_is_processing(data.get('is_processing'))
        elif sender is KaraboEventSender.MaximizePanel:
            self._panelContainerMaximized(data.get('container'))
        elif sender is KaraboEventSender.MinimizePanel:
            self._panelContainerMinimized(data.get('container'))
        elif sender is KaraboEventSender.LoginUserChanged:
            self.onUpdateAccessLevel()
        return False

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
        self.acServerConnect.triggered.connect(self.onConnectionButtonPress)

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

        # Add a widget (on the right side) for displaying network performance
        expander = QWidget()
        expander.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        self.networkPerfDisplay = QLabel()
        toolbar.addWidget(expander)
        toolbar.addWidget(self.networkPerfDisplay)

    def _setupStatusBar(self):
        self.statusBar().showMessage('Ready...')

    def _addFixedPanels(self):
        """Add panels which only need to be created once
        """
        # Left
        navigation = NavigationPanel()
        self.addPanel(navigation, PanelAreaEnum.LeftTop)
        self.addPanel(ProjectPanel(), PanelAreaEnum.LeftBottom)

        # Middle
        # XXX: Hide the logging panel with a feature flag
        if 'HIDE_LOGGING_PANEL' not in os.environ:
            self.addPanel(LoggingPanel(), PanelAreaEnum.MiddleBottom)
        self.addPanel(ScriptingPanel(), PanelAreaEnum.MiddleBottom)
        self.addPanel(NotificationPanel(), PanelAreaEnum.MiddleBottom)

        # Right
        configuration = ConfigurationPanel()
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
        # Check for project changes
        if self._should_save_project_before_closing():
            return False

        return True

    def _enable_toolbar(self, enable):
        self.acServerConnect.setEnabled(enable)
        self.tbAccessLevel.setEnabled(enable)

    def _database_is_processing(self, is_processing):
        """This method gets called whenever the database is switching its
        processing mode.

        Scenes, Macros, and Configurations should not be editable while the
        project DB is saving data.
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
                area_container.minimize(True)

    def _panelContainerMinimized(self, panel_container):
        """The given `panel_container` is about to be minimized.
        """
        for area_enum, area_container in self._panel_areas.items():
            if area_container is not panel_container:
                area_container.minimize(False)

    def _unminimize_remaining_panels(self):
        """Reset the maximization of any child panels
        """
        any_minimized = False
        maximized_container = None
        for area_container in self._panel_areas.values():
            if not area_container.minimized:
                maximized_container = area_container
            else:
                any_minimized = True

        if any_minimized:
            assert maximized_container is not None
            if maximized_container.count() > 0:
                maximized_container.currentWidget().onMinimize()

    def _should_save_project_before_closing(self):
        """Asks for discard/save changes on modified project.
        """
        project = get_project_model().traits_data_model
        if project is None:
            return False
        if project.modified or get_db_conn().is_writing():
            ask = ('Unsaved changes on project \"<b>{}</b>\" will be '
                   'permanently lost.<br /> Continue action?'
                   .format(project.simple_name))
            msg_box = QMessageBox(QMessageBox.Question, 'Unsaved project', ask,
                                  QMessageBox.Yes | QMessageBox.Cancel)
            msg_box.setDefaultButton(QMessageBox.Cancel)
            return msg_box.exec() != QMessageBox.Yes
        return False

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

        # Tell the world
        broadcast_event(KaraboEventSender.AccessLevelChanged, {})

    @pyqtSlot(bool)
    def onConnectionButtonPress(self, connect):
        """Slot triggered when the `acServerConnect` button is clicked.
        i.e. The user wishes to (dis)connect the GUI client from/to the server.
        """
        if not connect and self._should_save_project_before_closing():
            # Disconnecting AND need to save first
            self.acServerConnect.setChecked(True)
        else:
            # Either connecting or no need to save before disconnecting
            get_network().onServerConnection(connect)

    @pyqtSlot(bool)
    def onServerConnectionChanged(self, isConnected):
        """Slot triggered when the network connection goes up/down. At this
        point, there is no way to easily undo the state change.

        NOTE: If you need to do something before the state changes, see
        ``self.onConnectionButtonPress()``
        """
        # Un-minimize all panels when disconnecting!
        if not isConnected:
            self._unminimize_remaining_panels()

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
