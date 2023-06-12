#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 16, 2016
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

from qtpy.QtCore import QModelIndex, Slot
from qtpy.QtWidgets import (
    QAbstractItemView, QButtonGroup, QFrame, QHBoxLayout, QHeaderView, QLabel,
    QLineEdit, QPushButton, QRadioButton, QTableView, QVBoxLayout, QWidget)

from karabogui import icons
from karabogui.alarms.api import (
    ACKNOWLEDGE, ALARM_DATA, ALARM_WARNING_TYPES, INTERLOCK_TYPES,
    AlarmFilterModel)
from karabogui.controllers.table.api import TableButtonDelegate
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.singletons.api import get_alarm_model, get_network

from .base import BasePanelWidget

DEVICE_COLUMN = 2
ACKNOWLEDGE_COLUMN = 6


class AlarmPanel(BasePanelWidget):
    def __init__(self):
        super().__init__("Alarms", allow_closing=True)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        widget = QWidget(parent=self)
        self.ui_filter_group = QButtonGroup(parent=widget)
        self.ui_filter_group.buttonClicked.connect(self.onFilterChanged)
        self.ui_show_alarm_warn = QRadioButton("Show alarms and warnings",
                                               parent=widget)
        self.ui_show_alarm_warn.setIcon(icons.alarmWarning)
        self.ui_show_alarm_warn.setChecked(True)
        self.ui_filter_group.addButton(self.ui_show_alarm_warn)
        self.ui_show_interlock = QRadioButton("Show interlocks", parent=widget)
        self.ui_show_interlock.setIcon(icons.interlock)
        self.ui_filter_group.addButton(self.ui_show_interlock)

        search_layout = QHBoxLayout()
        text = "Device ID"
        label = QLabel()
        label.setText(text)
        label.setStatusTip(text)
        label.setToolTip(text)
        search_layout.addWidget(label)

        text = "Search"
        filter_text = QLineEdit()
        filter_text.setStatusTip(text)
        filter_text.setToolTip(text)
        search_layout.addWidget(filter_text)
        self.filter_text = filter_text

        filter_button = QPushButton("Filter")
        clear_button = QPushButton("Clear Filter")
        search_layout.addWidget(filter_button)
        search_layout.addWidget(clear_button)

        line = QFrame()
        line.setFrameShape(QFrame.HLine)
        line.setFrameShadow(QFrame.Sunken)
        line.setLineWidth(2)

        filter_layout = QHBoxLayout()
        filter_layout.setContentsMargins(0, 0, 0, 0)
        filter_layout.addLayout(search_layout)
        filter_layout.addWidget(line)
        filter_layout.addStretch()
        filter_layout.addWidget(self.ui_show_alarm_warn)
        filter_layout.addWidget(self.ui_show_interlock)

        clear_button.clicked.connect(self.clear_filter)
        filter_button.clicked.connect(self.onFilterChanged)
        filter_text.returnPressed.connect(self.onFilterChanged)

        self.table_view = QTableView(parent=widget)
        self.table_view.setSelectionBehavior(QAbstractItemView.SelectItems
                                             | QAbstractItemView.SelectRows)
        self.table_view.setWordWrap(True)
        self.table_view.setAlternatingRowColors(True)
        self.table_view.doubleClicked.connect(self.onRowDoubleClicked)

        header = self.table_view.horizontalHeader()
        header.setSectionResizeMode(QHeaderView.ResizeToContents)
        header.setStretchLastSection(True)

        self.model = AlarmFilterModel(get_alarm_model(), self.table_view)
        self.table_view.setModel(self.model)
        self.delegate = ButtonDelegate(parent=self.table_view)
        self.table_view.setItemDelegateForColumn(
            ACKNOWLEDGE_COLUMN, self.delegate)

        main_layout = QVBoxLayout(widget)
        main_layout.setContentsMargins(5, 5, 5, 5)
        main_layout.addLayout(filter_layout)
        main_layout.addWidget(self.table_view)

        return widget

    def closeEvent(self, event):
        """Tell main window to enable the button to add me back."""
        super().closeEvent(event)
        if event.isAccepted():
            self.signalPanelClosed.emit(self.windowTitle())

    @Slot()
    def clear_filter(self):
        self.filter_text.setText("")
        self.onFilterChanged()

    @Slot()
    def onFilterChanged(self):
        if self.ui_show_alarm_warn.isChecked():
            filter_types = ALARM_WARNING_TYPES
        elif self.ui_show_interlock.isChecked():
            filter_types = INTERLOCK_TYPES

        pattern = self.filter_text.text()
        self.filter_text.setPlaceholderText(pattern)
        self.model.filter_type = filter_types
        self.model.setFilterFixedString(pattern)

    @Slot(QModelIndex)
    def onRowDoubleClicked(self, index):
        """A double click on a column should select the device in topology"""
        value = index.data()
        if value is None:
            return
        deviceId = self.model.index(index.row(), DEVICE_COLUMN).data()
        broadcast_event(KaraboEvent.ShowDevice, {'deviceId': deviceId})


class ButtonDelegate(TableButtonDelegate):

    def get_button_text(self, index):
        """Reimplemented function of `TableButtonDelegate`"""
        text = ALARM_DATA[ACKNOWLEDGE]
        return text

    def isEnabled(self, index):
        """Reimplemented function of `TableButtonDelegate`"""
        needsAck, ack = index.data()
        return needsAck and ack

    def click_action(self, index):
        """Reimplemented function of `TableButtonDelegate`"""
        if not index.isValid():
            return

        # Send a signal to acknowledge alarm
        model = index.model()
        alarm_id = model.get_alarm_id(index)
        get_network().onAcknowledgeAlarm(model.instanceId, alarm_id)
