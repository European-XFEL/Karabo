from PyQt4.QtCore import QRect, Qt
from PyQt4.QtGui import QHBoxLayout, QLabel, QStackedLayout, QWidget

from karabo.common.api import AlarmCondition, DeviceStatus, State
from karabogui.alarms.api import get_alarm_pixmap
from karabogui.binding.api import has_changes
from karabogui.indicators import get_device_status_pixmap, STATE_COLORS
from karabogui.request import send_property_changes
from karabogui.util import generateObjectName
from .utils import get_proxy


class ControllerContainer(QWidget):
    """A container for scene widgets which provides the interface needed
    by the scene tools.
    """
    def __init__(self, klass, model, parent):
        super(ControllerContainer, self).__init__(parent)
        self.model = model
        # Variables used by editable widgets
        self._is_editable = False
        self._style_sheet = ''

        self.alarm_symbol = QLabel('', self)
        self.status_symbol = QLabel('', self)
        self.status_symbol.setAttribute(Qt.WA_TransparentForMouseEvents)

        self.layout = QStackedLayout(self)
        self.layout.setStackingMode(QStackedLayout.StackAll)
        self.layout.addWidget(self.status_symbol)

        proxies = [get_proxy(*key.split('.', 1)) for key in self.model.keys]
        self.widget_controller = self._create_controller(klass, proxies)
        self._setup_wrapped_widget()

        self.setGeometry(QRect(model.x, model.y, model.width, model.height))
        self.setToolTip(', '.join(self.model.keys))
        self.update_alarm_symbol()

        # Trigger the status change once (might be offline)
        device_proxy = self.widget_controller.proxy.root_proxy
        self._device_status_changed(device_proxy.status)

    def add_proxies(self, proxies):
        """Add more proxies to a controller which allows more than one proxy.
        ``True`` is returned when this was possible, otherwise ``False`` is
        returned.
        """
        if self.model.parent_component != 'DisplayComponent':
            return False

        controller = self.widget_controller
        proxies_added = []
        for proxy in proxies:
            if controller.visualize_additional_property(proxy):
                proxies_added.append(proxy)
        return len(proxies_added) == len(proxies)

    def apply_changes(self):
        """Apply user-entered values to the remote device properties
        """
        if not (self._is_editable and self.widget_controller.proxy.visible):
            return

        send_property_changes(self.widget_controller.proxies)

    def decline_changes(self):
        """Undo any user-entered changes
        """
        if not (self._is_editable and self.widget_controller.proxy.visible):
            return

        for proxy in self.widget_controller.proxies:
            proxy.revert_edit()

    def destroy(self):
        """Tell the controller to clean up
        """
        proxy = self.widget_controller.proxy
        proxy.root_proxy.on_trait_change(self._device_status_changed, 'status',
                                         remove=True)
        if self._is_editable:
            proxy.on_trait_change(
                self._on_user_edit, 'edit_value,binding.config_update',
                remove=True
            )

        self.widget_controller.destroy()
        self.widget_controller = None

    def set_geometry(self, rect):
        self.model.set(x=rect.x(), y=rect.y(),
                       width=rect.width(), height=rect.height())
        self.setGeometry(rect)

    def set_visible(self, visible):
        """Set whether this widget is seen by the user."""
        if visible:
            self.widget_controller.show()
        else:
            self.widget_controller.hide()

    def translate(self, offset):
        new_pos = self.pos() + offset
        self.model.set(x=new_pos.x(), y=new_pos.y())
        self.move(new_pos)

    def update_alarm_symbol(self):
        """Update the alarm symbol with a pixmap matching the given
        ``alarm_type``
        """
        widget_alarms = []
        for p in self.widget_controller.proxies:
            system_topo_node = p.root_proxy.topology_node
            if system_topo_node is None:
                continue

            alarm_dict = system_topo_node.alarm_info.alarm_dict
            property_name = p.path
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
        # Treat it like a device state update
        proxy = self.widget_controller.proxy
        self.widget_controller.state_update(proxy)

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
        super(ControllerContainer, self).keyPressEvent(event)

    # ---------------------------------------------------------------------
    # Internal methods

    def _add_alarm_symbol(self, layout):
        """Add alarm symbol to the given ``layout`` and return the widget this
        layout then belongs to
        """
        layout.addWidget(self.alarm_symbol)
        layout_widget = QWidget()
        layout_widget.setLayout(layout)
        objectName = generateObjectName(self)
        layout_widget.setObjectName(objectName)
        self._style_sheet = ('QWidget#{}'.format(objectName) +
                             ' {{ background-color : rgba{}; }}')
        return layout_widget

    def _create_controller(self, klass, proxies):
        """Create the binding controller instance.
        """
        controller = klass(proxy=proxies[0], model=self.model)
        controller.create(self)
        for proxy in proxies[1:]:
            # XXX: Assert that this returns True??
            controller.visualize_additional_property(proxy)

        # Attach a handler for the 'status' trait of our main device
        device_proxy = controller.proxy.root_proxy
        device_proxy.on_trait_change(self._device_status_changed, 'status')

        return controller

    def _device_status_changed(self, status):
        """Traits notification callback when the status of the device changes.
        """
        error = (status is DeviceStatus.ERROR)
        pixmap = get_device_status_pixmap(status, error)
        if pixmap is not None:
            self.status_symbol.setPixmap(pixmap)
            self.status_symbol.show()
        else:
            self.status_symbol.hide()

    def _setup_wrapped_widget(self):
        """Wrap up the alarm symbol and possible edit buttons in a layout with
        the `widget_controller`.
        """
        controller = self.widget_controller
        layout = QHBoxLayout()
        layout.addWidget(controller.widget)
        disp_widgets = self._add_alarm_symbol(layout)
        self.layout.addWidget(disp_widgets)

        if self.model.parent_component == 'EditableApplyLaterComponent':
            self._is_editable = True
            controller.proxy.on_trait_change(
                self._on_user_edit, 'edit_value,binding.config_update')
            layout.setContentsMargins(2, 2, 2, 2)
        else:
            layout.setContentsMargins(0, 0, 0, 0)

        # Tell the widget if it's editing
        controller.set_read_only(not self._is_editable)

        # Make the widget show something
        controller.finish_initialization()

    # ---------------------------------------------------------------------
    # Editing related code

    def _on_user_edit(self, binding, name, value):
        """Trait notification handler for when the value of a PropertyProxy
        changes from user or external action.
        """
        # Do some trait name filtering
        if name not in ('edit_value', 'config_update'):
            return
        self._update_background_color()

    def _update_background_color(self):
        proxy = self.widget_controller.proxy
        if not self._is_editable or proxy.binding is None:
            return

        if proxy.edit_value is not None:
            device_value = proxy.get_device_value()
            has_conflict = has_changes(proxy.binding, device_value,
                                       proxy.edit_value)
            if has_conflict:
                color = STATE_COLORS[State.UNKNOWN] + (128,)
            else:
                color = STATE_COLORS[State.CHANGING] + (128,)
            formatted_sheet = self._style_sheet.format(color)
            self.setStyleSheet(formatted_sheet)
        else:
            self.setStyleSheet('')
