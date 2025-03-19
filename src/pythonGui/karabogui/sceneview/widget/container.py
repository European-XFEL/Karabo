# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from qtpy.QtCore import QRect, QSize, Qt
from qtpy.QtWidgets import QHBoxLayout, QLabel, QStackedLayout, QWidget

import karabogui.access as krb_access
from karabo.common.api import State
from karabogui.binding.api import ProxyStatus
from karabogui.indicators import STATE_COLORS, get_device_status_pixmap
from karabogui.request import send_property_changes
from karabogui.util import generateObjectName
from karabogui.widgets.hints import KaraboSceneWidget

from .utils import get_proxy, is_controller_enabled


class ControllerContainer(KaraboSceneWidget, QWidget):
    """A container for scene widgets which provides the interface needed
    by the scene tools.
    """

    def __init__(self, klass, model, parent):
        super().__init__(model=model, parent=parent)
        # Variables used by editable widgets
        self.is_editable = False
        self._style_sheet = ""

        self.status_symbol = QLabel("", self)
        self.status_symbol.setAttribute(Qt.WA_TransparentForMouseEvents)

        self.layout = QStackedLayout(self)
        self.layout.setStackingMode(QStackedLayout.StackAll)
        self.layout.addWidget(self.status_symbol)

        proxies = [get_proxy(*key.split(".", 1)) for key in self.model.keys]
        self.widget_controller = self._create_controller(klass, proxies)
        self._setup_wrapped_widget()
        self.setGeometry(QRect(model.x, model.y, model.width, model.height))
        # Trigger the status change once (might be offline)
        proxy = self.widget_controller.proxy.root_proxy
        self._proxy_status_changed("status", proxy.status)

    def add_proxies(self, proxies):
        """Add proxies to a controller which allows more than one proxy.

        :returns: `True` if successful, otherwise `False`
        """
        controller = self.widget_controller
        proxies_added = []
        for proxy in proxies:
            if not controller.visualize_additional_property(proxy):
                continue
            proxies_added.append(proxy)
            if self.is_editable:
                proxy.on_trait_change(
                    self._on_user_edit, "edit_value,binding.config_update")
                self.setEditableToolTip(proxy.binding.requiredAccessLevel)
            else:
                self.setToolTip(", ".join(self.model.keys))
        return len(proxies_added) == len(proxies)

    def remove_proxies(self, proxies):
        """Remove proxies from a controller which allows more than one proxy.

        :returns: `True` if successful, otherwise `False`
        """
        controller = self.widget_controller
        proxies_removed = []
        for proxy in proxies:
            if not controller.remove_additional_property(proxy):
                continue
            proxies_removed.append(proxy)
            if self.is_editable:
                proxy.on_trait_change(
                    self._on_user_edit, "edit_value,binding.config_update",
                    remove=True)
                self.setEditableToolTip(proxy.binding.requiredAccessLevel)
            else:
                self.setToolTip(", ".join(self.model.keys))
        return len(proxies_removed) == len(proxies)

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
        """Tell the controller to clean up"""
        proxy = self.widget_controller.proxy
        proxy.on_trait_change(self._proxy_status_changed, "existing",
                              remove=True)
        proxy.root_proxy.on_trait_change(self._proxy_status_changed, "status",
                                         remove=True)
        if self.is_editable:
            for proxy in self.widget_controller.proxies:
                proxy.on_trait_change(
                    self._on_user_edit, "edit_value,binding.config_update",
                    remove=True)
            if proxy.binding is None:
                proxy.on_trait_change(
                    self._proxy_binding_changed, "binding.schema_update",
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
        """Enable the widget based on a new global access level."""
        proxy = self.widget_controller.proxy
        binding = proxy.binding
        if binding is not None:
            enabled = is_controller_enabled(proxy, level)
            self.widget_controller.setEnabled(enabled)
            if self.is_editable:
                self.setEditableToolTip(binding.requiredAccessLevel)

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
        super().keyPressEvent(event)

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

        # Attach a handler for the "status" trait of our main device
        proxy = controller.proxy
        proxy.on_trait_change(self._proxy_status_changed, "existing")
        proxy.root_proxy.on_trait_change(self._proxy_status_changed, "status")
        return controller

    def setEditableToolTip(self, level):
        """Set a tooltip for an editable controller"""
        keys = ", ".join(self.model.keys)
        allowed = krb_access.GLOBAL_ACCESS_LEVEL >= level
        tooltip = f"AccessLevel: {level.name} - Access: {allowed}\n\n{keys}"
        self.setToolTip(tooltip)

    def _proxy_binding_changed(self, proxy, name, new):
        """Traits notification callback when the binding of the proxy changes.

        This is set to synchronize the access level of the widget
        """
        proxy.on_trait_change(self._proxy_binding_changed,
                              "binding.schema_update", remove=True)
        self.setEditableToolTip(proxy.binding.requiredAccessLevel)

    def _proxy_status_changed(self, name, value):
        """Traits notification callback when the status of the proxy changes.

        The existing property of the proxy is evaluated here as it is coupled
        to the schema status change!
        """
        proxy = self.widget_controller.proxy
        if name == "status":
            existing = proxy.existing
            status = value
            if status is ProxyStatus.OFFLINE:
                self.widget_controller.clear_widget()
        elif name == "existing":
            existing = value
            status = proxy.root_proxy.status
        if not existing:
            status = ProxyStatus.MISSING

        pixmap = get_device_status_pixmap(status)
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
        self._style_sheet = (f"QWidget#{objectName}" +
                             " {{ background-color : rgba{}; }}")

        proxies = controller.proxies
        if self.model.parent_component == "EditableApplyLaterComponent":
            self.is_editable = True
            for proxy in proxies:
                proxy.on_trait_change(
                    self._on_user_edit, "edit_value,binding.config_update")
            layout.setContentsMargins(2, 1, 2, 1)
        else:
            layout.setContentsMargins(0, 0, 1, 1)

        # Tell the widget if it's editing
        controller.set_read_only(not self.is_editable)
        if self.is_editable:
            binding = proxies[0].binding
            if binding is None:
                proxy.on_trait_change(self._proxy_binding_changed,
                                      "binding.schema_update")
                # Use normal tooltip and wait for update
                self.setToolTip(", ".join(self.model.keys))
            else:
                self.setEditableToolTip(level=binding.requiredAccessLevel)
        else:
            self.setToolTip(", ".join(self.model.keys))

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
        if name not in ("edit_value", "config_update"):
            return

        self._update_background_color()

    def _update_background_color(self):
        proxy = self.widget_controller.proxy
        if not self.is_editable or proxy.binding is None:
            return

        proxies = self.widget_controller.proxies
        if any(proxy.edit_value is not None for proxy in proxies):
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
        size = super().minimumSize()
        if not size.isEmpty():
            left, top, right, bottom = self.getContentsMargins()
            size = QSize(size.width() - (left + right),
                         size.height() - (top + bottom))
        return size
