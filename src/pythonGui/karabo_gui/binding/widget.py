from PyQt4.QtGui import QWidget
from traits.api import HasStrictTraits, Bool, Instance, List, Property

from karabo.common.scenemodel.api import BaseWidgetObjectData
from .proxy import PropertyProxy


class BaseBindingWidget(HasStrictTraits):
    # Scene data model describing the widget (optional)
    model = Instance(BaseWidgetObjectData, allow_none=True)
    # The proxy of the main property being visualized
    proxy = Instance(PropertyProxy)
    # Convenience property for getting all proxies for a widget
    proxies = Property(List, depends_on=['proxy', '_additional_proxies'])
    # The QWidget containing the binding visualization
    widget = Instance(QWidget)

    # Additional proxies added to the widget
    _additional_proxies = List(Instance(PropertyProxy))
    _showing = Bool(False)

    # -------------------------------------------------------------------------
    # Subclass interface

    def add_proxy(self, proxy):
        """Implemented by subclasses to catch new `PropertyProxy` instances as
        they are added to the widget.

        NOTE: Most widgets will not need this and can leave it unimplemented.
        """
        raise NotImplementedError

    def create_widget(self, parent):
        """Implemented by subclasses to create the widget needed to visualize
        a binding
        """
        raise NotImplementedError

    def destroy_widget(self):
        """Optionally implemented by subclasses to clean up anything in the
        widget which is otherwise not automatically cleaned up.
        """
        raise NotImplementedError

    def set_read_only(self, readonly):
        """Optionally (if read_only=False is passed to binding_widget)
        implemented by subclasses to notify a widget of its read-only status.
        """
        raise NotImplementedError

    # -------------------------------------------------------------------------
    # Public interface

    def create(self, parent):
        """Create the widget for this binding
        """
        self.widget = self.create_widget(parent)

    def destroy(self):
        """Destroys the widget, giving subclasses a chance to do some cleanup
        """
        try:
            self.destroy_widget()
        except NotImplementedError:
            pass  # It's OK not to implement

        if self.widget:
            self.widget.setParent(None)

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

    def visualize_additional_property(self, proxy):
        """Attempt to add an additional `PropertyProxy` to the widget. This
        should fail gracefully if the subclass has not implemented `add_proxy`.
        """
        if not isinstance(proxy, self._binding_type):
            return  # Disallow unsupported binding types

        if proxy in self.proxies:
            return  # Disallow duplicates

        # Add it to `_additional_proxies` (without trait notifications),
        # in case subclass logic depends on iteration over the `proxies`
        # property...
        self.trait_setq(_additional_proxies=self._additional_proxies + [proxy])
        try:
            self.add_proxy(proxy)
            # Only if `add_proxy` doesn't raise an exception
            if self._showing:
                proxy.start_monitoring()
        except NotImplementedError:
            # Forget about it!
            self.trait_setq(_additional_proxies=self._additional_proxies[:-1])

    # -------------------------------------------------------------------------
    # Traits

    def _get_proxies(self):
        return (self.proxy,) + tuple(self._additional_proxies)
