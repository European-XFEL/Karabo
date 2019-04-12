#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 30, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from collections import deque

from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import (
    QFrame, QHBoxLayout, QLabel, QLineEdit, QPushButton, QVBoxLayout,
    QWidget)
from karabogui import icons
from karabogui.const import SEARCH_BUTTON_WIDTH, SEARCH_LABEL_WIDTH
from karabogui.events import KaraboEvent, register_for_broadcasts
from karabogui.navigation.system_view import SystemTreeView

from .base import BasePanelWidget


class TopologyPanel(BasePanelWidget):
    def __init__(self):
        super(TopologyPanel, self).__init__("System Topology")
        self._reset_search_filter()

        # Register to KaraboBroadcastEvent, Note: unregister_from_broadcasts is
        # not necessary for self due to the fact that the singleton mediator
        # object and `self` are being destroyed when the GUI exists
        event_map = {
            KaraboEvent.AccessLevelChanged: self._event_access_level,
            KaraboEvent.NetworkConnectStatus: self._event_network
        }
        register_for_broadcasts(event_map)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        widget = QWidget(self)
        main_layout = QVBoxLayout(widget)
        main_layout.setContentsMargins(5, 5, 5, 5)

        self.tree_view = SystemTreeView(widget)
        h_layout = self.create_search_bar()
        main_layout.addLayout(h_layout)

        main_layout.addWidget(self.tree_view)
        return widget

    def _event_access_level(self, data):
        self._reset_search_filter(connected_to_server=True)

    def _event_network(self, data):
        status = data.get('status', False)
        self._reset_search_filter(status)
        if not status:
            self.close_popup_widget()

    def create_search_bar(self):
        """Returns a QHBoxLayout containing the search bar.
        """
        self.la_search_filter = QLabel("Search for:")
        self.le_search_filter = QLineEdit()
        self.le_search_filter.setToolTip("Find")
        self.le_search_filter.textChanged.connect(self._search_filter_changed)
        self.le_search_filter.returnPressed.connect(self._arrow_right_clicked)
        self.pb_match = QPushButton("Aa")
        self.pb_match.setFixedWidth(SEARCH_BUTTON_WIDTH)
        self.pb_match.setToolTip("Match case")
        self.pb_match.setCheckable(True)
        self.pb_match.setChecked(False)
        self.pb_match.clicked.connect(self._update_search_filter)
        self.pb_reg_ex = QPushButton(".*")
        self.pb_reg_ex.setFixedWidth(SEARCH_BUTTON_WIDTH)
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
        self.la_result.setFixedWidth(SEARCH_LABEL_WIDTH)
        self.la_result.setAlignment(Qt.AlignCenter)
        self.pb_arrow_left = QPushButton()
        self.pb_arrow_left.setFixedWidth(SEARCH_BUTTON_WIDTH)
        self.pb_arrow_left.setToolTip("Previous match")
        self.pb_arrow_left.setIcon(icons.arrowLeft)
        self.pb_arrow_left.setMaximumHeight(25)
        self.pb_arrow_left.clicked.connect(self._arrow_left_clicked)
        self.pb_arrow_right = QPushButton()
        self.pb_arrow_right.setFixedWidth(SEARCH_BUTTON_WIDTH)
        self.pb_arrow_right.setToolTip("Next match")
        self.pb_arrow_right.setIcon(icons.arrowRight)
        self.pb_arrow_right.setMaximumHeight(25)
        self.pb_arrow_right.clicked.connect(self._arrow_right_clicked)

        h_layout = QHBoxLayout()
        h_layout.addLayout(frame_layout)
        h_layout.addWidget(self.la_result)
        h_layout.addWidget(self.pb_arrow_left)
        h_layout.addWidget(self.pb_arrow_right)

        return h_layout

    def _reset_search_filter(self, connected_to_server=False):
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

    def close_popup_widget(self):
        widget = self.tree_view.popupWidget
        if widget is not None:
            widget.close()

    def _select_node(self):
        """Pick the first number in index array, select the corresponding node
        """
        idx = next(iter(self.index_array))
        self.tree_view.model().selectNode(self.found[idx])

    # -----------------------------------------
    # Qt Slots

    @pyqtSlot(str)
    def _search_filter_changed(self, text):
        """ Slot is called whenever the search filter text was changed
        """
        if text:
            model = self.tree_view.model()
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
            result = "No results"
        self.la_result.setText(result)

        enable = n_found > 0
        self.pb_arrow_left.setEnabled(enable)
        self.pb_arrow_right.setEnabled(enable)

    @pyqtSlot()
    def _update_search_filter(self):
        self._search_filter_changed(self.le_search_filter.text())

    @pyqtSlot()
    def _arrow_left_clicked(self):
        """rotate index array to the right, so the first index point to the
        previous found node
        """
        self.index_array.rotate(1)
        self._select_node()

    @pyqtSlot()
    def _arrow_right_clicked(self):
        """rotate index array to the left, so the first index point to the
        next found node
        """
        # Enter key press is linked to arrow right, we have to validate if
        # there are results to show
        if self.index_array:
            self.index_array.rotate(-1)
            self._select_node()
