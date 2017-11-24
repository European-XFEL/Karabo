from PyQt4.QtGui import QWidget
from traits.api import HasStrictTraits, Bool, Instance, List, Property

from karabo.common.scenemodel.api import BaseWidgetObjectData
from karabogui import background
from karabogui.binding.api import PropertyProxy


class BaseBindingController(HasStrictTraits):
    """The base class of controllers which mediate the connection between a
    data binding model (and a scene model) and a scene widget view.
    """
    # Scene data model describing the widget (optional)
    model = Instance(BaseWidgetObjectData)
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

    def create_widget(self, parent):
        """Implemented by subclasses to create the widget needed to visualize
        a binding.
        """
        raise NotImplementedError

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
        """Implemented by subclasses to notify a widget of its read-only status.

        OPTIONAL: (if read_only=False passed to register_binding_controller)
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
            if self._showing:
                proxy.start_monitoring()
            return True  # The only successful exit from this method!
        except NotImplementedError:
            # Forget about it!
            return False

    # -------------------------------------------------------------------------
    # Traits

    def _get_proxies(self):
        return (self.proxy,) + tuple(self._additional_proxies)
