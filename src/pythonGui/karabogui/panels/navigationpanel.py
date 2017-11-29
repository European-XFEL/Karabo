#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 30, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import (QFrame, QHBoxLayout, QLabel, QLineEdit, QPushButton,
                         QWidget, QVBoxLayout)

from karabogui import icons
from karabogui.events import KaraboEventSender, register_for_broadcasts
from karabogui.navigation.view import NavigationTreeView
from .base import BasePanelWidget


class NavigationPanel(BasePanelWidget):
    def __init__(self):
        super(NavigationPanel, self).__init__("Navigation")
        self._init_search_filter()

        # Register to KaraboBroadcastEvent, Note: unregister_from_broadcasts is
        # not necessary for self due to the fact that the singleton mediator
        # object and `self` are being destroyed when the GUI exists
        register_for_broadcasts(self)

    def karaboBroadcastEvent(self, event):
        sender = event.sender
        data = event.data
        if sender is KaraboEventSender.AccessLevelChanged:
            self._init_search_filter(connected_to_server=True)
        elif sender is KaraboEventSender.NetworkConnectStatus:
            self._init_search_filter(data.get('status', False))
        return False

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        widget = QWidget(self)

        main_layout = QVBoxLayout(widget)
        main_layout.setContentsMargins(5, 5, 5, 5)

        self.la_search_filter = QLabel("Search for:")
        self.le_search_filter = QLineEdit()
        self.le_search_filter.setToolTip("Find")
        self.le_search_filter.textChanged.connect(self._search_filter_changed)
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

        self.twNavigation = NavigationTreeView(widget)

        main_layout.addLayout(h_layout)
        main_layout.addWidget(self.twNavigation)

        return widget

    def _init_search_filter(self, connected_to_server=False):
        # A list of nodes found via the search filter
        self.found = []
        # Current select index of `self.found`
        self.current_index = -1
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
            model = self.twNavigation.model()
            kwargs = {'case_sensitive': self.pb_match.isChecked(),
                      'use_reg_ex': self.pb_reg_ex.isChecked()}
            self.found = model.findNodes(text, **kwargs)
        else:
            self.found = []

        if self.found:
            result = "{} Results".format(len(self.found))
            self.current_index = 0
            self._select_node()
        else:
            result = "No Results"
            self.current_index = -1
        self.la_result.setText(result)

        enable = len(self.found) > 1
        self.pb_arrow_left.setEnabled(enable)
        self.pb_arrow_right.setEnabled(enable)

    def _update_search_filter(self):
        self._search_filter_changed(self.le_search_filter.text())

    def _arrow_left_clicked(self):
        first_index = 0
        if self.current_index == first_index:
            self.current_index = len(self.found)-1
        else:
            self.current_index -= 1
        self._select_node()

    def _arrow_right_clicked(self):
        last_index = len(self.found)-1
        if self.current_index == last_index:
            self.current_index = 0
        else:
            self.current_index += 1
        self._select_node()

    def _select_node(self):
        """Select node by given current index
        """
        selected_node = self.found[self.current_index]
        self.twNavigation.model().selectNode(selected_node)
