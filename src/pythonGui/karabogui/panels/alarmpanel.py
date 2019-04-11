#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 16, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import (
    QButtonGroup, QHBoxLayout, QPixmap, QPushButton, QRadioButton, QStyle,
    QStyledItemDelegate, QTableView, QVBoxLayout, QWidget)
from karabogui import icons
from karabogui.alarms.api import (
    ACKNOWLEDGE, ALARM_DATA, ALARM_ID, ALARM_WARNING_TYPES,
    SHOW_DEVICE, AlarmModel, AlarmFilterModel, INTERLOCK_TYPES,
    get_alarm_key_index)
from karabogui.events import (
    KaraboEvent, broadcast_event, register_for_events,
    unregister_from_events)
from karabogui.singletons.api import get_network

from .base import BasePanelWidget


class AlarmPanel(BasePanelWidget):
    def __init__(self, instanceId):
        self.instanceId = instanceId
        # Important: call the BasePanelWidget initializer
        super(AlarmPanel, self).__init__(instanceId)
        # Register to broadcast events

        self.event_map = {
            KaraboEvent.AlarmServiceInit: self._event_alarm_init,
            KaraboEvent.AlarmServiceUpdate: self._event_alarm_update,
            KaraboEvent.NetworkConnectStatus: self._event_network,
        }
        register_for_events(self.event_map)

    # ----------------------------------------------------------------
    # Karabo Events

    def _event_alarm_init(self, data):
        self.alarm_model.initAlarms(data.get('instance_id'),
                                    data.get('update_types'),
                                    data.get('alarm_entries'))

    def _event_alarm_update(self, data):
        self.alarm_model.updateAlarms(data.get('instance_id'),
                                      data.get('update_types'),
                                      data.get('alarm_entries'))

    def _event_network(self, data):
        if not data['status']:
            # If disconnected to server, unregister the alarm panel from
            # broadcast, otherwise we will get two times of the same info
            # next time
            unregister_from_events(self.event_map)

    def closeEvent(self, event):
        super(AlarmPanel, self).closeEvent(event)
        if event.isAccepted():
            unregister_from_events(self.event_map)

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
        # Add custom filter options

        self.table_view = QTableView(parent=widget)
        self.table_view.setWordWrap(True)
        self.table_view.setAlternatingRowColors(True)
        self.table_view.resizeColumnsToContents()
        self.table_view.horizontalHeader().setStretchLastSection(True)
        self.alarm_model = AlarmModel(self.instanceId, self.table_view)
        self.model = AlarmFilterModel(self.alarm_model, self.table_view)
        btn_delegate = ButtonDelegate(parent=self.table_view)
        self.table_view.setItemDelegate(btn_delegate)

        main_layout = QVBoxLayout(widget)
        main_layout.setContentsMargins(5, 5, 5, 5)
        main_layout.addLayout(filter_layout)
        main_layout.addWidget(self.table_view)

        return widget

    @property
    def model(self):
        return self.table_view.model()

    @model.setter
    def model(self, model):
        self.table_view.setModel(model)

    @pyqtSlot(object)
    def filterToggled(self, button):
        """ The filter ``button`` was activated. Update filtering needed."""
        if button is self.ui_show_alarm_warn:
            self.model.updateFilter(filter_type=ALARM_WARNING_TYPES)
        elif button is self.ui_show_interlock:
            self.model.updateFilter(filter_type=INTERLOCK_TYPES)


class ButtonDelegate(QStyledItemDelegate):
    def __init__(self, parent=None):
        super(ButtonDelegate, self).__init__(parent)
        # Fake button used for later rendering
        self.pbClick = QPushButton("")
        self.pbClick.hide()
        parent.clicked.connect(self.cellClicked)

    def _isRelevantColumn(self, index):
        """ This methods checks whether the column of the given ``index``
            belongs to either the acknowledging or show device column.

            Returns tuple:
            [0] - states whether this is a relevant column
            [1] - the text which needs to be shown on the button
            [2] - states whether this is clickable
            Otherwise ``False``, an empty string and ``False`` is returned.
        """
        column = index.column()
        ack_index = get_alarm_key_index(ACKNOWLEDGE)
        device_index = get_alarm_key_index(SHOW_DEVICE)
        if not (column == ack_index or column == device_index):
            return (False, '', False)

        if column == ack_index:
            text = ALARM_DATA[ACKNOWLEDGE]
            needsAck, ack = index.data()
            clickable = needsAck and ack
        else:
            text = ALARM_DATA[SHOW_DEVICE]
            clickable = True
        return (True, text, clickable)

    def _updateButton(self, button, text, enabled):
        """Update button with given ``text`` and set whether the button is
        ``enabled``.
        """
        button.setText(text)
        button.setEnabled(enabled)

    def createEditor(self, parent, option, index):
        """ This method is called whenever the delegate is in edit mode."""
        isRelevant, text, clickable = self._isRelevantColumn(index)
        if isRelevant:
            # This button is for the highlighting effect when clicking/editing
            button = QPushButton(parent)
            self._updateButton(button, text, clickable)
            return button
        else:
            return super(ButtonDelegate, self).createEditor(parent, option,
                                                            index)

    def setEditorData(self, button, index):
        isRelevant, text, clickable = self._isRelevantColumn(index)
        if isRelevant:
            self._updateButton(button, text, clickable)
        else:
            super(ButtonDelegate, self).setEditorData(button, index)

    def paint(self, painter, option, index):
        isRelevant, text, clickable = self._isRelevantColumn(index)
        if isRelevant:
            self.pbClick.setGeometry(option.rect)
            self._updateButton(self.pbClick, text, clickable)
            if option.state == QStyle.State_Selected:
                painter.fillRect(option.rect, option.palette.highlight())
            pixmap = QPixmap.grabWidget(self.pbClick)
            painter.drawPixmap(option.rect.x(), option.rect.y(), pixmap)
        else:
            super(ButtonDelegate, self).paint(painter, option, index)

    def updateEditorGeometry(self, button, option, index):
        isRelevant, text, clickable = self._isRelevantColumn(index)
        if isRelevant:
            button.setGeometry(option.rect)
            self._updateButton(button, text, clickable)

    @pyqtSlot(object)
    def cellClicked(self, index):
        if not index.isValid():
            return
        isRelevant, text, clickable = self._isRelevantColumn(index)
        if isRelevant and clickable:
            if text == ALARM_DATA[SHOW_DEVICE]:
                # Send signal to show device
                broadcast_event(KaraboEvent.ShowDevice,
                                {'deviceId': index.data()})
            else:
                # Send signal to acknowledge alarm
                id_index = get_alarm_key_index(ALARM_ID)
                model = index.model()
                alarm_id = model.index(index.row(), id_index).data()
                get_network().onAcknowledgeAlarm(model.instanceId, alarm_id)
