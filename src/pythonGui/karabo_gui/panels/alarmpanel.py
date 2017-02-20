#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 16, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import (QButtonGroup, QComboBox, QHBoxLayout, QLabel,
                         QLineEdit, QPixmap, QPushButton, QStyle,
                         QStyledItemDelegate, QTableView, QVBoxLayout, QWidget)

from karabo_gui.alarm_model import (ACKNOWLEDGE, ALARM_DATA, ALARM_ID,
                                    ALARM_TYPE, DEVICE_ID, PROPERTY,
                                    SHOW_DEVICE, AlarmModel, getAlarmKeyIndex)
from karabo_gui.events import (
    KaraboBroadcastEvent, KaraboEventSender, broadcast_event,
    register_for_broadcasts, unregister_from_broadcasts)
from karabo_gui.singletons.api import get_network
from .base import BasePanelWidget


class AlarmPanel(BasePanelWidget):
    def __init__(self, instanceId, title):
        self.instanceId = instanceId

        # Important: call the BasePanelWidget initializer
        super(AlarmPanel, self).__init__(title)

        # Register for KaraboBroadcastEvent
        # NOTE: unregister_from_broadcasts will be called by closeEvent()
        register_for_broadcasts(self)

    def eventFilter(self, obj, event):
        if isinstance(event, KaraboBroadcastEvent):
            if event.sender is KaraboEventSender.AlarmInitReply:
                data = event.data
                self._initAlarms(data.get('instanceId'), data.get('rows'))
                return False
            elif event.sender is KaraboEventSender.AlarmUpdate:
                data = event.data
                self._updateAlarms(data.get('instanceId'), data.get('rows'))
                return False
        return super(AlarmPanel, self).eventFilter(obj, event)

    def closeEvent(self, event):
        unregister_from_broadcasts(self)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        widget = QWidget(self)
        self.bgFilter = QButtonGroup(parent=widget)
        self.bgFilter.buttonClicked.connect(self.filterToggled)
        self.pbDefaultView = QPushButton("Default view", parent=widget)
        self.pbDefaultView.setCheckable(True)
        self.pbDefaultView.setChecked(True)
        self.bgFilter.addButton(self.pbDefaultView)
        self.pbAcknowledgeOnly = QPushButton("Acknowledge only", parent=widget)
        self.pbAcknowledgeOnly.setCheckable(True)
        self.bgFilter.addButton(self.pbAcknowledgeOnly)

        self.laFilterOptions = QLabel("Filter options", parent=widget)
        self.cbFilterType = QComboBox(parent=widget)
        self.cbFilterType.addItem(ALARM_DATA[DEVICE_ID], DEVICE_ID)
        self.cbFilterType.addItem(ALARM_DATA[PROPERTY], PROPERTY)
        self.cbFilterType.addItem(ALARM_DATA[ALARM_TYPE], ALARM_TYPE)
        self.leFilterText = QLineEdit(parent=widget)
        self.pbCustomFilter = QPushButton("Filter", parent=widget)
        self.pbCustomFilter.setCheckable(True)
        self.bgFilter.addButton(self.pbCustomFilter)
        self._enableCustomFilter(False)
        self.cbFilterType.currentIndexChanged.connect(
            partial(self.filterToggled, self.pbCustomFilter))
        self.leFilterText.textChanged.connect(
            partial(self.filterToggled, self.pbCustomFilter))

        filterLayout = QHBoxLayout()
        filterLayout.setContentsMargins(0, 0, 0, 0)
        filterLayout.addWidget(self.pbDefaultView)
        filterLayout.addWidget(self.pbAcknowledgeOnly)
        # Add custom filter options
        filterLayout.addWidget(self.laFilterOptions)
        filterLayout.addWidget(self.cbFilterType)
        filterLayout.addWidget(self.leFilterText)
        filterLayout.addWidget(self.pbCustomFilter)
        filterLayout.addStretch()

        self.twAlarm = QTableView(parent=widget)
        self.twAlarm.setWordWrap(True)
        self.twAlarm.setAlternatingRowColors(True)
        self.twAlarm.resizeColumnsToContents()
        self.twAlarm.horizontalHeader().setStretchLastSection(True)
        alarm_model = AlarmModel(self.instanceId, self.twAlarm)
        self.twAlarm.setModel(alarm_model)
        btn_delegate = ButtonDelegate(parent=self.twAlarm)
        self.twAlarm.setItemDelegate(btn_delegate)

        main_layout = QVBoxLayout(widget)
        main_layout.setContentsMargins(5, 5, 5, 5)
        main_layout.addLayout(filterLayout)
        main_layout.addWidget(self.twAlarm)

        return widget

    def _initAlarms(self, instanceId, rows):
        self.alarm_model.initAlarms(instanceId, rows)

    def _updateAlarms(self, instanceId, rows):
        self.alarm_model.updateAlarms(instanceId, rows)

    def _enableCustomFilter(self, enable):
        self.cbFilterType.setEnabled(enable)
        self.leFilterText.setEnabled(enable)

    @property
    def alarm_model(self):
        return self.twAlarm.model()

    @pyqtSlot(object)
    def filterToggled(self, button):
        """ The filter ``button`` was activated. Update filtering needed."""
        if button is self.pbDefaultView:
            self._enableCustomFilter(False)
            self.alarm_model.updateFilter(filterType=None)
        elif button is self.pbAcknowledgeOnly:
            self._enableCustomFilter(False)
            self.alarm_model.updateFilter(filterType=ACKNOWLEDGE)
        elif button is self.pbCustomFilter:
            self._enableCustomFilter(True)
            data = self.cbFilterType.itemData(self.cbFilterType.currentIndex())
            filterText = self.leFilterText.text()
            self.alarm_model.updateFilter(filterType=data, text=filterText)


class ButtonDelegate(QStyledItemDelegate):
    def __init__(self, parent=None):
        super(ButtonDelegate, self).__init__(parent)
        # Fake button used for later rendering
        self.pbClick = QPushButton("", parent)
        self.pbClick.hide()
        parent.clicked.connect(self.cellClicked)
        self.cellEditMode = False
        self.currentCellIndex = None  # QPersistentModelIndex

    def _isRelevantColumn(self, index):
        """ This methods checks whether the column of the given ``index``
            belongs to either the acknowledging or show device column.

            Returns tuple:
            [0] - states whether this is a relevant column
            [1] - the text which needs to be shown on the button
            Otherwise ``False`` and an empty string is returned.
        """
        column = index.column()
        ack_index = getAlarmKeyIndex(ACKNOWLEDGE)
        device_index = getAlarmKeyIndex(SHOW_DEVICE)
        if column == ack_index or column == device_index:
            if column == ack_index:
                text = ALARM_DATA[ACKNOWLEDGE]
            else:
                text = ALARM_DATA[SHOW_DEVICE]
            return (True, text)
        return (False, '')

    def _updateButton(self, button, index, text):
        """ Set the enabling of the button depending on the
            properties ``NEEDS_ACKNOWLEDGING`` and ``ACKNOWLEDGEABLE``.
        """
        button.setText(text)
        column = index.column()
        if column == getAlarmKeyIndex(ACKNOWLEDGE):
            needsAck, ack = index.data()
            button.setEnabled(needsAck and ack)
        elif column == getAlarmKeyIndex(SHOW_DEVICE):
            button.setEnabled(True)

    def createEditor(self, parent, option, index):
        """ This method is called whenever the delegate is in edit mode."""
        isRelevant, text = self._isRelevantColumn(index)
        if isRelevant:
            # This button is for the highlighting effect when clicking/editing
            # the index, is deleted whenever `closePersistentEditor` is called
            button = QPushButton(parent)
            self._updateButton(button, index, text)
            return button
        else:
            return super(ButtonDelegate, self).createEditor(parent, option, index)

    def setEditorData(self, button, index):
        isRelevant, text = self._isRelevantColumn(index)
        if isRelevant:
            self._updateButton(button, index, text)
        else:
            super(ButtonDelegate, self).setEditorData(button, index)

    def paint(self, painter, option, index):
        isRelevant, text = self._isRelevantColumn(index)
        if isRelevant:
            self.pbClick.setGeometry(option.rect)
            self._updateButton(self.pbClick, index, text)
            if option.state == QStyle.State_Selected:
                painter.fillRect(option.rect, option.palette.highlight())
            pixmap = QPixmap.grabWidget(self.pbClick)
            painter.drawPixmap(option.rect.x(), option.rect.y(), pixmap)
        else:
            super(ButtonDelegate, self).paint(painter, option, index)

    def updateEditorGeometry(self, button, option, index):
        isRelevant, text = self._isRelevantColumn(index)
        if isRelevant:
            button.setGeometry(option.rect)
            self._updateButton(button, index, text)

    @pyqtSlot(object)
    def cellClicked(self, index):
        isRelevant, text = self._isRelevantColumn(index)
        if isRelevant:
            if self.cellEditMode:
                # Remove old persistent model index
                self.parent().closePersistentEditor(self.currentCellIndex)
            # Current model index is stored and added to stay persistent until
            # editing mode is done
            self.currentCellIndex = index
            # If no editor exists, the delegate will create a new editor which
            # means that here ``createEditor`` is called
            self.parent().openPersistentEditor(self.currentCellIndex)
            self.cellEditMode = True
            if text == ALARM_DATA[SHOW_DEVICE]:
                # Send signal to show device
                deviceId = index.data()
                data = {'deviceId': deviceId}
                # Create KaraboBroadcastEvent
                broadcast_event(KaraboBroadcastEvent(
                    KaraboEventSender.ShowDevice, data))
            else:
                # Send signal to acknowledge alarm
                id_index = getAlarmKeyIndex(ALARM_ID)
                model = index.model()
                alarm_id = model.index(index.row(), id_index).data()
                get_network().onAcknowledgeAlarm(model.instanceId, alarm_id)
        else:
            if self.cellEditMode:
                self.cellEditMode = False
                # Persistent model index and data namely QPushButton cleaned up
                self.parent().closePersistentEditor(self.currentCellIndex)
