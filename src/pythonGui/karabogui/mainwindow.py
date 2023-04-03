#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 3, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
import os.path
import webbrowser
from enum import Enum
from functools import partial

from qtpy.QtCore import QPoint, QSize, Qt, Slot
from qtpy.QtGui import QColor
from qtpy.QtWidgets import (
    QAction, QActionGroup, QFrame, QLabel, QMainWindow, QMenu, QMessageBox,
    QSizePolicy, QSplitter, QTextBrowser, QToolButton, qApp)

import karabogui.access as krb_access
from karabo.common.project.api import get_project_models
from karabo.common.project.utils import check_instance_duplicates
from karabo.native import AccessLevel
from karabogui import const, icons, messagebox
from karabogui.access import ACCESS_LEVELS, AccessRole
from karabogui.background import background
from karabogui.dialogs.api import (
    AboutDialog, ClientTopologyDialog, ConfigurationDialog, DataViewDialog,
    UpdateDialog)
from karabogui.events import (
    KaraboEvent, broadcast_event, register_for_broadcasts)
from karabogui.indicators import get_processing_color
from karabogui.logger import StatusLogWidget, get_logger
from karabogui.panels.api import (
    AlarmPanel, ConfigurationPanel, DevicePanel, LoggingPanel, PanelContainer,
    ProjectPanel, ScriptingPanel, TopologyPanel)
from karabogui.programs.register_protocol import register_protocol
from karabogui.singletons.api import (
    get_config, get_db_conn, get_network, get_project_model)
from karabogui.util import generateObjectName, process_qt_events
from karabogui.wizards import TipsTricksWizard

SERVER_INFO_WIDTH = 250

CONSOLE_TITLE = 'Console'
LOG_TITLE = 'Log'
AlARM_TITLE = 'Alarms'

CONFIGURATOR_TITLE = 'Configuration Editor'
SYSTEM_TOPOLOGY_TITLE = 'System Topology'
DEVICE_TOPOLOGY_TITLE = 'Device Topology'
PROJECT_TITLE = 'Projects'

_PANEL_TITLE_CONFIG = {
    CONSOLE_TITLE: 'console_visible',
    LOG_TITLE: 'log_visible',
    AlARM_TITLE: 'alarm_visible'
}

MENU_HEIGHT = 40


class MainWindowBanner(QTextBrowser):
    """The MainWindow Banner"""

    def __init__(self, parent=None):
        super().__init__(parent)
        self.setReadOnly(True)
        self.setFixedHeight(MENU_HEIGHT)
        self.setFrameStyle(QFrame.NoFrame)
        self.setSizePolicy(QSizePolicy.Expanding,
                           QSizePolicy.Fixed)
        self.setAlignment(Qt.AlignCenter)
        self.setTextInteractionFlags(
            Qt.LinksAccessibleByMouse | Qt.TextSelectableByMouse)
        self.setOpenExternalLinks(True)

        self.setObjectName(generateObjectName(self))
        self.setFocusPolicy(Qt.NoFocus)
        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(
            self._show_context_menu)
        self.setLineWidth(1)
        self.set_banner("")

    def set_banner(self, text="", foreground="black",
                   background="transparent"):
        """Set the banner of the main window"""
        if text:
            text = f'<p align="center">{text}</p>'
            style = QFrame.Box
        else:
            background = "transparent"
            style = QFrame.NoFrame

        self.setStyleSheet(f"QAbstractScrollArea#{self.objectName()} "
                           f"{{ color: {foreground};"
                           f"background-color: {background}; }}")
        # text
        self.setFrameStyle(style)
        self.setHtml(text)

    @Slot(QPoint)
    def _show_context_menu(self, pos):
        """Show a context menu"""
        menu = QMenu(self)
        select_action = menu.addAction("Select All")
        select_action.triggered.connect(self.selectAll)
        enable_select = len(self.toPlainText()) > 0
        select_action.setEnabled(enable_select)

        copy_action = menu.addAction("Copy Selected")
        copy_action.triggered.connect(self.copy)
        enable_copy = not self.textCursor().selection().isEmpty()
        copy_action.setEnabled(enable_copy)
        menu.exec(self.viewport().mapToGlobal(pos))


class PanelAreaEnum(Enum):
    """The different panel areas in the main window
    """
    Left = 0
    Middle = 1
    Right = 2


def get_panel_visible(name):
    """Get the associated panel configuration of a closable panel

    This function uses a mapping between `panel title` and `configuration
    name` to retrieve the configuration singleton configuration
    """

    config_name = _PANEL_TITLE_CONFIG[name]
    return get_config()[config_name]


def set_panel_visible(name, visible):
    """Write the associated panel configuration of a closable panel"""
    config_name = _PANEL_TITLE_CONFIG[name]
    get_config()[config_name] = visible


_CLOSABLE_PANELS = {
    # Title: (class, position, icon)
    CONSOLE_TITLE: (ScriptingPanel, PanelAreaEnum.Middle, icons.consoleMenu),
    LOG_TITLE: (LoggingPanel, PanelAreaEnum.Middle, icons.logMenu),
    AlARM_TITLE: (AlarmPanel, PanelAreaEnum.Middle, icons.alarmWarning)
}

_PANELS = {
    # Title: (class, position)
    CONFIGURATOR_TITLE: (ConfigurationPanel, PanelAreaEnum.Right),
    SYSTEM_TOPOLOGY_TITLE: (TopologyPanel, PanelAreaEnum.Left),
    DEVICE_TOPOLOGY_TITLE: (DevicePanel, PanelAreaEnum.Left),
    PROJECT_TITLE: (ProjectPanel, PanelAreaEnum.Left),
}

SETTINGS_TITLE = '&Settings'
PANEL_MENU_TITLE = '&Panels'
GEOMETRY_TITLE = "&Window Geometry"
GRAFANA_LINK = "https://ctrend.xfel.eu/"
RTD_LINK = "https://rtd.xfel.eu/docs"
RTD_GUI_LINK = f"{RTD_LINK}/howtogui/en/latest/"
RTD_FW_LINK = f"{RTD_LINK}/karabo/en/latest/"
RTD_MACRO_LINK = f"{RTD_LINK}/howtomacro/en/latest/"


class MainWindow(QMainWindow):
    """The main window of the application which includes all relevant panels
    and the main toolbar.
    """

    def __init__(self):
        super().__init__()

        # Create projects folder, if not exists
        if not os.path.exists(const.KARABO_PROJECT_FOLDER):
            os.makedirs(const.KARABO_PROJECT_FOLDER, exist_ok=True)

        self._setupActions()
        self._setupMenuBar()
        self._setupToolBar()
        self._setupStatusBar()

        # Keep track of the panels!
        self._active_panels = {}

        # Keep track of the closable panels!
        self._active_closable_panels = {}

        self._panel_areas = {}
        self._setupPanelAreas()
        for name in _PANELS:
            self._open_panel(name)

        # Create the menu bar for panels which are by default closed!
        for name, data in _CLOSABLE_PANELS.items():
            callback = partial(self._open_closable_panel, name)
            action = QAction(name, self)
            _, _, icon = data
            action.setIcon(icon)
            action.triggered.connect(callback)
            self._addPanelMenuAction(action)
            # Set the visibility with the panel configuration!
            visible = get_panel_visible(name)
            self.panelActions[name] = action
            self.panelActions[name].setEnabled(not visible)
            if visible:
                self._open_closable_panel(name)

        self.title_info = {
            "project": None,
            "version": const.GUI_VERSION_LONG,
            "topic": None
        }
        self._set_window_title()
        # Connect to some important network signals
        network = get_network()
        network.signalServerConnectionChanged.connect(
            self.onServerConnectionChanged)
        network.signalNetworkPerformance.connect(
            self.onNetworkPerformance)

        # Register to KaraboBroadcastEvent, Note: unregister_from_broadcasts is
        # not necessary
        event_map = {
            KaraboEvent.ServerInformationUpdate: self._event_server_connection,
            KaraboEvent.BigDataProcessing: self._event_big_data,
            KaraboEvent.DatabaseIsBusy: self._event_db_processing,
            KaraboEvent.MaximizePanel: self._event_container_maximized,
            KaraboEvent.MinimizePanel: self._event_container_minimized,
            KaraboEvent.LoginUserChanged: self._event_access_level,
            KaraboEvent.NetworkConnectStatus: self._event_network,
            KaraboEvent.ProjectUpdated: self._event_project_updated,
            KaraboEvent.ServerNotification: self._event_server_notification,
            KaraboEvent.ProjectName: self._event_project_title,
        }
        register_for_broadcasts(event_map)

        self.readSettings()

    # -----------------------------------------------------------------------
    # Karabo Events

    def _event_server_connection(self, data):
        self.update_server_connection(data)

    def _event_network(self, data):
        status = data.get('status', False)
        if not status:
            active_panels = list(self._active_closable_panels.values())
            for info in active_panels:
                panel, area_enum = info
                self.removePanel(panel, area_enum)

            # On disconnect, we select again the system topology!
            container = self._panel_areas[PanelAreaEnum.Left]
            tab = self._active_panels[SYSTEM_TOPOLOGY_TITLE]
            container.setCurrentIndex(container.indexOf(tab))

            # Set a default title on disconnect
            self._set_window_title(project=None, topic=None)

    def _event_big_data(self, data):
        """Show the big data latency including value set in the gui"""
        name = data['name']
        proc = data['proc']
        self.ui_big_data.setText("{} - {:.3f} s".format(name, proc))

    def _event_db_processing(self, data):
        """This method gets called whenever the database is switching its
        processing mode.

        Scenes, Macros, and Configurations should not be editable while the
        project DB is saving data.
        """
        enable = not data.get('is_processing')
        # Update toolbar
        self._enable_toolbar(enable)

        # Update all open scenes and macros
        container = self._panel_areas[PanelAreaEnum.Middle]
        for panel in container.panel_set:
            panel.setEnabled(enable)

        # Update configuration panels
        container = self._panel_areas[PanelAreaEnum.Right]
        for panel in container.panel_set:
            panel.setEnabled(enable)

    def _event_container_maximized(self, data):
        """The given `panel_container` is about to be maximized.
        """
        panel_container = data['container']
        for area_enum, area_container in self._panel_areas.items():
            if area_container is not panel_container:
                area_container.minimize(True)

    def _event_container_minimized(self, data):
        """The given `panel_container` is about to be minimized.
        """
        panel_container = data['container']
        for area_enum, area_container in self._panel_areas.items():
            if area_container is not panel_container:
                area_container.minimize(False)

    def _event_server_notification(self, data):
        text = data.get('message', '')
        foreground = QColor(data.get('foreground', 'black')).name()
        background = QColor(data.get('background', 'white')).name()
        self.notification_banner.set_banner(text, foreground, background)

    def _event_access_level(self, data):
        self.onUpdateAccessLevel()

    def _event_project_updated(self, data):
        """Projects were externally updated and we check if we are affected"""
        project = get_project_model().root_model
        if project is None:
            return

        projects = get_project_models(project)
        external_uuids = data['uuids']
        conflict = [item for item in projects if item.uuid in external_uuids]
        for model in conflict:
            model.conflict = True
            get_logger().warning(f"The project {model.simple_name} was "
                                 f"updated externally. Please reload the "
                                 f"project to synchronize the changes.")

    # -----------------------------------------------------------------------

    def _event_project_title(self, data):
        name = data["simple_name"]
        self._set_window_title(project=name)

    def _set_window_title(self, **kwargs):
        self.title_info.update(kwargs)
        titles = ["Project: {}", "European XFEL - Karabo GUI {}", "Topic: {}"]
        title = " - ".join([info.format(value) for info, value in
                            zip(titles, self.title_info.values())
                            if value is not None])
        if get_network().username and get_network().one_time_token:
            title = f"{title} - User: {get_network().username}"
        self.setWindowTitle(title)

    def readSettings(self):
        window_geometry = get_config()["main_geometry"]
        if window_geometry:
            self.restoreGeometry(window_geometry)
        else:
            self.adjustSize()

    def update_server_connection(self, data=None):
        """Update the status bar with our broker connection information
        """
        if data is not None:
            topic = data["topic"]
            # Store this information in the config singleton!
            get_config()["broker_topic"] = topic
            self._set_window_title(topic=topic)

            hostname = data["hostname"]
            hostport = data["hostport"]
            info = f"<b>{hostname}:{hostport} ({topic})</b>"
            self.serverInfo.setText(info)

            # Place some user log info messages
            logger = get_logger()
            text = f"Successfully connected to gui server (topic): {info}"
            logger.info(text)
            # If the server is read only, we notify as well
            read_only = data.get("readOnly", False)
            if read_only:
                text = "The gui server device is configured as <b>readOnly</b>"
                logger.info(text)
        else:
            self.serverInfo.setText("")

    # --------------------------------------
    # Qt virtual methods

    def sizeHint(self):
        return QSize(1200, 800)

    def closeEvent(self, event):
        if not self._quit():
            event.ignore()
            return
        self._quit_console()

        event.accept()
        QMainWindow.closeEvent(self, event)
        # Process eventual events to gracefully close main window
        process_qt_events(timeout=1000)
        qApp.quit()

    # --------------------------------------
    # public methods

    def addPanel(self, panel, area_enum):
        """Add a panel to an area's container in this window
        """
        container = self._panel_areas[area_enum]
        container.addPanel(panel)

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

    def dockPanel(self, name):
        """Public method to ensure that a panel by `name` is docked"""
        if name in self._active_panels:
            panel = self._active_panels[name]
            if not panel.is_docked:
                panel.onDock()

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

        # Store actions to reopen panels
        self.panelActions = {}

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
        self.acExit.triggered.connect(self.close)

        self.acHelpAbout = QAction("About", self)
        self.acHelpAbout.triggered.connect(self.onHelpAbout)

        self.acClientTopology = QAction("Client Topology", self)
        self.acClientTopology.triggered.connect(self.onClientTopology)

        self.acConfig = QAction("Application Configuration", self)
        self.acConfig.triggered.connect(self.onConfiguration)

        self.acWizard = QAction("Tips'N'Tricks", self)
        self.acWizard.triggered.connect(self.onWizard)

        self.acHelpAboutQt = QAction("About Qt", self)
        self.acHelpAboutQt.triggered.connect(qApp.aboutQt)

        self.acCheckUpdates = QAction("Check for Updates", self)
        self.acCheckUpdates.triggered.connect(self.onCheckUpdates)

        self.acCheckProject = QAction("Check for Project Duplicates", self)
        self.acCheckProject.triggered.connect(self.onInvestigateProject)

        self.acGrafana = QAction(icons.grafana, "Grafana", self)
        self.acGrafana.triggered.connect(self.onGrafana)
        self.acGrafana.setCheckable(False)

        self.acGuiDocumentation = QAction(
            icons.weblink, "GUI Documentation", self)
        self.acGuiDocumentation.triggered.connect(self.onGuiDocumentation)
        self.acGuiDocumentation.setCheckable(False)

        self.acKaraboDocumentation = QAction(
            icons.logo, "Karabo Documentation", self)
        self.acKaraboDocumentation.triggered.connect(
            self.onKaraboDocumentation)
        self.acKaraboDocumentation.setCheckable(False)

        self.acMacroDocumentation = QAction(
            icons.logo, "HowToMacro Documentation", self)
        self.acMacroDocumentation.triggered.connect(
            self.onMacroDocumentation)
        self.acMacroDocumentation.setCheckable(False)

    def _setupMenuBar(self):
        menuBar = self.menuBar()

        mFileMenu = menuBar.addMenu("&File")
        mFileMenu.addAction(self.acExit)

        # Display actions to reopen panels
        mPanelMenu = menuBar.addMenu(PANEL_MENU_TITLE)
        # reference to view menu and its submenus {menuName: QMenu}
        self.viewMenus = {PANEL_MENU_TITLE: mPanelMenu}

        # As basic action, no extra sub menu is required and we directly insert
        panelAction = QAction(icons.save, 'Save panel configuration', self)
        panelAction.triggered.connect(self._store_panel_configuration)
        mPanelMenu.addAction(panelAction)
        mPanelMenu.addSeparator()

        mSettingsMenu = menuBar.addMenu(SETTINGS_TITLE)
        self.settingsMenus = {SETTINGS_TITLE: mSettingsMenu}

        self.acEnableHighDPI = QAction('Enable HighDPI', self)
        self.acEnableHighDPI.setCheckable(True)
        enable = get_config()["highDPI"]
        self.acEnableHighDPI.setChecked(enable)
        self.acEnableHighDPI.triggered.connect(self._store_dpi_setting)
        mSettingsMenu.addAction(self.acEnableHighDPI)

        mGeometryMenu = mSettingsMenu.addMenu(GEOMETRY_TITLE)
        self.acStoreMainWindowGeometry = QAction('Store', self)
        self.acStoreMainWindowGeometry.triggered.connect(
            self.onStoreMainWindowGeometry)
        mGeometryMenu.addAction(self.acStoreMainWindowGeometry)

        self.acEraseMainWindowGeometry = QAction('Erase', self)
        self.acEraseMainWindowGeometry.triggered.connect(
            self.onEraseMainWindowGeometry)
        mGeometryMenu.addAction(self.acEraseMainWindowGeometry)

        self.acRegisterApplication = QAction('Register Application', self)
        self.acRegisterApplication.triggered.connect(
            self.onRegisterApplication)
        if not const.IS_MAC_SYSTEM:
            mSettingsMenu.addAction(self.acRegisterApplication)

        mHelpMenu = menuBar.addMenu("&Links")
        mHelpMenu.addAction(self.acGrafana)
        mHelpMenu.addAction(self.acGuiDocumentation)
        mHelpMenu.addAction(self.acMacroDocumentation)
        mHelpMenu.addAction(self.acKaraboDocumentation)

        mViewMenu = menuBar.addMenu("&View")
        mViewMenu.addAction(self.acConfig)
        mViewMenu.addAction(self.acClientTopology)

        mHelpMenu = menuBar.addMenu("&Help")
        mHelpMenu.addAction(self.acHelpAbout)
        mHelpMenu.addAction(self.acHelpAboutQt)
        mHelpMenu.addAction(self.acWizard)
        mHelpMenu.addAction(self.acCheckUpdates)
        mHelpMenu.addAction(self.acCheckProject)

    def _setupToolBar(self):

        self.toolbar = toolbar = self.addToolBar("Standard")
        toolbar.setObjectName(generateObjectName(toolbar))
        toolbar.addAction(self.acExit)

        toolbar.addSeparator()
        toolbar.addAction(self.acServerConnect)

        toolbar.addWidget(self.tbAccessLevel)

        self.notification_banner = MainWindowBanner()

        # Add a widget (on the right side) for displaying network performance
        self.ui_big_data = QLabel()
        self.ui_big_data.setMaximumWidth(600)
        self.ui_big_data.setAlignment(Qt.AlignRight)
        self.ui_lamp = QLabel()
        self.ui_lamp.setFixedWidth(60)
        self.ui_lamp.setAlignment(Qt.AlignCenter)
        self.ui_lamp.setFrameStyle(QFrame.Box)

        toolbar.addWidget(self.notification_banner)
        toolbar.addWidget(self.ui_big_data)
        toolbar.addWidget(self.ui_lamp)

    def _setupStatusBar(self):
        self.log_widget = StatusLogWidget(self)
        status_bar = self.statusBar()
        status_bar.addPermanentWidget(self.log_widget, 1)

        # More information ...
        self.serverInfo = QLabel(self)
        self.serverInfo.setMinimumWidth(SERVER_INFO_WIDTH)
        self.serverInfo.setAlignment(Qt.AlignCenter)
        status_bar.addPermanentWidget(self.serverInfo)

        get_logger().info("Started Karabo GUI application ...")

    def _setupPanelAreas(self):
        """Build the main splitter structure of the main window
        """
        # Create a main splitter with left, middle, and right areas
        mainSplitter = QSplitter(Qt.Horizontal)
        mainSplitter.setContentsMargins(5, 5, 5, 5)
        left_area = QSplitter(Qt.Vertical, mainSplitter)
        middle_area = QSplitter(Qt.Vertical, mainSplitter)
        right_area = QSplitter(Qt.Vertical, mainSplitter)
        mainSplitter.setStretchFactor(0, 1)
        mainSplitter.setStretchFactor(1, 7)
        mainSplitter.setStretchFactor(2, 2)
        self.setCentralWidget(mainSplitter)

        # Set up the left area
        left = PanelContainer("Navigation", left_area)
        self._panel_areas[PanelAreaEnum.Left] = left

        # Set up the middle area
        middle = PanelContainer("Custom view", middle_area, handle_empty=True)
        self._panel_areas[PanelAreaEnum.Middle] = middle

        # Set up the right area
        right = PanelContainer("Configuration", right_area)
        self._panel_areas[PanelAreaEnum.Right] = right

    def _addPanelMenuAction(self, action, name=PANEL_MENU_TITLE):
        """Add a QAction to the view menu, If name is not PANEL_MENU_TITLE,
        put the action in a sub menu.

        :param action: a QAction to a panel
        :param name: name of the submenu or PANEL_MENU_TITLE
        """
        viewMenus = self.viewMenus
        if name not in viewMenus:
            viewMenus[name] = viewMenus[PANEL_MENU_TITLE].addMenu(name)
            submenu = viewMenus[name]
        else:
            submenu = viewMenus[name]
        submenu.addAction(action)
        for submenu in self.viewMenus.values():
            submenu.setEnabled(not submenu.isEmpty())

    def _quit(self):
        """ Check for project changes"""
        if self._should_save_project_before_closing():
            return False

        return True

    def _quit_console(self):
        """Make sure that we close the console when we exit the GUI! """
        info = self._active_closable_panels.get(CONSOLE_TITLE, None)
        if info is not None:
            panel, area_enum = info
            self.removePanel(panel, area_enum)
            process_qt_events(timeout=5000)

    def _enable_toolbar(self, enable):
        self.acServerConnect.setEnabled(enable)
        self.tbAccessLevel.setEnabled(enable)

    def _open_panel(self, name):
        panel_info = _PANELS.get(name)
        if panel_info is None:
            return

        klass, area_enum = panel_info
        panel = klass()
        self._active_panels[name] = panel
        self.addPanel(panel, area_enum)

    def _open_closable_panel(self, name):
        panel_info = _CLOSABLE_PANELS.get(name)
        if panel_info is None:
            return

        klass, area_enum, _ = panel_info
        panel = klass()

        # We must have a closable panel!
        assert panel.allow_closing

        self.addPanel(panel, area_enum)
        self.onPanelOpen(name)
        panel.signalPanelClosed.connect(self.onPanelClose)
        action = self.panelActions.get(name)
        if action is not None:
            action.setEnabled(False)

        self._active_closable_panels[name] = (panel, area_enum)

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
        project = get_project_model().root_model
        if project is None:
            return False
        allowed = krb_access.access_role_allowed(AccessRole.PROJECT_EDIT)
        if get_db_conn().is_writing() or (project.modified and allowed):
            ask = ('Unsaved changes on project \"<b>{}</b>\" will be '
                   'permanently lost.<br /> Continue action?'
                   .format(project.simple_name))
            msg_box = QMessageBox(QMessageBox.Question, 'Unsaved project',
                                  ask, QMessageBox.Yes | QMessageBox.Cancel)
            msg_box.setDefaultButton(QMessageBox.Cancel)
            return msg_box.exec() != QMessageBox.Yes
        return False

    def onPanelOpen(self, name):
        if name == LOG_TITLE:
            get_network().onSubscribeLogs(True)

    def _open_link(self, link):
        try:
            webbrowser.open_new(link)
        except webbrowser.Error:
            messagebox.show_error("No web browser available!", parent=self)

    # --------------------------------------
    # Qt slots

    @Slot()
    def _store_dpi_setting(self):
        enabled = get_config()['highDPI']
        get_config()['highDPI'] = not enabled
        self.acEnableHighDPI.setChecked(not enabled)
        text = ("Changing the high dpi settings requires a restart of the "
                "client application of the setting to become active.")
        messagebox.show_information(text)

    @Slot()
    def onRegisterApplication(self):
        register_protocol()
        get_logger().info("Registered the Karabo client application with "
                          f"version <b>{const.GUI_VERSION_LONG}</b> "
                          "for the <b>URL scheme</b> protocol.")

    @Slot()
    def _store_panel_configuration(self):
        for name in _CLOSABLE_PANELS:
            visible = name in self._active_closable_panels
            set_panel_visible(name, visible=visible)

    @Slot()
    def onConfiguration(self):
        ConfigurationDialog(parent=self).open()

    @Slot()
    def onStoreMainWindowGeometry(self):
        get_config()['main_geometry'] = self.saveGeometry()

    @Slot()
    def onEraseMainWindowGeometry(self):
        del get_config()['main_geometry']

    @Slot()
    def onClientTopology(self):
        ClientTopologyDialog(parent=self).open()

    @Slot()
    def onHelpAbout(self):
        AboutDialog(parent=self).open()

    @Slot()
    def onCheckUpdates(self):
        dialog = UpdateDialog(parent=self)
        dialog.open()

    @Slot()
    def onInvestigateProject(self):
        root = get_project_model().root_model
        if root is None:
            return
        if get_db_conn().is_reading():
            messagebox.show_information(
                "Please wait until project loading has finished.", parent=self)
            return

        @Slot(object)
        def handler(future):
            data = future.result()
            text = (
                "The view shows the number of instances for both servers and "
                "devices and their occurrence as duplicates.\n"
                "Duplicates can lead to configuration conflicts.")
            dialog = DataViewDialog(
                "Project Instance Conflicts", text, data, parent=self)
            dialog.show()

        background(check_instance_duplicates, root, callback=handler)

    @Slot()
    def onGrafana(self):
        self._open_link(GRAFANA_LINK)

    @Slot()
    def onGuiDocumentation(self):
        self._open_link(RTD_GUI_LINK)

    @Slot()
    def onKaraboDocumentation(self):
        self._open_link(RTD_FW_LINK)

    @Slot()
    def onMacroDocumentation(self):
        self._open_link(RTD_MACRO_LINK)

    @Slot()
    def onWizard(self):
        TipsTricksWizard(parent=self).open()

    @Slot(QAction)
    def onChangeAccessLevel(self, action):
        level = action.data()
        assert isinstance(level, AccessLevel), 'Garbage access level value!'
        krb_access.GLOBAL_ACCESS_LEVEL = level

        # Tell the world
        broadcast_event(KaraboEvent.AccessLevelChanged, {})

    @Slot(bool)
    def onConnectionButtonPress(self, connect):
        """Slot triggered when the `acServerConnect` button is clicked.
        i.e. The user wishes to (dis)connect the GUI client from/to the server.
        """
        if not connect and self._should_save_project_before_closing():
            # Disconnecting AND need to save first
            self.acServerConnect.setChecked(True)
        else:
            self.update_server_connection()
            # Either connecting or no need to save before disconnecting
            get_network().onServerConnection(connect, parent=self)

    @Slot(str)
    def onPanelClose(self, name):
        action = self.panelActions.get(name)
        if action is not None:
            action.setEnabled(True)
        if name == LOG_TITLE:
            get_network().onSubscribeLogs(False)

        self._active_closable_panels.pop(name)

    @Slot(float, bool)
    def onNetworkPerformance(self, proc_delay, active):
        """Color our network lamp with respect to the processing delay
        """
        color = get_processing_color(proc_delay=proc_delay)
        self.ui_lamp.setStyleSheet("background-color: rgba{}".format(color))

        # The CHOOCH from the QDialog will still set the diff!
        if active:
            self.ui_lamp.setText('{:.3f}'.format(proc_delay))

    @Slot(bool)
    def onServerConnectionChanged(self, isConnected):
        """Slot triggered when the network connection goes up/down. At this
        point, there is no way to easily undo the state change.

        NOTE: If you need to do something before the state changes, see
        ``self.onConnectionButtonPress()``
        """
        # Un-minimize all panels when disconnecting!
        if not isConnected:
            self._unminimize_remaining_panels()
            # Erase lamp information
            self.ui_lamp.setStyleSheet("")
            self.ui_lamp.clear()
            self.ui_big_data.setText("")
            self.notification_banner.set_banner("")

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

    @Slot()
    def onUpdateAccessLevel(self):
        global_access_level = krb_access.GLOBAL_ACCESS_LEVEL

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
