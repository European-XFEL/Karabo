#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 16, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from functools import partial

from PyQt4.QtCore import pyqtSlot, Qt, QTimer
from PyQt4.QtGui import (
    QButtonGroup, QColor, QComboBox, QHBoxLayout, QLabel, QLineEdit, QPixmap,
    QPushButton, QStyle, QStyledItemDelegate, QTableView, QVBoxLayout, QWidget)

from karabogui.alarms.api import (
    ACKNOWLEDGE, ALARM_COLOR, ALARM_DATA, ALARM_ID, ALARM_TYPE, DEVICE_ID,
    PROPERTY, SHOW_DEVICE, AlarmModel, get_alarm_key_index)
from karabogui.events import (
    KaraboEventSender, broadcast_event, register_for_broadcasts,
    unregister_from_broadcasts)
from karabogui.singletons.api import get_network
from .base import BasePanelWidget


class AlarmPanel(BasePanelWidget):
    TIMER_INTERVAL = 500
    BLACK_COLOR = QColor(Qt.black)
    RED_COLOR = QColor(*ALARM_COLOR)

    def __init__(self, instanceId):
        self.instanceId = instanceId

        # Important: call the BasePanelWidget initializer
        super(AlarmPanel, self).__init__(instanceId)

        self._color_toggle = False
        self._color_timer = QTimer()
        self._color_timer.setInterval(self.TIMER_INTERVAL)
        self._color_timer.timeout.connect(self._color_timeout)

        # Register to broadcast events
        register_for_broadcasts(self)

    def karaboBroadcastEvent(self, event):
        sender = event.sender
        data = event.data
        if sender is KaraboEventSender.AlarmServiceInit:
            self.model.initAlarms(data.get('instance_id'),
                                  data.get('update_types'),
                                  data.get('alarm_entries'))
            self._alternate_title_color()
        elif sender is KaraboEventSender.AlarmServiceUpdate:
            self.model.updateAlarms(data.get('instance_id'),
                                    data.get('update_types'),
                                    data.get('alarm_entries'))
            self._alternate_title_color()
        elif sender is KaraboEventSender.NetworkConnectStatus:
            if not data['status']:
                # If disconnected to server, unregister the alarm panel from
                # broadcast, otherwise we will get two times of the same info
                # next time
                unregister_from_broadcasts(self)
        return False

    def closeEvent(self, event):
        super(AlarmPanel, self).closeEvent(event)
        if event.isAccepted():
            unregister_from_broadcasts(self)

    def get_content_widget(self):
        """Returns a QWidget containing the main content of the panel.
        """
        widget = QWidget(parent=self)
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
        self.model = AlarmModel(self.instanceId, self.twAlarm)
        btn_delegate = ButtonDelegate(parent=self.twAlarm)
        self.twAlarm.setItemDelegate(btn_delegate)

        main_layout = QVBoxLayout(widget)
        main_layout.setContentsMargins(5, 5, 5, 5)
        main_layout.addLayout(filterLayout)
        main_layout.addWidget(self.twAlarm)

        return widget

    def _alternate_title_color(self):
        if self.model.allEntries:
            self._color_timer.start()
        else:
            self._color_timer.stop()
            # Make sure to have default color again
            self.update_tab_text_color(self.BLACK_COLOR)

    def _enableCustomFilter(self, enable):
        self.cbFilterType.setEnabled(enable)
        self.leFilterText.setEnabled(enable)

    @pyqtSlot()
    def _color_timeout(self):
        if self._color_toggle:
            title_color = self.RED_COLOR
        else:
            title_color = self.BLACK_COLOR
        self._color_toggle = not self._color_toggle
        self.update_tab_text_color(title_color)

    @property
    def model(self):
        return self.twAlarm.model()

    @model.setter
    def model(self, model):
        self.twAlarm.setModel(model)

    @pyqtSlot(object)
    def filterToggled(self, button):
        """ The filter ``button`` was activated. Update filtering needed."""
        if button is self.pbDefaultView:
            self._enableCustomFilter(False)
            self.model.updateFilter(filterType=None)
        elif button is self.pbAcknowledgeOnly:
            self._enableCustomFilter(False)
            self.model.updateFilter(filterType=ACKNOWLEDGE)
        elif button is self.pbCustomFilter:
            self._enableCustomFilter(True)
            data = self.cbFilterType.itemData(self.cbFilterType.currentIndex())
            filterText = self.leFilterText.text()
            self.model.updateFilter(filterType=data, text=filterText)


class ButtonDelegate(QStyledItemDelegate):
    def __init__(self, parent=None):
        super(ButtonDelegate, self).__init__(parent)
        # Fake button used for later rendering
        self.pbClick = QPushButton("")
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
            # the index, is deleted whenever `closePersistentEditor` is called
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
        isRelevant, text, clickable = self._isRelevantColumn(index)
        if isRelevant and clickable:
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
                broadcast_event(KaraboEventSender.ShowDevice,
                                {'deviceId': index.data()})
            else:
                # Send signal to acknowledge alarm
                id_index = get_alarm_key_index(ALARM_ID)
                model = index.model()
                alarm_id = model.index(index.row(), id_index).data()
                get_network().onAcknowledgeAlarm(model.instanceId, alarm_id)
        else:
            if self.cellEditMode:
                self.cellEditMode = False
                # Persistent model index and data namely QPushButton cleaned up
                self.parent().closePersistentEditor(self.currentCellIndex)
