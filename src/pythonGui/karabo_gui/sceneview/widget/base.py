from PyQt4.QtCore import QRect, QSize, Qt, QTimer, pyqtSlot
from PyQt4.QtGui import (QAction, QHBoxLayout, QLabel, QStackedLayout,
                         QToolButton, QWidget)

from karabo.common.api import AlarmCondition
from karabo_gui import icons
from karabo_gui.alarms.api import get_alarm_pixmap
from karabo_gui.displaywidgets.displaymissingbox import DisplayMissingBox
from karabo_gui.indicators import get_device_status_pixmap
from karabo_gui.singletons.api import get_network
from .utils import PendingBoxes, get_box, determine_if_value_unchanged


class BaseWidgetContainer(QWidget):
    """ A container for scene widgets which provides the interface
    needed by the scene tools.
    """
    def __init__(self, model, parent):
        super(BaseWidgetContainer, self).__init__(parent)
        self.model = model
        self._visible = False
        # Keep track of `Configuration` objects whose signals are connected
        self._configuration_connections = {}
        # Initialize this attribute. Will be used if any boxes are missing
        self._pending_boxes = None

        self.alarm_symbol = QLabel("", self)
        self.status_symbol = QLabel("", self)
        self.status_symbol.setAttribute(Qt.WA_TransparentForMouseEvents)

        self.layout = QStackedLayout(self)
        self.layout.setStackingMode(QStackedLayout.StackAll)
        self.layout.addWidget(self.status_symbol)

        # Handle boxes. If anything is missing, we wipe the slate clean
        self.boxes = [get_box(*key.split('.', 1)) for key in self.model.keys]
        if None in self.boxes:
            self.boxes = []
            self.old_style_widget = DisplayMissingBox(self)
            self.layout.addWidget(self.old_style_widget.widget)
            self._watch_devices_for_boxes()
        else:
            # Normal operation
            self.old_style_widget = self._create_widget(self.boxes)
            self._setup_wrapped_widget()

        self.setGeometry(QRect(model.x, model.y, model.width, model.height))
        self.setToolTip(", ".join(self.model.keys))
        self.update_alarm_symbol()

    def _create_widget(self, boxes):
        """ A method for creating the child widget.

        THIS MUST BE IMPLEMENTED BY DERIVED CLASSES
        """
        raise NotImplementedError

    def add_boxes(self, boxes):
        """ Add more boxes to a display widget which allows more than one
            box. ``True`` is returned when this was possible, otherwise
            ``False`` is returned.
        """
        widget = self.old_style_widget
        widget_added = False
        if self.model.parent_component == 'DisplayComponent':
            for box in boxes:
                device_id, property_path = box.key().split('.', 1)
                device_box = get_box(device_id, property_path)
                if device_box is None:
                    continue
                # This is only ``True`` if the display widget allows more
                # than one box to be added to it
                if widget.addBox(device_box):
                    self.model.keys.append(device_box.key())
                    self.boxes.append(device_box)
                    self._make_box_connections(device_box)
                    widget_added = True
        return widget_added

    def destroy(self):
        """ Disconnect the box signals
        """
        # Do this first; just in case it finishes its work during this method
        self._destroy_pending_boxes()

        widget = self.old_style_widget
        if self.model.parent_component == 'EditableApplyLaterComponent':
            if self.boxes:
                box = self.boxes[0]
                sig = widget.signalEditingFinished
                sig.disconnect(self._on_editing_finished)
                # These are connected in `EditableWidget.__init__`
                sig = box.configuration.boxvalue.state.signalUpdateComponent
                sig.disconnect(widget.updateStateSlot)

        for box in self.boxes:
            box.signalNewDescriptor.disconnect(widget.typeChangedSlot)
            if self.model.parent_component == 'EditableApplyLaterComponent':
                box.signalUserChanged.disconnect(self._on_user_edit)
                box.signalUpdateComponent.disconnect(
                    self._on_display_value_change)
            else:  # DisplayWidgets
                box.signalUpdateComponent.disconnect(widget.valueChangedSlot)

        for config in self._configuration_connections.values():
            config.signalStatusChanged.disconnect(self._device_status_changed)

    def set_geometry(self, rect):
        self.model.set(x=rect.x(), y=rect.y(),
                       width=rect.width(), height=rect.height())
        self.setGeometry(rect)

    def set_visible(self, visible):
        """ Set whether this widget is seen by the user."""
        self._visible = visible
        if visible:
            for box in self.boxes:
                box.addVisible()
        else:
            for box in self.boxes:
                box.removeVisible()

    def translate(self, offset):
        new_pos = self.pos() + offset
        self.model.set(x=new_pos.x(), y=new_pos.y())
        self.move(new_pos)

    def update_alarm_symbol(self):
        """Update the alarm symbol with a pixmap matching the given
        ``alarm_type``
        """
        widget_alarms = []
        for b in self.boxes:
            system_topo_node = b.configuration.topology_node
            if system_topo_node is None:
                continue

            alarm_dict = system_topo_node.alarm_info.alarm_dict
            property_name = '.'.join(b.path)
            for alarm_type, properties in alarm_dict.items():
                # feature: show global alarm_type for 'state' property
                if property_name == 'state' or property_name in properties:
                    widget_alarms.append(AlarmCondition.fromString(alarm_type))

        pixmap = None
        if widget_alarms:
            widget_alarms.sort()  # AlarmCondition is sortable
            pixmap = get_alarm_pixmap(widget_alarms[-1].asString())

        if pixmap is not None:
            self.alarm_symbol.setPixmap(pixmap)
            self.alarm_symbol.show()
        else:
            self.alarm_symbol.hide()

    def update_global_access_level(self):
        """Update the widget based on a new global access level.
        """
        self.old_style_widget.updateState()

    # ---------------------------------------------------------------------
    # Internal methods

    def _add_alarm_symbol(self, layout):
        """ Add alarm symbol to the given ``layout`` and return the widget this
        layout then belongs to
        """
        layout.addWidget(self.alarm_symbol)
        layout_widget = QWidget()
        layout_widget.setLayout(layout)
        return layout_widget

    def _boxes_ready(self):
        """Boxes which had to wait for schema changes are now ready to be used!
        """
        self.boxes = self._pending_boxes.boxes[:]
        self._destroy_pending_boxes()

        self.layout.takeAt(1)  # There are only two widgets in this layout
        self.old_style_widget = self._create_widget(self.boxes)
        self._setup_wrapped_widget()
        # Only if we are already visible!
        if self._visible:
            self.set_visible(True)

    def _destroy_pending_boxes(self):
        if self._pending_boxes is None:
            return

        self._pending_boxes.on_trait_change(self._boxes_ready, 'ready',
                                            remove=True)
        self._pending_boxes.destroy()
        self._pending_boxes = None

    def _make_box_connections(self, box):
        """ Hook up all the box signals to the old_style_widget instance.
        """
        widget = self.old_style_widget
        box.signalNewDescriptor.connect(widget.typeChangedSlot)
        if box.descriptor is not None:
            widget.typeChangedSlot(box)
        box.signalUpdateComponent.connect(widget.valueChangedSlot)
        if box.hasValue():
            widget.valueChanged(box, box.value, box.timestamp)
        if self.model.parent_component == 'EditableApplyLaterComponent':
            widget.signalEditingFinished.connect(self._on_editing_finished)
            box.signalUserChanged.connect(self._on_user_edit)
            box.signalUpdateComponent.disconnect(widget.valueChangedSlot)
            box.signalUpdateComponent.connect(self._on_display_value_change)
            widget.setReadOnly(False)
        else:
            widget.setReadOnly(True)

        device = box.configuration
        if device.id not in self._configuration_connections:
            device.signalStatusChanged.connect(self._device_status_changed)
            self._device_status_changed(device, device.status, device.error)
            self._configuration_connections[device.id] = device

    def _setup_wrapped_widget(self):
        """Wrap up the alarm symbol and possible edit buttons in a layout with
        the `old_style_widget`.
        """
        for box in self.boxes:
            self._make_box_connections(box)

        if self.model.parent_component == 'EditableApplyLaterComponent':
            layout = self._add_edit_widgets()
            edit_widgets = self._add_alarm_symbol(layout)
            self.layout.addWidget(edit_widgets)
        else:
            layout = QHBoxLayout()
            layout.addWidget(self.old_style_widget.widget)
            disp_widgets = self._add_alarm_symbol(layout)
            self.layout.addWidget(disp_widgets)
            layout.setContentsMargins(0, 0, 0, 0)

    def _watch_devices_for_boxes(self):
        """Find out which boxes are missing and arrange to fill them in as
        their controlling device updates its schema.
        """
        self._pending_boxes = PendingBoxes()
        self._pending_boxes.on_trait_change(self._boxes_ready, 'ready')
        self._pending_boxes.keys = self.model.keys

    @pyqtSlot(object, object, bool)
    def _device_status_changed(self, configuration, status, error):
        """ Callback when the status of the device is changes.
        """
        pixmap = get_device_status_pixmap(status, error)
        if pixmap is not None:
            self.status_symbol.setPixmap(pixmap)
            self.status_symbol.show()
        else:
            self.status_symbol.hide()

    # ---------------------------------------------------------------------
    # Edit buttons related code

    def _add_edit_widgets(self):
        """ Add the extra buttons needed for editable widgets
        """
        def _create_button(text, description, icon, trigger, layout):
            button = QAction(icon, text, self)
            button.setStatusTip(description)
            button.setToolTip(description)
            button.triggered.connect(trigger)
            tb = QToolButton()
            tb.setDefaultAction(button)
            tb.setIconSize(QSize(24, 24))
            layout.addWidget(tb)
            return button, tb

        layout = QHBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self.old_style_widget.widget)

        text = "Apply"
        description = "Apply property changes"
        self.apply_button, tb = _create_button(
            text, description, icons.applyGrey, self._on_apply_clicked, layout)
        tb.setPopupMode(QToolButton.InstantPopup)

        text = "Decline"
        description = ("Decline property changes "
                       "and reset them to value on device")
        self.decline_button, _ = _create_button(
            text, description, icons.no, self._on_decline_clicked, layout)

        self.__busyTimer = QTimer(self)
        self.__busyTimer.setSingleShot(True)
        self.__busyTimer.timeout.connect(self._on_timeout)
        self.__current_value = None
        self.__has_conflict = False

        return layout

    def _on_apply_clicked(self):
        widget = self.old_style_widget
        network = get_network()
        changes = []
        for b in widget.boxes:
            b.signalUserChanged.emit(b, widget.value, None)
            if b.configuration.type == "macro":
                b.set(widget.value)
            elif b.descriptor is not None:
                changes.append((b, widget.value))

        if changes:
            self.__busyTimer.start(5000)
            network.onReconfigure(changes)

    def _on_decline_clicked(self):
        widget = self.old_style_widget
        for b in widget.boxes:
            widget.valueChanged(b, self.__current_value)
        self._update_buttons()

    @pyqtSlot(object, object)
    def _on_display_value_change(self, box, value):
        widget = self.old_style_widget
        if self.__current_value is None:
            widget.valueChanged(box, value)
        self.__current_value = value
        self.__busyTimer.stop()
        self.__has_conflict = True
        self._update_buttons()

    def _on_editing_finished(self, box, value):
        if self.__current_value is None:
            return
        self._update_buttons()

    def _on_timeout(self):
        pass

    @pyqtSlot(object, object, object)
    def _on_user_edit(self, box, value, timestamp=None):
        self.old_style_widget.valueChangedSlot(box, value, timestamp)
        self._update_buttons()

    def _update_buttons(self):
        widget = self.old_style_widget
        if self.boxes:
            box = self.boxes[0]
            allowed = box.isAllowed()
            value_unchanged = determine_if_value_unchanged(
                self.__current_value, widget.value, box)
        else:
            allowed = False
            value_unchanged = True

        self.apply_button.setEnabled(allowed)

        if value_unchanged:
            self.apply_button.setIcon(icons.applyGrey)
            self.__has_conflict = False
            description = None
        elif self.__has_conflict:
            self.apply_button.setIcon(icons.applyConflict)
            description = "Apply my property changes"
        else:
            description = "Apply property changes"
            self.apply_button.setIcon(icons.apply)

        self.apply_button.setStatusTip(description)
        self.apply_button.setToolTip(description)
        self.decline_button.setEnabled(allowed and not value_unchanged)
