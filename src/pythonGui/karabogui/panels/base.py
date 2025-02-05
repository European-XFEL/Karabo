#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on February 16, 2017
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################
from enum import Enum

from qtpy.QtCore import Signal, Slot
from qtpy.QtWidgets import (
    QAction, QFrame, QHBoxLayout, QSizePolicy, QVBoxLayout, QWidget)

from karabogui import icons
from karabogui.dialogs.logbook_preview import LogBookPreview
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.util import generateObjectName
from karabogui.widgets.toolbar import ToolBar


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
    signalPanelClosed = Signal(str)

    def __init__(self, title, allow_closing=False):
        super().__init__(parent=None)
        self.setWindowTitle(title)
        self.setFrameStyle(QFrame.Box | QFrame.Plain)
        self.setLineWidth(1)

        self.index = -1
        self.allow_closing = allow_closing
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
        """Called before this panel is docked into the main window.
        """

    def undock(self, maximized=False):
        """Called before this panel is undocked from the main window.
        """

    def maximize(self):
        """Called before this panel is maximized to fill the main window.
        """

    def minimize(self):
        """Called before this panel is minimized after filling the main window.
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

    def info(self):
        """Retrieve the info for the KaraboLogBook"""

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

    @Slot()
    def handleLogBookPreview(self):
        dialog = LogBookPreview(parent=self)
        dialog.show()

    def closeEvent(self, event):
        if not self.allow_closing and self.panel_container is not None:
            self.onDock()
            event.ignore()
        else:
            if self.close():
                event.accept()
                if self.panel_container is not None and not self.is_docked:
                    # We are undocked and notify!
                    self.panel_container.removeUndockedPanel(self)
            else:
                event.ignore()

    def onUndock(self):
        self._update_toolbar_buttons(PanelActions.Undock)
        is_maximized = self.panel_container.maximized
        self.undock(is_maximized)
        self.panel_container.undock(self)

    def onDock(self):
        self._update_toolbar_buttons(PanelActions.Dock)
        self.dock()
        self.panel_container.dock(self)

    def onMaximize(self):
        self._update_toolbar_buttons(PanelActions.Maximize)
        self.maximize()
        i = self.panel_container.count()
        while i > -1:
            i -= 1
            w = self.panel_container.widget(i)
            if w != self:
                self.panel_container.removeTab(i)

        broadcast_event(KaraboEvent.MaximizePanel,
                        {'container': self.panel_container})
        self.panel_container.maximized = True

    def onMinimize(self):
        self._update_toolbar_buttons(PanelActions.Minimize)
        self.minimize()
        self.panel_container.insert_panels_after_maximize(self.index)

        broadcast_event(KaraboEvent.MinimizePanel,
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
        self.standard_toolbar.addAction(self.acUndock)
        self.standard_toolbar.addAction(self.acDock)
        self.standard_toolbar.addAction(self.acMaximize)
        self.standard_toolbar.addAction(self.acMinimize)

        # Hidden by default
        self.standard_toolbar.setVisible(False)

    def _build_application_toolbar(self):
        # Creating a separate toolbar that appears in all modes, like
        # Cinema, Concert etc.
        text = "Save LogBook"
        self.acSaveLogBook = QAction(icons.logbook, "&Save LogBook", self)
        self.acSaveLogBook.setToolTip(text)
        self.acSaveLogBook.setStatusTip(text)
        self.acSaveLogBook.triggered.connect(self.handleLogBookPreview)

        self.application_toolbar = ToolBar("Logbook", parent=self)
        self.application_toolbar.add_expander()
        self.application_toolbar.addAction(self.acSaveLogBook)

    def _fill_panel(self):
        # Create the content widget first
        main_content = self.get_content_widget()
        # Then the standard toolbar
        self._build_standard_toolbar()
        self._build_application_toolbar()

        # Build the toolbar container
        toolbar = QWidget(self)
        toolbar_layout = QHBoxLayout(toolbar)
        all_toolbars = self.toolbars() + [self.application_toolbar,
                                          self.standard_toolbar]
        for tb in all_toolbars:
            toolbar_layout.addWidget(tb)
        toolbar_layout.setContentsMargins(0, 0, 0, 0)
        toolbar_layout.setSpacing(0)
        # Make the first toolbars expand to fill all horizontal space
        toolbar_layout.setStretch(toolbar_layout.count() - 2, 1)
        # Default visibility is set to True and updated on container setting
        toolbar.setVisible(True)

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
