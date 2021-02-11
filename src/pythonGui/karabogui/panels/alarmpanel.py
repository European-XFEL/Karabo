#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 16, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt5.QtCore import pyqtSlot, QModelIndex
from PyQt5.QtWidgets import (
    QAbstractButton, QAbstractItemView, QButtonGroup, QHBoxLayout, QHeaderView,
    QPushButton, QRadioButton, QStyle, QStyledItemDelegate, QTableView,
    QVBoxLayout, QWidget)
from karabogui import icons
from karabogui.alarms.api import (
    ACKNOWLEDGE, ALARM_DATA, ALARM_ID, ALARM_WARNING_TYPES, AlarmFilterModel,
    INTERLOCK_TYPES, get_alarm_key_index)
from karabogui.events import broadcast_event, KaraboEvent
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

    @pyqtSlot(QAbstractButton)
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

    @pyqtSlot(QModelIndex)
    def onRowDoubleClicked(self, index):
        """A double click on a column should select the device in topology"""
        value = index.data()
        if value is None:
            return
        deviceId = self.model.index(index.row(), DEVICE_COLUMN).data()
        broadcast_event(KaraboEvent.ShowDevice, {'deviceId': deviceId})


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
        if column != ACKNOWLEDGE_COLUMN:
            return (False, 'None', False)

        text = ALARM_DATA[ACKNOWLEDGE]
        needsAck, ack = index.data()
        clickable = needsAck and ack

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
            pixmap = self.pbClick.grab()
            painter.drawPixmap(option.rect.x(), option.rect.y(), pixmap)
        else:
            super(ButtonDelegate, self).paint(painter, option, index)

    def updateEditorGeometry(self, button, option, index):
        isRelevant, text, clickable = self._isRelevantColumn(index)
        if isRelevant:
            button.setGeometry(option.rect)
            self._updateButton(button, text, clickable)

    @pyqtSlot(QModelIndex)
    def cellClicked(self, index):
        if not index.isValid():
            return
        isRelevant, text, clickable = self._isRelevantColumn(index)
        if isRelevant and clickable:
            # Send signal to acknowledge alarm
            id_index = get_alarm_key_index(ALARM_ID)
            model = index.model()
            alarm_id = model.index(index.row(), id_index).data()
            get_network().onAcknowledgeAlarm(model.instanceId, alarm_id)
