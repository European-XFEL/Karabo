from qtpy.QtCore import QRect, QSize, Qt
from qtpy.QtWidgets import QHBoxLayout, QLabel, QStackedLayout, QWidget

from karabo.common.api import ProxyStatus, State
from karabogui import globals as krb_globals
from karabogui.indicators import get_device_status_pixmap, STATE_COLORS
from karabogui.request import send_property_changes
from karabogui.util import generateObjectName
from karabogui.widgets.hints import KaraboSceneWidget
from .utils import get_proxy


class ControllerContainer(KaraboSceneWidget, QWidget):
    """A container for scene widgets which provides the interface needed
    by the scene tools.
    """
    def __init__(self, klass, model, parent):
        super(ControllerContainer, self).__init__(model=model, parent=parent)
        # Variables used by editable widgets
        self.is_editable = False
        self._style_sheet = ''

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

        # Trigger the status change once (might be offline)
        proxy = self.widget_controller.proxy.root_proxy
        self._proxy_status_changed("status", proxy.status)
        # ... and the access level check
        self.update_global_access_level(krb_globals.GLOBAL_ACCESS_LEVEL)

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
        if not (self.is_editable and self.widget_controller.proxy.visible):
            return

        send_property_changes(self.widget_controller.proxies)

    def decline_changes(self):
        """Undo any user-entered changes
        """
        if not (self.is_editable and self.widget_controller.proxy.visible):
            return

        for proxy in self.widget_controller.proxies:
            proxy.revert_edit()
        # allow the controller to react on the decline
        self.widget_controller.on_decline()

    def destroy(self):
        """Tell the controller to clean up
        """
        proxy = self.widget_controller.proxy
        proxy.on_trait_change(self._proxy_status_changed, 'existing',
                              remove=True)
        proxy.root_proxy.on_trait_change(self._proxy_status_changed, 'status',
                                         remove=True)
        if self.is_editable:
            proxy.on_trait_change(
                self._on_user_edit, 'edit_value,binding.config_update',
                remove=True)

        self.widget_controller.destroy()
        self.widget_controller = None

    def set_geometry(self, rect):
        self.model.trait_set(x=rect.x(), y=rect.y(),
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
        self.model.trait_set(x=new_pos.x(), y=new_pos.y())
        self.move(new_pos)

    def update_global_access_level(self, level):
        """Update the widget based on a new global access level.
        """
        # enable/disable based on level
        binding = self.widget_controller.proxy.binding
        if binding is not None:
            enabled = (binding.required_access_level <= level)
            self.widget_controller.setEnabled(enabled)

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

    def _create_controller(self, klass, proxies):
        """Create the binding controller instance.
        """
        controller = klass(proxy=proxies[0], model=self.model)
        controller.create(self)
        for proxy in proxies[1:]:
            # XXX: Assert that this returns True??
            controller.visualize_additional_property(proxy)

        # Attach a handler for the 'status' trait of our main device
        proxy = controller.proxy
        proxy.on_trait_change(self._proxy_status_changed, 'existing')
        proxy.root_proxy.on_trait_change(self._proxy_status_changed, 'status')

        return controller

    def _proxy_status_changed(self, name, value):
        """Traits notification callback when the status of the proxy changes.

        The existing property of the proxy is evaluated here as it is coupled
        to the schema status change!
        """
        proxy = self.widget_controller.proxy
        if name == 'status':
            existing = proxy.existing
            status = value
            if status is ProxyStatus.OFFLINE:
                self.widget_controller.clear_widget()
        elif name == 'existing':
            existing = value
            status = proxy.root_proxy.status
        if not existing:
            status = ProxyStatus.MISSING

        # NOTE: The error notification on the scene is DEPRECATED as the online
        # device status can not anymore be separated from the proxy status
        pixmap = get_device_status_pixmap(status, False)
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
        layout = ControllerLayout()
        layout.addWidget(controller.widget)

        # create our color layout
        layout_widget = QWidget()
        layout_widget.setLayout(layout)
        objectName = generateObjectName(self)
        layout_widget.setObjectName(objectName)
        self._style_sheet = ('QWidget#{}'.format(objectName) +
                             ' {{ background-color : rgba{}; }}')

        if self.model.parent_component == 'EditableApplyLaterComponent':
            self.is_editable = True
            controller.proxy.on_trait_change(
                self._on_user_edit, 'edit_value,binding.config_update')
            layout.setContentsMargins(2, 1, 2, 1)
        else:
            layout.setContentsMargins(0, 0, 1, 1)

        # Tell the widget if it's editing
        controller.set_read_only(not self.is_editable)

        # Make the widget show something
        controller.finish_initialization()
        self.layout.addWidget(layout_widget)

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
        if not self.is_editable or proxy.binding is None:
            return

        if proxy.edit_value is not None:
            color = STATE_COLORS[State.CHANGING] + (128,)
        else:
            # XXX: We make sure not to lose our stylesheet and apply a
            # transparent color!
            color = (0, 0, 0, 0)

        formatted_sheet = self._style_sheet.format(color)
        self.setStyleSheet(formatted_sheet)


class ControllerLayout(QHBoxLayout):
    """This is the layout for the layout widget that contains the controller
       widget.

       There is a need to subclass the minimum size to not dynamically update
       the minimum size depending on the content margins (for the borders)."""

    def minimumSize(self):
        size = super(ControllerLayout, self).minimumSize()
        if not size.isEmpty():
            left, top, right, bottom = self.getContentsMargins()
            size = QSize(size.width() - (left + right),
                         size.height() - (top + bottom))
        return size
