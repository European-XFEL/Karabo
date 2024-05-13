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
from qtpy.QtWidgets import QWidget
from traits.api import (
    Bool, HasStrictTraits, Instance, List, Property, on_trait_change)

from karabo.common.scenemodel.api import BaseWidgetObjectData
from karabogui import background
from karabogui.binding.api import PropertyProxy, get_binding_value

from .util import get_class_const_trait


class BaseBindingController(HasStrictTraits):
    """The base class of controllers which mediate the connection between a
    data binding model (and a scene model) and a scene widget view.
    """
    # Scene data model describing the widget
    model = Instance(BaseWidgetObjectData, args=())
    # The proxy of the main property being visualized
    proxy = Instance(PropertyProxy)
    # Convenience property for getting all proxies for a widget
    proxies = Property(List, depends_on=['proxy', '_additional_proxies'])
    # The QWidget containing the binding visualization
    widget = Instance(QWidget)

    # Additional proxies added to the widget
    _additional_proxies = List(Instance(PropertyProxy))
    _showing = Bool(False)
    _deferred = Bool(False)

    # -------------------------------------------------------------------------
    # Subclass interface

    def add_proxy(self, proxy):
        """Implemented by subclasses to catch new `PropertyProxy` instances as
        they are added to the controller. Returns `True` if the controller
        can make use of the added proxy instance.

        OPTIONAL: Not all widgets are capable of visualizing mutiple properties
        """
        raise NotImplementedError

    def remove_proxy(self, proxy):
        """Implemented by subclasses to remove `PropertyProxy` instances.
        Returns `True` if the controller can make use of the removed proxy
        instance.

        OPTIONAL: Not all widgets allow removal of properties
        """
        raise NotImplementedError

    def binding_update(self, proxy):
        """Implemented by subclasses to receive notifications that the
        `binding` trait of a proxy attached to the controller has been updated.

        OPTIONAL: Not all widgets care when their bindings change.
        """

    def create_widget(self, parent):
        """Implemented by subclasses to create the widget needed to visualize
        a binding.
        """
        raise NotImplementedError

    @classmethod
    def initialize_model(cls, proxy, model):
        """Implemented by subclasses to initialize a model from a proxy.

        This method is only invoked on controller creation
        """

    def destroy_widget(self):
        """Implemented by subclasses to clean up anything in the widget which
        is otherwise not automatically cleaned up.

        OPTIONAL: Not all widgets will need additional cleanup.
        """

    def deferred_update(self):
        """Implemented by subclasses to update controller as requested by
        `update_later`.

        OPTIONAL: Override this method to run long-running code, and call
        `update_later` instead.
        """

    def set_read_only(self, readonly):
        """Implemented by subclasses to notify a widget of its read-only status

        OPTIONAL: (if can_edit=True passed to register_binding_controller)
        """

    def state_update(self, proxy):
        """Implemented by subclasses to receive notifications that the device
        parent of `proxy` has updated its state.

        OPTIONAL: Not all widgets care about device state
        """

    def value_update(self, proxy):
        """Implemented by subclasses to receive notifications that the
        data value in a proxy attached to the controller has been updated.

        "OPTIONAL": Not all widgets care when their values change, but most do.
        """

    # -------------------------------------------------------------------------
    # Public interface

    def create(self, parent):
        """Create the widget for this controller
        """
        self.widget = self.create_widget(parent)

    def destroy(self):
        """Destroys the widget, giving subclasses a chance to do some cleanup
        """
        self.destroy_widget()

        if self.widget:
            self.widget.setParent(None)
            self.widget = None

        self.hide()
        self.trait_setq(proxy=None, _additional_proxies=[])

    def finish_initialization(self):
        """Force the controller to update itself in the absence of a device
        update on its main proxy.
        """
        if self.widget is None or self.proxy.binding is None:
            return

        # controllers which specify themselves `_can_show_nothing` should
        # be able to deal with Undefined, i.e. their value_update() and
        # state_update() function should use get_binding_value() and
        # specify a proper default value if the proxy.value is Undefined
        self._proxy_update(self.proxy)

    def hide(self):
        """Hide the proxies. Stops monitoring the parent device of each proxy
        being visualized.
        """
        if not self._showing:
            return

        for proxy in self.proxies:
            proxy.stop_monitoring()
        self._showing = False

    def show(self):
        """Show the proxies. Starts monitoring the parent device of each proxy
        being visualized.
        """
        if self._showing:
            return

        for proxy in self.proxies:
            proxy.start_monitoring()
        self._showing = True

    def update_later(self):
        """Call longer-running code at a later time

        If the controller has code that takes a while to run, it should be
        moved to the `deferred_update` method and the widget should call
        `update_later` to schedule this update.

        This keeps the GUI responsive.

        This is especially important for properties which change often, as
        `deferred_update` will only be called after many updates have finished.
        """

        def updater():
            self.deferred_update()
            self._deferred = False

        if not self._deferred:
            background.executeLater(updater, background.Priority.BACKGROUND)
        self._deferred = True

    def visualize_additional_property(self, proxy):
        """Attempt to add an additional `PropertyProxy` to the controller. This
        should fail gracefully if the subclass has not implemented `add_proxy`.
        """
        # Check the binding type _if possible_. It's also OK when building
        # scenes to supply proxies which do not yet have a binding.
        binding = proxy.binding
        if binding is not None and not isinstance(binding, self._binding_type):
            return False  # Disallow unsupported binding types

        if proxy in self.proxies:
            return False  # Disallow duplicates

        try:
            if not self.add_proxy(proxy):
                return False
            # Only if `add_proxy` doesn't raise an exception
            self._additional_proxies.append(proxy)
            # Append the key to data model if it's not in there yet
            if proxy.key not in self.model.keys:
                self.model.keys.append(proxy.key)
            if self._showing:
                proxy.start_monitoring()
            self._proxy_update(proxy)
            return True  # The only successful exit from this method!
        except NotImplementedError:
            # Forget about it!
            return False

    def remove_additional_property(self, proxy):
        """Remove an additional property proxy"""
        if proxy not in self.proxies:
            return False  # Not there

        try:
            if not self.remove_proxy(proxy):
                return False
            if self._showing:
                # Stop monitoring if necessary
                proxy.stop_monitoring()

            # And finally stop tracking of this proxy
            self._additional_proxies.remove(proxy)
            self.model.keys.remove(proxy.key)
            return True
        except NotImplementedError:
            # Forget about it!
            return False

    def _proxy_update(self, proxy):
        if proxy.binding is None:
            return

        has_data = (proxy.binding.timestamp is not None)
        if has_data or get_class_const_trait(type(self), '_can_show_nothing'):
            self.value_update(proxy)

        state_binding = proxy.root_proxy.state_binding
        has_state = (state_binding is not None
                     and get_binding_value(state_binding))
        if has_state:
            self.state_update(proxy)

    def on_decline(self):
        """Implemented by subclasses to receive notifications that the value
        was declined.
        """

    def clear_widget(self):
        """Implemented by subclasses to receive notifications that the root
        proxy went offline."""

    def setEnabled(self, enable):
        """Implemented by subclasses to update controller as requested by
        a change in the access level.
        """
        if get_class_const_trait(type(self), '_can_edit'):
            self.widget.setEnabled(enable)

    def getInstanceId(self):
        """Retrieve the `instanceId` of the root proxy of this controller

        Note: Function added with Karabo 2.16.X
        """
        return self.proxy.root_proxy.device_id

    # -------------------------------------------------------------------------
    # Traits

    def _get_proxies(self):
        return (self.proxy,) + tuple(self._additional_proxies)

    @on_trait_change('proxy:binding,_additional_proxies:binding',
                     post_init=True)
    def _proxy_binding_update(self, proxy, name, binding):
        # One of the attached proxies got a new binding which is not None
        if binding is not None:
            self.binding_update(proxy)

    @on_trait_change('proxies.binding.config_update')
    def _proxy_value_update(self, binding, name, event):
        if self.widget is None or name != 'config_update':
            return

        try:
            # One of the attached proxies got a new value on its binding
            proxy = [p for p in self.proxies if p.binding is binding][0]
            if get_binding_value(proxy) is not None:
                self.value_update(proxy)
        except IndexError:
            pass

    @on_trait_change('proxies.root_proxy.state_binding.config_update')
    def _proxy_state_update(self, binding, name, event):
        if self.widget is None or name != 'config_update':
            return

        try:
            # One of the attached proxies got a new state on its device
            proxy = [p for p in self.proxies
                     if p.root_proxy.state_binding is binding][0]
            # Since this notification was from a different binding, we have
            # to check that the proxy's binding is there.
            if proxy.binding:
                self.state_update(proxy)
        except IndexError:
            pass

    def _widget_changed(self, new):
        """Tell the controller about its proxies whenever it gets a new widget.
        """
        if new is None:
            return

        for proxy in self.proxies:
            if proxy.binding is None:
                continue
            self.binding_update(proxy)
