#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on August 3, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import OrderedDict
from PyQt4.QtCore import pyqtSignal, QRect, Qt
from PyQt4.QtGui import QAbstractItemView, QCursor, QTreeView

from karabo.middlelayer import Type
from karabo_gui.alarms.api import ALARM_LOW, ALARM_HIGH, WARN_LOW, WARN_HIGH
from karabo_gui.events import (
    KaraboEventSender, register_for_broadcasts, unregister_from_broadcasts)
from karabo_gui.popupwidget import PopupWidget
from karabo_gui.schema import (
    EditableAttributeInfo, VectorHashCellInfo, VectorHashRowInfo)
from .qt_item_model import ConfigurationTreeModel
from .slot_delegate import SlotButtonDelegate
from .value_delegate import ValueDelegate


class ConfigurationTreeView(QTreeView):
    """A tree view for `Configuration` instances
    """
    signalApplyChanged = pyqtSignal()
    itemSelectionChanged = pyqtSignal()

    def __init__(self, conf=None, parent=None):
        super(ConfigurationTreeView, self).__init__(parent)
        self.setDragEnabled(True)
        self.setSelectionMode(QAbstractItemView.ExtendedSelection)
        self.setSelectionBehavior(QAbstractItemView.SelectRows)
        self.setEditTriggers(QAbstractItemView.AllEditTriggers)

        # For compatibility with ConfigurationPanel
        self.conf = conf

        model = ConfigurationTreeModel(parent=self)
        self.setModel(model)
        self._set_model_configuration(conf)

        # Add a delegate for rows with slot buttons
        delegate = SlotButtonDelegate(parent=self)
        self.setItemDelegateForColumn(0, delegate)
        # ... and a delegate for the editable value column
        delegate = ValueDelegate(parent=self)
        self.setItemDelegateForColumn(2, delegate)

        # Widget for more information of an index
        self.popup_widget = None

        # Don't forget to unregister!
        register_for_broadcasts(self)

    # ------------------------------------
    # Private methods

    def _set_model_configuration(self, conf):
        self.model().configuration = conf
        if conf is None:
            return

        if conf.type == 'device':
            # Show second column only for devices
            self.setColumnHidden(1, False)
        else:
            # Hide second column for others
            self.setColumnHidden(1, True)

    def _show_popup_widget(self, index, event_pos):
        # Only if the icon was clicked
        # Get the tree widget's x position
        tree_x = self.header().sectionViewportPosition(0)
        # Get the x coordinate of the root index in order to calculate
        # the identation of the index
        root_x = self.visualRect(self.rootIndex()).x()
        # Get the rectangle of the viewport occupied by index
        vis_rect = self.visualRect(index)
        # Calculate the x coordinate of the item
        index_x = tree_x + vis_rect.x() - root_x
        # Get the rect surrounding the icon
        icon_rect = QRect(index_x, vis_rect.y(), vis_rect.height(),
                          vis_rect.height())

        if not icon_rect.contains(event_pos):
            return

        if self.popup_widget is None:
            self.popup_widget = PopupWidget(self)

        # Get the index's stored object
        obj = self.model().index_ref(index)
        if obj is None:
            return

        if isinstance(obj, EditableAttributeInfo):
            obj = obj.parent()
            if obj is None:
                return
        elif isinstance(obj, (VectorHashCellInfo, VectorHashRowInfo)):
            # Nothing to show here
            return
        descriptor = obj.descriptor
        property_name = index.data(Qt.DisplayRole)
        info = OrderedDict()
        if property_name:
            info["Property"] = property_name
        if descriptor.description is not None:
            info["Description"] = descriptor.description

        info["Key"] = '.'.join(obj.path)
        if isinstance(descriptor, Type):
            info["Value Type"] = descriptor.hashname()
        if descriptor.defaultValue is not None:
            info["Default Value"] = descriptor.defaultValue
        if descriptor.alias is not None:
            info["Alias"] = descriptor.alias
        if descriptor.tags is not None:
            info["Tags"] = ", ".join(descriptor.tags)
        if obj.timestamp is not None:
            info["Timestamp"] = obj.timestamp.toLocal()
        if (descriptor.displayType
                and descriptor.displayType.startswith('bin|')):
            info["Bits"] = descriptor.displayType[4:]
        if obj.configuration.type == "device":
            info["Value on device"] = obj.value

        alarms = [('Warn low', WARN_LOW), ('Warn high', WARN_HIGH),
                  ('Alarm low', ALARM_LOW), ('Alarm high', ALARM_HIGH)]
        for label, alarm in alarms:
            this_alarm = getattr(descriptor, alarm)
            info[label] = 'n/a' if this_alarm is None else this_alarm

        self.popup_widget.setInfo(info)

        pos = QCursor.pos()
        pos.setX(pos.x() + 10)
        pos.setY(pos.y() + 10)
        self.popup_widget.move(pos)
        self.popup_widget.show()

    # ------------------------------------
    # Event handlers

    def closeEvent(self, event):
        event.accept()
        unregister_from_broadcasts(self)

    def karaboBroadcastEvent(self, event):
        sender = event.sender
        if sender is KaraboEventSender.AccessLevelChanged:
            model = self.model()
            config = model.configuration
            self._set_model_configuration(None)
            self._set_model_configuration(config)

        return False

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            index = self.indexAt(event.pos())
            if index.isValid() and index.column() == 0:
                self._show_popup_widget(index, event.pos())
        super(ConfigurationTreeView, self).mousePressEvent(event)

    # ------------------------------------
    # NOTE: All of these methods are to stay compatible with the existing
    # configurator panel code.

    def addContextAction(self, action):
        pass

    def addContextMenu(self, menu):
        pass

    def addContextSeparator(self):
        pass

    def clear(self):
        self._set_model_configuration(None)

    def decline_all(self):
        pass

    def decline_all_changes(self):
        pass

    def decline_item_changes(self, item):
        pass

    def globalAccessLevelChanged(self):
        pass

    def hideColumn(self, col):
        pass

    def onApplyAll(self):
        pass

    def nbSelectedApplyEnabledItems(self):
        """Return only selected items for not applied yet
        """
        return 0

    def selectedItems(self):
        return []

    def setErrorState(self, inErrorState):
        pass

    def setHeaderLabels(self, labels):
        pass

    def setReadOnly(self, readOnly):
        pass
