#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on September 16, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import (QButtonGroup, QComboBox, QHBoxLayout, QLabel, QPixmap,
                         QPushButton, QStyle, QStyledItemDelegate, QTableView,
                         QVBoxLayout, QWidget)

from karabo_gui.alarm_model import (
    ACKNOWLEDGE, ALARM_DATA, ALARM_ID, AlarmModel, getAlarmKeyIndex,
    SHOW_DEVICE)
from karabo_gui.docktabwindow import Dockable
from karabo_gui.mediator import (KaraboBroadcastEvent, KaraboEventSender,
                                 broadcast_event, register_for_broadcasts)
from karabo_gui.network import Network


class AlarmPanel(Dockable, QWidget):
    def __init__(self):
        super(AlarmPanel, self).__init__()

        self.bg_filter = QButtonGroup()
        pb_default_view = QPushButton("Default view")
        pb_default_view.setCheckable(True)
        pb_default_view.setChecked(True)
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

        self.twAlarm = QTableView()
        self.twAlarm.setWordWrap(True)
        self.twAlarm.setAlternatingRowColors(True)
        self.twAlarm.resizeColumnsToContents()
        self.twAlarm.horizontalHeader().setStretchLastSection(True)
        alarm_model = AlarmModel()
        self.twAlarm.setModel(alarm_model)
        btn_delegate = ButtonDelegate(self.twAlarm)
        self.twAlarm.setItemDelegate(btn_delegate)

        main_layout = QVBoxLayout(self)
        main_layout.setContentsMargins(5, 5, 5, 5)
        main_layout.addLayout(filter_layout)
        main_layout.addWidget(self.twAlarm)

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
        pass

    def setupToolBars(self, toolBar, parent):
        pass

    def setEnabled(self, enable):
        if enable:
            Network().onRequestAlarms()
        super(AlarmPanel, self).setEnabled(enable)

    def _initAlarms(self, instanceId, rows):
        self.twAlarm.model().initAlarms(instanceId, rows)

    def _updateAlarms(self, instanceId, rows):
        self.twAlarm.model().updateAlarms(instanceId, rows)


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
                Network().onAcknowledgeAlarm(model.instanceId, alarm_id)
        else:
            if self.cellEditMode:
                self.cellEditMode = False
                # Persistent model index and data namely QPushButton cleaned up
                self.parent().closePersistentEditor(self.currentCellIndex)
