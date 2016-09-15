#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 16, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtGui import (
    QButtonGroup, QComboBox, QHBoxLayout, QLabel, QPushButton, QTableWidget,
    QVBoxLayout, QWidget)

from karabo_gui.docktabwindow import Dockable
from karabo_gui.mediator import (
    KaraboBroadcastEvent, KaraboEventSender, register_for_broadcasts)
from karabo_gui.network import Network


class AlarmPanel(Dockable, QWidget):
    def __init__(self):
        super(AlarmPanel, self).__init__()

        self.bg_filter = QButtonGroup()
        pb_default_view = QPushButton("Default view")
        pb_default_view.setCheckable(True)
        self.bg_filter.addButton(pb_default_view)
        pb_acknowledge_only = QPushButton("Acknowledge only")
        pb_acknowledge_only.setCheckable(True)
        self.bg_filter.addButton(pb_acknowledge_only)

        la_filter_options = QLabel("Filter options")
        cb_filter_type = QComboBox()
        cb_filter_type.addItem("Device ID")
        cb_filter_type.addItem("Type")
        cb_filter_type.addItem("Class")
        pb_custom_filter = QPushButton("Filter")
        pb_custom_filter.setCheckable(True)
        self.bg_filter.addButton(pb_custom_filter)

        filter_layout = QHBoxLayout()
        filter_layout.setContentsMargins(0, 0, 0, 0)
        filter_layout.addWidget(pb_default_view)
        filter_layout.addWidget(pb_acknowledge_only)
        # Add custom filter options
        filter_layout.addWidget(la_filter_options)
        filter_layout.addWidget(cb_filter_type)
        filter_layout.addWidget(pb_custom_filter)
        filter_layout.addStretch()

        tw_alarm_widget = QTableWidget()
        headers = ["Start time", "Device ID", "Device Type", "Alarm Type",
                   "Message", "Acknowledge", "Show Device"]
        tw_alarm_widget.setColumnCount(len(headers))
        tw_alarm_widget.setHorizontalHeaderLabels(headers)

        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(5, 5, 5, 5)
        main_layout.addLayout(filter_layout)
        main_layout.addWidget(tw_alarm_widget)

        # Register to KaraboBroadcastEvent, Note: unregister_from_broadcasts is
        # not necessary for self due to the fact that the singleton mediator
        # object and `self` are being destroyed when the GUI exists
        register_for_broadcasts(self)

    def eventFilter(self, obj, event):
        if isinstance(event, KaraboBroadcastEvent):
            if event.sender is KaraboEventSender.AlarmInitReply:
                data = event.data
                self._initAlarms(data.get('instanceId'), data.get('rows'))
                return True
            elif event.sender is KaraboEventSender.AlarmUpdate:
                data = event.data
                self._updateAlarms(data.get('instanceId'), data.get('rows'))
                return True
        return super(AlarmPanel, self).eventFilter(obj, event)

    def setupActions(self):
        print("setupActions")

    def setupToolBars(self, toolBar, parent):
        print("setupToolBars")

    def setEnabled(self, enable):
        if enable:
            Network().onRequestAlarms()
        super(AlarmPanel, self).setEnabled(enable)

    def _initAlarms(self, instanceId, rows):
        print("+++_initAlarms", instanceId)
        for k, v in rows.items():
            print()
            print("k", k)
            print("v", v)
            print()
        print()

    def _updateAlarms(self, instanceId, rows):
        print("+++_updateAlarms", instanceId)
        for k, v in rows.items():
            print()
            print("k", k)
            print("v", v)
            print()
        print()
