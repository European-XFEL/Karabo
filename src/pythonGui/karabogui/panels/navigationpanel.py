#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 30, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from collections import deque
import os.path as op

from PyQt4 import uic
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QVBoxLayout, QWidget

from karabogui import icons
from karabogui.events import KaraboEvent, register_for_broadcasts
from karabogui.navigation.system_view import SystemTreeView

from .base import BasePanelWidget


class TopologyPanel(BasePanelWidget):
    def __init__(self):
        super(TopologyPanel, self).__init__("System Topology")
        self._reset_filter()

        event_map = {
            KaraboEvent.NetworkConnectStatus: self._event_network,
            KaraboEvent.AccessLevelChanged: self._event_access_level
        }
        register_for_broadcasts(event_map)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel."""
        widget = QWidget(self)
        main_layout = QVBoxLayout(widget)
        main_layout.setContentsMargins(2, 2, 2, 2)

        self.tree_view = SystemTreeView(widget)
        self.sbar = self.create_search_bar()
        self.sbar.label_filter.textChanged.connect(self._filter_changed)
        self.sbar.label_filter.returnPressed.connect(self._arrow_right_clicked)
        self.sbar.arrow_left.setIcon(icons.arrowLeft)
        self.sbar.arrow_left.clicked.connect(self._arrow_left_clicked)
        self.sbar.arrow_right.setIcon(icons.arrowRight)
        self.sbar.arrow_right.clicked.connect(self._arrow_right_clicked)
        self.sbar.case_sensitive.clicked.connect(self._update_filter)
        self.sbar.regexp.clicked.connect(self._update_filter)

        main_layout.addWidget(self.sbar)
        main_layout.addWidget(self.tree_view)

        return widget

    def _event_access_level(self, data):
        self._reset_filter(enable=True)

    def _event_network(self, data):
        status = data.get('status', False)
        self._reset_filter(status)
        if not status:
            self.close_popup_widget()

    def create_search_bar(self):
        """Returns a QHBoxLayout containing the search bar."""
        search_widget = QWidget(parent=self)
        uic.loadUi(op.join(op.dirname(__file__), "search_widget.ui"),
                   search_widget)
        return search_widget

    def close_popup_widget(self):
        widget = self.tree_view.popupWidget
        if widget is not None:
            widget.close()

    def _reset_filter(self, enable=False):
        """The enable is linked to if we are connected to the server"""
        # A list of nodes found via the search filter
        self.found = []
        # A deque array indicates the current selection in `self.found`
        self.index_array = deque([])
        self.sbar.label_filter.setText("")
        self.sbar.label_filter.setEnabled(enable)
        self.sbar.case_sensitive.setEnabled(enable)
        self.sbar.regexp.setEnabled(enable)
        self.sbar.arrow_left.setEnabled(False)
        self.sbar.arrow_right.setEnabled(False)

    def _select_node(self):
        """Pick the first number in index array, select the corresponding node
        """
        idx = next(iter(self.index_array))
        node = self.found[idx]
        parent = node.parent
        # NOTE: The node might have left already!
        if parent is not None and node in parent.children:
            self.tree_view.model().selectNode(node)

    # -----------------------------------------
    # Qt Slots

    @pyqtSlot(str)
    def _filter_changed(self, text):
        """ Slot is called whenever the search filter text was changed"""
        if text:
            model = self.tree_view.model()
            kwargs = {'case_sensitive': self.sbar.case_sensitive.isChecked(),
                      'use_reg_ex': self.sbar.regexp.isChecked()}
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
        self.sbar.result.setText(result)

        enable = n_found > 0
        self.sbar.arrow_left.setEnabled(enable)
        self.sbar.arrow_right.setEnabled(enable)

    @pyqtSlot()
    def _update_filter(self):
        text = self.sbar.label_filter.text()
        self._filter_changed(text)

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
