#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 16, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from qtpy.QtCore import QModelIndex, Slot
from qtpy.QtWidgets import (
    QAbstractButton, QAbstractItemView, QButtonGroup, QHBoxLayout, QHeaderView,
    QRadioButton, QTableView, QVBoxLayout, QWidget)

from karabogui import icons
from karabogui.alarms.api import (
    ACKNOWLEDGE, ALARM_DATA, ALARM_ID, ALARM_WARNING_TYPES, INTERLOCK_TYPES,
    AlarmFilterModel, get_alarm_key_index)
from karabogui.controllers.table.api import TableButtonDelegate
from karabogui.events import KaraboEvent, broadcast_event
from karabogui.singletons.api import get_alarm_model, get_network

from .base import BasePanelWidget

DEVICE_COLUMN = 3
ACKNOWLEDGE_COLUMN = 7


class AlarmPanel(BasePanelWidget):
    def __init__(self):
        super(AlarmPanel, self).__init__("Alarms", allow_closing=True)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        widget = QWidget(parent=self)
        self.ui_filter_group = QButtonGroup(parent=widget)
        self.ui_filter_group.buttonClicked.connect(self.filterToggled)
        self.ui_show_alarm_warn = QRadioButton("Show alarms and warnings",
                                               parent=widget)
        self.ui_show_alarm_warn.setIcon(icons.alarmWarning)
        self.ui_show_alarm_warn.setChecked(True)
        self.ui_filter_group.addButton(self.ui_show_alarm_warn)
        self.ui_show_interlock = QRadioButton("Show interlocks", parent=widget)
        self.ui_show_interlock.setIcon(icons.interlock)
        self.ui_filter_group.addButton(self.ui_show_interlock)

        filter_layout = QHBoxLayout()
        filter_layout.setContentsMargins(0, 0, 0, 0)
        filter_layout.addWidget(self.ui_show_alarm_warn)
        filter_layout.addWidget(self.ui_show_interlock)
        filter_layout.addStretch()

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

    @Slot(QAbstractButton)
    def filterToggled(self, button):
        """ The filter ``button`` was activated. Update filtering needed."""
        if button is self.ui_show_alarm_warn:
            self.model.updateFilter(filter_type=ALARM_WARNING_TYPES)
        elif button is self.ui_show_interlock:
            self.model.updateFilter(filter_type=INTERLOCK_TYPES)

    def closeEvent(self, event):
        """Tell main window to enable the button to add me back."""
        super(AlarmPanel, self).closeEvent(event)
        if event.isAccepted():
            self.signalPanelClosed.emit(self.windowTitle())

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
        id_index = get_alarm_key_index(ALARM_ID)
        model = index.model()
        alarm_id = model.index(index.row(), id_index).data()
        get_network().onAcknowledgeAlarm(model.instanceId, alarm_id)
