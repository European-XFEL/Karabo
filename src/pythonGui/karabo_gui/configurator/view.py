#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on August 3, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import OrderedDict
from PyQt4.QtCore import pyqtSignal, pyqtSlot, QRect, Qt
from PyQt4.QtGui import (QAbstractItemDelegate, QAbstractItemView, QCursor,
                         QTreeView)

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
    signalApplyChanged = pyqtSignal(object, bool, bool)
    itemSelectionChanged = pyqtSignal()

    def __init__(self, conf=None, parent=None):
        super(ConfigurationTreeView, self).__init__(parent)
        self.setDragEnabled(True)
        self.setTabKeyNavigation(True)
        self.setSelectionMode(QAbstractItemView.ExtendedSelection)
        self.setSelectionBehavior(QAbstractItemView.SelectRows)
        triggers = (QAbstractItemView.CurrentChanged |
                    QAbstractItemView.SelectedClicked)
        self.setEditTriggers(triggers)

        # For compatibility with ConfigurationPanel
        self.conf = conf

        model = ConfigurationTreeModel(parent=self)
        self.setModel(model)
        self._set_model_configuration(conf)
        model.signalHasModifications.connect(self._update_apply_buttons)

        # Add a delegate for rows with slot buttons
        delegate = SlotButtonDelegate(parent=self)
        self.setItemDelegateForColumn(0, delegate)
        # ... and a delegate for the editable value column
        self.value_delegate = ValueDelegate(parent=self)
        self.setItemDelegateForColumn(2, self.value_delegate)

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

    @pyqtSlot(bool)
    def _update_apply_buttons(self, buttons_enabled):
        """The model is telling us to enable/disable the apply/decline buttons
        """
        configuration = self.model().configuration
        self.signalApplyChanged.emit(configuration, buttons_enabled, False)

    # ------------------------------------
    # Event handlers

    def closeEditor(self, editor, hint):
        """XXX: PyQt does not send this signal properly in the item delegate,
        so we avoid signals altogether and call it directly...
        """
        self.value_delegate.close_editor(editor, hint)
        super(ConfigurationTreeView, self).closeEditor(editor, hint)

    def closeEvent(self, event):
        event.accept()
        unregister_from_broadcasts(self)

    def currentChanged(self, current, previous):
        """Pass selection changes along to the value delegate
        """
        self.value_delegate.current_changed(current)
        super(ConfigurationTreeView, self).currentChanged(current, previous)

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

    def keyPressEvent(self, event):
        """Reimplemented function of QTreeView.
        """
        key_event = event.key()
        if key_event in (Qt.Key_Return, Qt.Key_Enter, Qt.Key_Escape):
            # Get current index
            indexes = self.selectionModel().selectedIndexes()
            if indexes:
                index = [idx for idx in indexes if idx.column() == 2][0]
                # Act on that item
                if key_event in (Qt.Key_Return, Qt.Key_Enter):
                    self.value_delegate.update_model_data(
                        index, QAbstractItemDelegate.SubmitModelCache)
                    return
                elif key_event == Qt.Key_Escape:
                    self.value_delegate.update_model_data(
                        index, QAbstractItemDelegate.RevertModelCache)
                    return

        super(ConfigurationTreeView, self).keyPressEvent(event)

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
        configuration = self.model().configuration
        configuration.clearUserValues()
        self.signalApplyChanged.emit(configuration, False, False)
        self.model().layoutChanged.emit()

    def decline_all_changes(self):
        pass

    def decline_item_changes(self, item):
        pass

    def globalAccessLevelChanged(self):
        pass

    def hideColumn(self, col):
        pass

    def onApplyAll(self):
        configuration = self.model().configuration
        configuration.sendAllUserValues()

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
