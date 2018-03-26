#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on February 16, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import deque
from enum import Enum

from PyQt4.QtCore import pyqtSignal
from PyQt4.QtGui import (
    QAction, QFrame, QHBoxLayout, QLabel, QLineEdit, QPushButton, QSizePolicy,
    QVBoxLayout, QWidget)

from karabogui import icons
from karabogui.events import KaraboEventSender, broadcast_event
from karabogui.util import generateObjectName
from karabogui.widgets.toolbar import ToolBar


class Searchable:
    def __init__(self, name):
        super(Searchable, self).__init__(name)

    def create_search_bar(self, treemodel):
        """Returns a QHBoxLayout containing the search bar.
        """
        self.la_search_filter = QLabel("Search for:")
        self.le_search_filter = QLineEdit()
        self.le_search_filter.setToolTip("Find")
        self.le_search_filter.textChanged.connect(self._search_filter_changed)
        self.le_search_filter.returnPressed.connect(self._arrow_right_clicked)
        self.pb_match = QPushButton("Aa")
        self.pb_match.setToolTip("Match case")
        self.pb_match.setCheckable(True)
        self.pb_match.setChecked(False)
        self.pb_match.clicked.connect(self._update_search_filter)
        self.pb_reg_ex = QPushButton(".*")
        self.pb_reg_ex.setToolTip("Use regular expression")
        self.pb_reg_ex.setCheckable(True)
        self.pb_reg_ex.setChecked(False)
        self.pb_reg_ex.clicked.connect(self._update_search_filter)

        frame_layout = QHBoxLayout()
        self.filter_frame = QFrame()
        self.filter_frame.setFrameShape(QFrame.Box)
        self.filter_frame.setStyleSheet("background-color: white;")
        frame_layout.addWidget(self.la_search_filter)
        frame_layout.addWidget(self.le_search_filter)
        frame_layout.addWidget(self.pb_match)
        frame_layout.addWidget(self.pb_reg_ex)

        self.la_result = QLabel("No results")
        self.pb_arrow_left = QPushButton()
        self.pb_arrow_left.setToolTip("Previous match")
        self.pb_arrow_left.setIcon(icons.arrowLeft)
        self.pb_arrow_left.setMaximumHeight(25)
        self.pb_arrow_left.clicked.connect(self._arrow_left_clicked)
        self.pb_arrow_right = QPushButton()
        self.pb_arrow_right.setToolTip("Next match")
        self.pb_arrow_right.setIcon(icons.arrowRight)
        self.pb_arrow_right.setMaximumHeight(25)
        self.pb_arrow_right.clicked.connect(self._arrow_right_clicked)

        h_layout = QHBoxLayout()
        h_layout.addLayout(frame_layout)
        h_layout.addWidget(self.la_result)
        h_layout.addWidget(self.pb_arrow_left)
        h_layout.addWidget(self.pb_arrow_right)

        self.treemodel = treemodel

        return h_layout

    def _init_search_filter(self, connected_to_server=False):
        # A list of nodes found via the search filter
        self.found = []
        # A deque array indicates the current selection in `self.found`
        self.index_array = deque([])
        self.le_search_filter.setText("")
        self.le_search_filter.setEnabled(connected_to_server)
        self.pb_match.setEnabled(connected_to_server)
        self.pb_reg_ex.setEnabled(connected_to_server)
        self.pb_arrow_left.setEnabled(False)
        self.pb_arrow_right.setEnabled(False)

    def _search_filter_changed(self, text):
        """ Slot is called whenever the search filter text was changed
        """
        if text:
            model = self.treemodel
            kwargs = {'case_sensitive': self.pb_match.isChecked(),
                      'use_reg_ex': self.pb_reg_ex.isChecked()}
            self.found = model.findNodes(text, **kwargs)
        else:
            self.found = []

        n_found = len(self.found)
        self.index_array = deque(range(n_found))
        if self.index_array:
            result = "{} Results".format(n_found)
            self._select_node()
        else:
            result = "No Results"
        self.la_result.setText(result)

        enable = n_found > 0
        self.pb_arrow_left.setEnabled(enable)
        self.pb_arrow_right.setEnabled(enable)

    def _update_search_filter(self):
        self._search_filter_changed(self.le_search_filter.text())

    def _arrow_left_clicked(self):
        """rotate index array to the right, so the first index point to the
        previous found node
        """
        self.index_array.rotate(1)
        self._select_node()

    def _arrow_right_clicked(self):
        """rotate index array to the left, so the first index point to the
        next found node
        """
        # Enter key press is linked to arrow right, we have to validate if
        # there are results to show
        if self.index_array:
            self.index_array.rotate(-1)
            self._select_node()

    def _select_node(self):
        """Pick the first number in index array, select the corresponding node
        """
        idx = next(iter(self.index_array))
        self.treemodel.selectNode(self.found[idx])


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

    def __init__(self, title, allow_closing=False):
        super(BasePanelWidget, self).__init__(parent=None)
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

    def undock(self):
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

    def closeEvent(self, event):
        if not self.allow_closing and self.panel_container is not None:
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
        self.maximize()
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
        self.minimize()
        self.panel_container.insert_panels_after_maximize(self.index)

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
