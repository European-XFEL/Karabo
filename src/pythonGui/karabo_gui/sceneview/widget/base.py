from collections import defaultdict

from PyQt4.QtCore import QRect, Qt, pyqtSlot
from PyQt4.QtGui import QHBoxLayout, QLabel, QStackedLayout, QWidget

from karabo.common.api import AlarmCondition, State
from karabo_gui.alarms.api import get_alarm_pixmap
from karabo_gui.displaywidgets.displaymissingbox import DisplayMissingBox
from karabo_gui.indicators import get_device_status_pixmap, STATE_COLORS
from karabo_gui.schema import box_has_changes
from karabo_gui.util import generateObjectName
from .utils import PendingBoxes, get_box


class BaseWidgetContainer(QWidget):
    """ A container for scene widgets which provides the interface
    needed by the scene tools.
    """
    def __init__(self, model, parent):
        super(BaseWidgetContainer, self).__init__(parent)
        self.model = model
        self._visible = False
        # Keep track of `Configuration` objects whose signals are connected
        self._configuration_connections = set()
        # Keep track of signals that are connected
        self._connected_signals = defaultdict(list)
        # Initialize this attribute. Will be used if any boxes are missing
        self._pending_boxes = None
        # Variables used by editable widgets
        self._editor_initialized = False
        self._is_editable = False
        self._style_sheet = ''

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
                    # Make new box visible
                    device_box.addVisible()
                    widget_added = True
        return widget_added

    def apply_changes(self):
        """Apply user-entered values to the remote device properties
        """
        if not (self._is_editable and self._visible):
            return

        for box in self.old_style_widget.boxes:
            if box.configuration.hasUserValue(box):
                box.configuration.sendUserValue(box)

    def decline_changes(self):
        """Undo any user-entered changes
        """
        if not (self._is_editable and self._visible):
            return

        for b in self.old_style_widget.boxes:
            b.configuration.clearUserValue(b)
        self._update_background_color()

    def destroy(self):
        """ Disconnect the box signals
        """
        # Do this first; just in case it finishes its work during this method
        self._destroy_pending_boxes()

        # Then disconnect all the connected signals
        for signal, receivers in self._connected_signals.items():
            for recvr in receivers:
                signal.disconnect(recvr)
        self._connected_signals.clear()
        self._configuration_connections.clear()

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
    # Qt methods

    def keyPressEvent(self, event):
        """Watch for editing events involving the enter and escape keys
        """
        key_code = event.key()
        if key_code == Qt.Key_Escape:
            self.decline_changes()
        elif key_code in (Qt.Key_Enter, Qt.Key_Return):
            self.apply_changes()
        super(BaseWidgetContainer, self).keyPressEvent(event)

    # ---------------------------------------------------------------------
    # Internal methods

    def _add_alarm_symbol(self, layout):
        """ Add alarm symbol to the given ``layout`` and return the widget this
        layout then belongs to
        """
        layout.addWidget(self.alarm_symbol)
        layout_widget = QWidget()
        layout_widget.setLayout(layout)
        objectName = generateObjectName(self)
        layout_widget.setObjectName(objectName)
        self._style_sheet = ("QWidget#{}".format(objectName) +
                             " {{ background-color : rgba{}; }}")
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

    def _connect_signal(self, signal, receiver):
        self._connected_signals[signal].append(receiver)
        signal.connect(receiver)

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
        signals = {}
        widget = self.old_style_widget
        signals[box.signalNewDescriptor] = widget.typeChangedSlot
        if self.model.parent_component == 'EditableApplyLaterComponent':
            signals[widget.signalEditingFinished] = self._on_editing_finished
            signals[box.signalUserChanged] = self._on_user_edit
            signals[box.signalUpdateComponent] = self._on_display_value_change
            widget.setReadOnly(False)
        else:
            signals[box.signalUpdateComponent] = widget.valueChangedSlot
            widget.setReadOnly(True)

        for signal, receiver in signals.items():
            self._connect_signal(signal, receiver)

        if box.descriptor is not None:
            widget.typeChangedSlot(box)
        if box.hasValue():
            widget.valueChanged(box, box.value, box.timestamp)

        device = box.configuration
        if device.id not in self._configuration_connections:
            self._connect_signal(device.signalStatusChanged,
                                 self._device_status_changed)
            self._configuration_connections.add(device.id)
            self._device_status_changed(device, device.status, device.error)

    def _setup_wrapped_widget(self):
        """Wrap up the alarm symbol and possible edit buttons in a layout with
        the `old_style_widget`.
        """
        for box in self.boxes:
            self._make_box_connections(box)

        layout = QHBoxLayout()
        layout.addWidget(self.old_style_widget.widget)
        disp_widgets = self._add_alarm_symbol(layout)
        self.layout.addWidget(disp_widgets)

        if self.model.parent_component == 'EditableApplyLaterComponent':
            self._is_editable = True
            layout.setContentsMargins(2, 2, 2, 2)
        else:
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
    # Editing related code

    @pyqtSlot(object, object)
    def _on_display_value_change(self, box, value):
        widget = self.old_style_widget
        if not self._editor_initialized:
            widget.valueChanged(box, value)
            self._editor_initialized = True
        self._update_background_color()

    @pyqtSlot(object, object)
    def _on_editing_finished(self, box, value):
        self._update_box_value(box, value)

    @pyqtSlot(object, object, object)
    def _on_user_edit(self, box, value, timestamp=None):
        self.old_style_widget.valueChangedSlot(box, value, timestamp)
        self._update_background_color()

    def _update_background_color(self):
        if not (self._is_editable and self.boxes):
            return

        box = self.boxes[0]
        conf = box.configuration
        if conf.hasUserValue(box):
            if box.has_conflict:
                color = STATE_COLORS[State.UNKNOWN] + (128,)
            else:
                color = STATE_COLORS[State.CHANGING] + (128,)
            formatted_sheet = self._style_sheet.format(color)
            self.setStyleSheet(formatted_sheet)
        else:
            self.setStyleSheet('')

    def _update_box_value(self, box, value):
        old_value = box.value if box.hasValue() else None
        has_changes = box_has_changes(box.descriptor, old_value, value)
        apply_changed = box.isAllowed() and has_changes
        configuration = box.configuration
        if apply_changed:
            configuration.setUserValue(box, value)  # Store in config
        else:
            configuration.clearUserValue(box)  # Remove from config
        self._update_background_color()
