#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on February 16, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from enum import Enum

from PyQt4.QtCore import pyqtSignal
from PyQt4.QtGui import (QAction, QFrame, QHBoxLayout, QSizePolicy,
                         QVBoxLayout, QWidget)

from karabo_gui.events import KaraboEventSender, broadcast_event
import karabo_gui.icons as icons
from karabo_gui.toolbar import ToolBar
from karabo_gui.util import generateObjectName


class PanelActions(Enum):
    """Actions which can be done to all panels.
    """
    Maximize = 0
    Minimize = 1
    Dock = 2
    Undock = 3


class BasePanelWidget(QFrame):
    """The base class for all panels shown in the GUI. Panels can either be
    independent top-level windows or tabs within the main window of the Karabo
    gui.
    """
    signalPanelClosed = pyqtSignal(str)

    def __init__(self, title):
        super(BasePanelWidget, self).__init__(parent=None)
        self.setWindowTitle(title)
        self.setFrameStyle(QFrame.Box | QFrame.Plain)
        self.setLineWidth(1)

        self.index = -1
        self.doesDockOnClose = True
        self.panel_container = None
        self.is_docked = False
        self.tab_text_color = None

        self._fill_panel()

    # --------------------------------------
    # BasePanelWidget interface

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        raise NotImplementedError

    def dock(self):
        """Called when this panel is docked into the main window.
        """

    def undock(self):
        """Called when this panel is undocked from the main window.
        """

    def update_tab_text_color(self, color=None):
        """Called when this panels tab text color should be changed.
        """
        self.tab_text_color = color
        self.panel_container.update_tab_text_color(self, self.tab_text_color)

    def toolbars(self):
        """This should create and return a list of `ToolBar` instances needed
        by this panel.
        """
        return []

    # --------------------------------------
    # public methods

    def attach_to_container(self, container):
        if container is not None:
            self.is_docked = True
        else:
            self.is_docked = False

        self.panel_container = container
        # Set the toolbar visibility based on whether we're attached or not
        self.standard_toolbar.setVisible(container is not None)
        self.toolbar.setVisible(container is not None)

    def force_close(self):
        """
        This function sets the member variable to False which decides whether
        this widget should be closed on dockonevent and calls closeEvent.
        """
        self.doesDockOnClose = False
        return self.close()

    def set_title(self, name):
        """Set the title of this panel whether its docked or undocked.
        """
        # Set the window title
        self.setWindowTitle(name)
        # And (maybe) the tab label
        if self.is_docked:
            panel = self.panel_container.widget(self.index)
            if panel is self:
                self.panel_container.setTabText(self.index, name)

    # --------------------------------------
    # Qt slots and callbacks

    def closeEvent(self, event):
        if self.doesDockOnClose and self.panel_container is not None:
            self.onDock()
            event.ignore()
        else:
            if self.close():
                event.accept()
            else:
                event.ignore()

    def onUndock(self):
        self._update_toolbar_buttons(PanelActions.Undock)
        self.undock()
        self.panel_container.undock(self)

    def onDock(self):
        self._update_toolbar_buttons(PanelActions.Dock)
        self.dock()
        self.panel_container.dock(self)

    def onMaximize(self):
        self._update_toolbar_buttons(PanelActions.Maximize)
        i = self.panel_container.count()
        while i > -1:
            i -= 1
            w = self.panel_container.widget(i)
            if w != self:
                self.panel_container.removeTab(i)

        broadcast_event(KaraboEventSender.MaximizePanel,
                        {'container': self.panel_container})
        self.panel_container.maximized = True

    def onMinimize(self):
        self._update_toolbar_buttons(PanelActions.Minimize)
        self.panel_container.removeTab(0)
        # Add the tabs back to the container in sorted order
        panels = sorted(self.panel_container.panel_set, key=lambda x: x.index)
        for pan in panels:
            if not pan.is_docked:
                continue
            self.panel_container.insertTab(pan.index, pan, pan.windowTitle())

        self.panel_container.setCurrentIndex(self.index)
        self.panel_container._remove_alarmpanel_close_bt()

        broadcast_event(KaraboEventSender.MinimizePanel,
                        {'container': self.panel_container})
        self.panel_container.maximized = False

    # --------------------------------------
    # private methods

    def _build_standard_toolbar(self):
        """This toolbar is shown by all panels which are attached to a
        container.
        """
        text = "Unpin as individual window"
        self.acUndock = QAction(icons.undock, "&Undock", self)
        self.acUndock.setToolTip(text)
        self.acUndock.setStatusTip(text)
        self.acUndock.triggered.connect(self.onUndock)

        text = "Pin this window to main program"
        self.acDock = QAction(icons.dock, "&Dock", self)
        self.acDock.setToolTip(text)
        self.acDock.setStatusTip(text)
        self.acDock.triggered.connect(self.onDock)
        self.acDock.setVisible(False)

        text = "Maximize panel"
        self.acMaximize = QAction(icons.maximize, "&Maximize", self)
        self.acMaximize.setToolTip(text)
        self.acMaximize.setStatusTip(text)
        self.acMaximize.triggered.connect(self.onMaximize)

        text = "Minimize panel"
        self.acMinimize = QAction(icons.minimize, "&Minimize", self)
        self.acMinimize.setToolTip(text)
        self.acMinimize.setStatusTip(text)
        self.acMinimize.triggered.connect(self.onMinimize)
        self.acMinimize.setVisible(False)

        self.standard_toolbar = ToolBar("Standard", parent=self)
        self.standard_toolbar.add_expander()
        self.standard_toolbar.addAction(self.acUndock)
        self.standard_toolbar.addAction(self.acDock)
        self.standard_toolbar.addAction(self.acMaximize)
        self.standard_toolbar.addAction(self.acMinimize)

        # Hidden by default
        self.standard_toolbar.setVisible(False)

    def _fill_panel(self):
        # Create the content widget first
        main_content = self.get_content_widget()
        # Then the standard toolbar
        self._build_standard_toolbar()

        # Build the toolbar container
        toolbar = QWidget(self)
        toolbar_layout = QHBoxLayout(toolbar)
        all_toolbars = self.toolbars() + [self.standard_toolbar]
        for tb in all_toolbars:
            toolbar_layout.addWidget(tb)
        toolbar_layout.setContentsMargins(0, 0, 0, 0)
        toolbar_layout.setSpacing(0)
        # Make the first toolbars expand to fill all horizontal space
        toolbar_layout.setStretch(toolbar_layout.count()-1, 1)
        toolbar.setVisible(False)

        # Setup some visual characteristics of the toolbar container
        stylesheet = 'QWidget#{} {{background-color: rgb(180,180,180); }}'
        name = generateObjectName(toolbar)
        toolbar.setObjectName(name)
        toolbar.setStyleSheet(stylesheet.format(name))
        toolbar.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)

        # Save a reference so that subclasses can query its properties
        self.toolbar = toolbar

        main_layout = QVBoxLayout(self)
        main_layout.addWidget(self.toolbar)
        main_layout.addWidget(main_content)
        main_layout.setContentsMargins(0, 0, 0, 0)
        main_layout.setSpacing(0)

    def _update_toolbar_buttons(self, action):
        """When a panel is either undocked or maximized, we do not allow any
        action to be performed on it except the one which resets it to its
        default state (either undock or minimize).
        """
        if action is PanelActions.Maximize:
            self.acMaximize.setVisible(False)
            self.acMinimize.setVisible(True)
            self.acDock.setVisible(False)
            self.acUndock.setVisible(False)
        elif action is PanelActions.Minimize:
            self.acMaximize.setVisible(True)
            self.acMinimize.setVisible(False)
            self.acDock.setVisible(False)
            self.acUndock.setVisible(True)
        elif action is PanelActions.Undock:
            self.acDock.setVisible(True)
            self.acUndock.setVisible(False)
            self.acMaximize.setVisible(False)
            self.acMinimize.setVisible(False)
        elif action is PanelActions.Dock:
            self.acDock.setVisible(False)
            self.acUndock.setVisible(True)
            self.acMaximize.setVisible(True)
            self.acMinimize.setVisible(False)
        else:
            raise ValueError('Unrecognized panel action!')
